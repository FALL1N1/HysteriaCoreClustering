/*
REWRITTEN FROM SCRATCH BY XINEF, IT OWNS NOW!
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "naxxramas.h"
#include "PassiveAI.h"

enum Spells
{
    SPELL_WEB_WRAP                      = 28622,
    SPELL_WEB_SPRAY_10                  = 29484,
    SPELL_WEB_SPRAY_25                  = 54125,
    SPELL_POISON_SHOCK_10               = 28741,
    SPELL_POISON_SHOCK_25               = 54122,
    SPELL_NECROTIC_POISON_10            = 54121,
    SPELL_NECROTIC_POISON_25            = 28776,
    SPELL_FRENZY_10                     = 54123,
    SPELL_FRENZY_25                     = 54124,
};

enum Events
{
    EVENT_SPELL_WEB_SPRAY               = 1,
    EVENT_SPELL_POISON_SHOCK            = 2,
    EVENT_SPELL_NECROTIC_POISON         = 3,
    EVENT_WEB_WRAP                      = 4,
    EVENT_HEALTH_CHECK                  = 5,
    EVENT_SUMMON_SPIDERLINGS            = 6,
};

const float WEB_WRAP_MOVE_SPEED = 20.0f;

struct WebTargetSelector : public std::unary_function<Unit*, bool>
{
    WebTargetSelector(Unit* maexxna) : _maexxna(maexxna) {}
    bool operator()(Unit const* target) const
    {
        if (target->GetTypeId() != TYPEID_PLAYER) // never web nonplayers (pets, guardians, etc.)
            return false;
        if (_maexxna->GetVictim() == target) // never target tank
            return false;
        if (target->HasAura(SPELL_WEB_WRAP)) // never target targets that are already webbed
            return false;
        return true;
    }

private:
    const Unit* _maexxna;
};

enum Misc
{
    NPC_WEB_WRAP                        = 16486,
    NPC_MAEXXNA_SPIDERLING                = 17055,
};

#define MAX_WRAP_POSITION  7
const Position WrapPositions[MAX_WRAP_POSITION] =
{
    { 3453.818f, -3854.651f, 308.7581f, 4.362833f },
    { 3535.042f, -3842.383f, 300.795f, 3.179324f },
    { 3538.399f, -3846.088f, 299.964f, 4.310297f },
    { 3548.464f, -3854.676f, 298.6075f, 4.546609f },
    { 3557.663f, -3870.123f, 297.5027f, 3.756433f },
    { 3560.546f, -3879.353f, 297.4843f, 2.508937f },
    { 3562.535f, -3892.507f, 298.532f, 6.022466f },
};

/*const Position PosWrap[3] =
{
    {3546.796f, -3869.082f, 296.450f},
    {3531.271f, -3847.424f, 299.450f},
    {3497.067f, -3843.384f, 302.384f}
}; */

