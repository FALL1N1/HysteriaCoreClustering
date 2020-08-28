/*
REWRITTEN FROM SCRATCH BY XINEF, IT OWNS NOW!
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "naxxramas.h"

enum Yells
{
    SAY_AGGRO                                   = 0,
    SAY_SLAY                                    = 1,
    SAY_DEATH                                   = 2,
    EMOTE_BERSERK                               = 3,
    EMOTE_ENRAGE                                = 4
};

enum Spells
{
    SPELL_HATEFUL_STRIKE_10         = 41926,
    SPELL_HATEFUL_STRIKE_25         = 59192,
    SPELL_FRENZY                    = 28131,
    SPELL_BERSERK                   = 26662,
    SPELL_SLIME_BOLT                = 32309,
};

enum HatefulThreatAmounts
{
    HATEFUL_THREAT_AMT = 1000,
};

enum Events
{
    EVENT_HEALTH_CHECK              = 1,
    EVENT_SPELL_HATEFUL_STRIKE      = 2,
    EVENT_SPELL_SLIME_BOLT          = 3,
    EVENT_SPELL_BERSERK             = 4,
};

enum Misc
{
    ACHIEV_TIMED_START_EVENT        = 10286,
};

class boss_patchwerk : public CreatureScript
{
public:
    boss_patchwerk() : CreatureScript("boss_patchwerk") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_patchwerkAI (pCreature);
    }

    struct boss_patchwerkAI : public ScriptedAI
    {
        boss_patchwerkAI(Creature *c) : ScriptedAI(c) 
        {
            pInstance = me->GetInstanceScript();
        }

        EventMap events;
        InstanceScript* pInstance;

        void Reset()
        {
            events.Reset();
            if (pInstance)
                pInstance->SetData(EVENT_PATCHWERK, NOT_STARTED);
        }

        void KilledUnit(Unit* who)
        {
            if (who->GetTypeId() != TYPEID_PLAYER)
                return;

            if (!urand(0,3))
                Talk(SAY_SLAY);

            if (pInstance)
                pInstance->SetData(DATA_IMMORTAL_FAIL, 0);
        }

        void JustDied(Unit* Killer)
        {
            Talk(SAY_DEATH);
            if (pInstance)
                pInstance->SetData(EVENT_PATCHWERK, DONE); 
        }

        void EnterCombat(Unit *who)
        {
            Talk(SAY_AGGRO);
            
            //me->CallForHelp(250.0f);
            me->SetInCombatWithZone();
            events.ScheduleEvent(EVENT_SPELL_HATEFUL_STRIKE, 1200);
            events.ScheduleEvent(EVENT_SPELL_BERSERK, 360000);
            events.ScheduleEvent(EVENT_HEALTH_CHECK, 1000);

            if (pInstance)
            {
                pInstance->DoStartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_TIMED_START_EVENT);
                pInstance->SetData(EVENT_PATCHWERK, IN_PROGRESS);
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);
            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            switch (events.GetEvent())
            {
                case EVENT_SPELL_HATEFUL_STRIKE:
                {
                    // Hateful Strike targets the highest non-MT threat in melee range on 10man
                    // and the higher HP target out of the two highest non-MT threats in melee range on 25man
                    float MostThreat = 0.0f;
                    Unit* secondThreatTarget = NULL;
                    Unit* thirdThreatTarget = NULL;

                    std::list<HostileReference*>::const_iterator i = me->getThreatManager().getThreatList().begin();
                    for (; i != me->getThreatManager().getThreatList().end(); ++i)
                    { // find second highest
                        Unit* target = (*i)->getTarget();
                        if (target->IsAlive() && target != me->GetVictim() && (*i)->getThreat() >= MostThreat && me->IsWithinMeleeRange(target))
                        {
                            MostThreat = (*i)->getThreat();
                            secondThreatTarget = target;
                        }
                    }

                    if (secondThreatTarget && Is25ManRaid())
                    { // find third highest
                        MostThreat = 0.0f;
                        i = me->getThreatManager().getThreatList().begin();
                        for (; i != me->getThreatManager().getThreatList().end(); ++i)
                        {
                            Unit* target = (*i)->getTarget();
                            if (target->IsAlive() && target != me->GetVictim() && target != secondThreatTarget && (*i)->getThreat() >= MostThreat && me->IsWithinMeleeRange(target))
                            {
                                MostThreat = (*i)->getThreat();
                                thirdThreatTarget = target;
                            }
                        }
                    }

                    Unit* pHatefulTarget = NULL;
                    if (!thirdThreatTarget)
                        pHatefulTarget = secondThreatTarget;
                    else if (secondThreatTarget)
                        pHatefulTarget = (secondThreatTarget->GetHealth() < thirdThreatTarget->GetHealth()) ? thirdThreatTarget : secondThreatTarget;

                    if (!pHatefulTarget)
                        pHatefulTarget = me->GetVictim();

                    if (pHatefulTarget)
                        me->CastSpell(pHatefulTarget, RAID_MODE(SPELL_HATEFUL_STRIKE_10, SPELL_HATEFUL_STRIKE_25), false);
    
                    // add threat to highest threat targets
                    if (me->GetVictim() && me->IsWithinMeleeRange(me->GetVictim()))
                        me->getThreatManager().addThreat(me->GetVictim(), HATEFUL_THREAT_AMT);
                    if (secondThreatTarget)
                        me->getThreatManager().addThreat(secondThreatTarget, HATEFUL_THREAT_AMT);
                    if (thirdThreatTarget)
                        me->getThreatManager().addThreat(thirdThreatTarget, HATEFUL_THREAT_AMT); // this will only ever be used in 25m

                    events.RepeatEvent(1000);
                    break;
                }
                case EVENT_SPELL_BERSERK:
                    Talk(EMOTE_BERSERK);
                    me->CastSpell(me, SPELL_BERSERK, true);
                    events.ScheduleEvent(EVENT_SPELL_SLIME_BOLT, 2000);
                    events.PopEvent();
                    break;
                case EVENT_SPELL_SLIME_BOLT:
                    me->CastSpell(me, SPELL_SLIME_BOLT, false);
                    events.RepeatEvent(3000);
                    break;
                case EVENT_HEALTH_CHECK:
                    if (me->GetHealthPct() <= 5)
                    {
                        Talk(EMOTE_ENRAGE);
                        me->CastSpell(me, SPELL_FRENZY, true);
                        events.PopEvent();
                        break;
                    }
                    events.RepeatEvent(1000);
                    break;

            }

            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_patchwerk()
{
    new boss_patchwerk();
}