class boss_maexxna : public CreatureScript
{
public:
    boss_maexxna() : CreatureScript("boss_maexxna") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_maexxnaAI (pCreature);
    }

    struct boss_maexxnaAI : public ScriptedAI
    {
        boss_maexxnaAI(Creature *c) : ScriptedAI(c), summons(me)
        {
            pInstance = me->GetInstanceScript();
        }

        InstanceScript* pInstance;
        EventMap events;
        SummonList summons;

        bool IsInRoom()
        {
            if (me->GetExactDist(3486.6f, -3890.6f, 291.8f) > 100.0f)
            {
                EnterEvadeMode();
                return false;
            }

            return true;
        }

        void Reset()
        {
            events.Reset();
            summons.DespawnAll();

            if (pInstance)
            {
                pInstance->SetData(EVENT_MAEXXNA, NOT_STARTED);
                    if (GameObject* go = me->GetMap()->GetGameObject(pInstance->GetData64(DATA_MAEXXNA_GATE)))
                        go->SetGoState(GO_STATE_ACTIVE);
            }
        }

        void EnterCombat(Unit *who)
        {
            me->SetInCombatWithZone();
            events.ScheduleEvent(EVENT_WEB_WRAP, 20000);
            events.ScheduleEvent(EVENT_SPELL_WEB_SPRAY, 40000);
            events.ScheduleEvent(EVENT_SPELL_POISON_SHOCK, 10000);
            events.ScheduleEvent(EVENT_SPELL_NECROTIC_POISON, 5000);
            events.ScheduleEvent(EVENT_HEALTH_CHECK, 1000);
            events.ScheduleEvent(EVENT_SUMMON_SPIDERLINGS, 30000);

            if (pInstance)
            {
                pInstance->SetData(EVENT_MAEXXNA, IN_PROGRESS);
                if (GameObject* go = me->GetMap()->GetGameObject(pInstance->GetData64(DATA_MAEXXNA_GATE)))
                    go->SetGoState(GO_STATE_READY);
            }
        }

        void JustDied(Unit* Killer)
        {
            if (pInstance)
                pInstance->SetData(EVENT_MAEXXNA, DONE);
        }

        void JustSummoned(Creature* cr)
        {
            if (cr->GetEntry() == NPC_MAEXXNA_SPIDERLING)
            {
                cr->SetInCombatWithZone();
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    cr->AI()->AttackStart(target);
            }

            summons.Summon(cr);
        }

        void KilledUnit(Unit* who)
        {
            if (who->GetTypeId() == TYPEID_PLAYER && pInstance)
                pInstance->SetData(DATA_IMMORTAL_FAIL, 0);
        }

        void UpdateAI(uint32 diff)
        {
            if (!IsInRoom())
                return;

            if (!UpdateVictim())
                return;

            events.Update(diff);
            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            switch (events.GetEvent())
                {
                case EVENT_WEB_WRAP:
                {
                std::list<Unit*> targets;
                SelectTargetList(targets, WebTargetSelector(me), RAID_MODE(1, 2), SELECT_TARGET_RANDOM);
                if (!targets.empty())
                {
                    me->MonsterTextEmote("%s spins her web into a cocoon!", 0, true);
                    //Talk(EMOTE_WEB_WRAP);
                    int8 wrapPos = -1;
                    for (Unit* target : targets)
                    {
                        if (wrapPos == -1) // allow all positions on the first target
                            wrapPos = urand(0, MAX_WRAP_POSITION - 1);
                        else // on subsequent iterations, only allow positions that are not equal to the previous one (this is sufficient since we should only have two targets at most, ever)
                            wrapPos = (wrapPos + urand(1, MAX_WRAP_POSITION - 1)) % MAX_WRAP_POSITION;

                        target->RemoveAura(sSpellMgr->GetSpellIdForDifficulty(SPELL_WEB_SPRAY_10,  me));
                        target->RemoveAura(sSpellMgr->GetSpellIdForDifficulty(SPELL_WEB_SPRAY_25, me));
                        if (Creature* wrap = DoSummon(NPC_WEB_WRAP, WrapPositions[wrapPos], 70 * IN_MILLISECONDS, TEMPSUMMON_TIMED_DESPAWN))
                        {
                            wrap->AI()->SetGUID(target->GetGUID()); // handles application of debuff
                            target->GetMotionMaster()->MoveJump(WrapPositions[wrapPos], WEB_WRAP_MOVE_SPEED, WEB_WRAP_MOVE_SPEED); // move after stun to avoid stun cancelling move
                        }
                    }
                }
                events.RepeatEvent(40000);
                break;
                }
                case EVENT_SPELL_WEB_SPRAY:
                    me->MonsterTextEmote("%s sprays strands of web everywhere!", 0, true);
                    me->CastSpell(me, RAID_MODE(SPELL_WEB_SPRAY_10, SPELL_WEB_SPRAY_25), true);
                    events.RepeatEvent(40000);
                    break;
                case EVENT_SPELL_POISON_SHOCK:
                    me->CastSpell(me->GetVictim(), RAID_MODE(SPELL_POISON_SHOCK_10, SPELL_POISON_SHOCK_25), false);
                    events.RepeatEvent(10000);
                    break;
                case EVENT_SPELL_NECROTIC_POISON:
                    me->CastSpell(me->GetVictim(), RAID_MODE(SPELL_NECROTIC_POISON_10, SPELL_NECROTIC_POISON_25), false);
                    events.RepeatEvent(urand(10000,30000));
                    break;
                case EVENT_SUMMON_SPIDERLINGS:
                    me->MonsterTextEmote("Spiderlings appear on the web!", 0, true);
                    for (uint8 i = 0; i < 8; ++i)
                        me->SummonCreature(NPC_MAEXXNA_SPIDERLING, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
                    events.RepeatEvent(40000);
                    break;
                case EVENT_HEALTH_CHECK:
                    if (me->GetHealthPct() < 30)
                    {
                        me->CastSpell(me, RAID_MODE(SPELL_FRENZY_10, SPELL_FRENZY_25), true);
                        events.PopEvent();
                        break;
                    }
                    events.RepeatEvent(1000);
                    break;
                /*case EVENT_WEB_WRAP:
                    me->MonsterTextEmote("%s spins her web into a cocoon!", 0, true);
                    for (uint8 i = 0; i < RAID_MODE(1,2); ++i)
                        if (Unit *target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0, true, -SPELL_WEB_WRAP))
                        {
                            target->RemoveAura(RAID_MODE(SPELL_WEB_SPRAY_10, SPELL_WEB_SPRAY_25));
                            uint8 pos = urand(0,2);

                            target->GetMotionMaster()->MoveJump(PosWrap[pos].GetPositionX(), PosWrap[pos].GetPositionY(), PosWrap[pos].GetPositionZ(), 20, 20);
                            if (Creature *wrap = me->SummonCreature(NPC_WEB_WRAP, PosWrap[pos].GetPositionX(), PosWrap[pos].GetPositionY(), PosWrap[pos].GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 60000))
                                wrap->AI()->SetGUID(target->GetGUID());
                        }
                    events.RepeatEvent(40000);
                    break; */
            }

            DoMeleeAttackIfReady();
        }
    };
};

class boss_maexxna_webwrap : public CreatureScript
{
public:
    boss_maexxna_webwrap() : CreatureScript("boss_maexxna_webwrap") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_maexxna_webwrapAI (pCreature);
    }

    struct boss_maexxna_webwrapAI : public NullCreatureAI
    {
        boss_maexxna_webwrapAI(Creature *c) : NullCreatureAI(c), victimGUID(0) {}

        uint64 victimGUID;
        uint32 visibleTimer;

        void InitializeAI() override
        {
            me->SetVisible(false);
        }

        void SetGUID(uint64 guid, int32 param)
        {
            if (!guid)
                return;
            victimGUID = guid;
            if (me->m_spells[0] && victimGUID)
                if (Unit *victim = ObjectAccessor::GetUnit(*me, victimGUID))
                {
                    visibleTimer = (me->GetDistance2d(victim) / WEB_WRAP_MOVE_SPEED + 0.5f) * IN_MILLISECONDS;
                    victim->CastSpell(victim, me->m_spells[0], true, NULL, NULL, me->GetGUID());
                }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!visibleTimer)
                return;

            if (diff >= visibleTimer)
            {
                visibleTimer = 0;
                me->SetVisible(true);
            }
            else
                visibleTimer -= diff;
        }

        void JustDied(Unit *killer)
        {
            if (me->m_spells[0] && victimGUID)
                if (Unit *victim = ObjectAccessor::GetUnit(*me, victimGUID))
                    victim->RemoveAurasDueToSpell(me->m_spells[0], me->GetGUID());

            me->DespawnOrUnsummon(5 * IN_MILLISECONDS);
        }
    };
};

void AddSC_boss_maexxna()
{
    new boss_maexxna();
    new boss_maexxna_webwrap();
}

