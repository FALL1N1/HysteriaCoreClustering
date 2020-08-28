/*

 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "Vehicle.h"
#include "ObjectMgr.h"
#include "ScriptedEscortAI.h"
#include "CombatAI.h"
#include "PassiveAI.h"
#include "Player.h"
#include "SpellInfo.h"
#include "CreatureTextMgr.h"
#include "PetAI.h"

// Ours
enum eyeOfAcherus
{
    EVENT_REMOVE_CONTROL            = 1,
    EVENT_SPEAK_1                   = 2,
    EVENT_LAUNCH                    = 3,
    EVENT_REGAIN_CONTROL            = 4,

    EYE_TEXT_LAUNCH                 = 0,
    EYE_TEXT_CONTROL                = 1,

    SPELL_EYE_OF_ACHERUS_VISUAL     = 51892,
};

enum TheEmblazonedRuneblade
{
    SPELL_EMBLAZON_RUNEBLADE = 51769,
    SPELL_SHADOW_STORM       = 51738,

    NPC_RUNEBLADED_SWORD     = 28476,
};

class npc_runeforge_trigger : public CreatureScript
{
public:
    npc_runeforge_trigger() : CreatureScript("npc_runeforge_trigger") {}

    struct npc_runeforge_triggerAI : public CreatureAI
    {
        npc_runeforge_triggerAI(Creature* creature) : CreatureAI(creature) {}

        void SpellHit(Unit* caster, SpellInfo const* spell)
        {
            if (spell->Id == SPELL_EMBLAZON_RUNEBLADE)
            {
                if (Player* player = caster->ToPlayer())
                {
                    DoCast(me, SPELL_SHADOW_STORM, true);
                    caster->CastSpell(caster, SPELL_SHADOW_STORM, true);
                    me->SummonCreature(NPC_RUNEBLADED_SWORD, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ() + 1.25, me->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 20 * IN_MILLISECONDS);
                }
            }
        }

        void UpdateAI(uint32 diff) { }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_runeforge_triggerAI(creature);
    }
};

class npc_runebladed_sword : public CreatureScript
{
public:
    npc_runebladed_sword() : CreatureScript("npc_runebladed_sword") {}

    struct npc_runebladed_swordAI : public CreatureAI
    {
        npc_runebladed_swordAI(Creature* creature) : CreatureAI(creature) {}

        void Reset()
        {
            DoCast(me, SPELL_SHADOW_STORM, true);
            me->SetCanFly(true);
            me->GetMotionMaster()->MoveRotate(200000, ROTATE_DIRECTION_RIGHT);
        }

        void UpdateAI(uint32 diff) { }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_runebladed_swordAI(creature);
    }
};

class npc_eye_of_acherus : public CreatureScript
{
public:
    npc_eye_of_acherus() : CreatureScript("npc_eye_of_acherus") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_eye_of_acherusAI(creature);
    }

    struct npc_eye_of_acherusAI : public NullCreatureAI
    {
        npc_eye_of_acherusAI(Creature* creature) : NullCreatureAI(creature) { }

        EventMap events;

        void InitializeAI()
        {
            events.Reset();
            events.ScheduleEvent(EVENT_REMOVE_CONTROL, 500);
            events.ScheduleEvent(EVENT_SPEAK_1, 4000);
            events.ScheduleEvent(EVENT_LAUNCH, 7000);

            me->CastSpell(me, SPELL_EYE_OF_ACHERUS_VISUAL, true);
        }

        void MovementInform(uint32 type, uint32 point)
        {
            if (type == ESCORT_MOTION_TYPE && me->movespline->Finalized())
                events.ScheduleEvent(EVENT_REGAIN_CONTROL, 1000);
        }

        void SetControl(Player* player, bool on)
        {
            WorldPacket data(SMSG_CLIENT_CONTROL_UPDATE, me->GetPackGUID().size()+1);
            data.append(me->GetPackGUID());
            data << uint8(on ? 1 : 0);
            player->GetSession()->SendPacket(&data);
        }

        void JustSummoned(Creature* creature)
        {
            if (Unit* target = creature->SelectNearbyTarget())
                creature->AI()->AttackStart(target);
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);
            switch (events.ExecuteEvent())
            {
                case EVENT_REMOVE_CONTROL:
                    if (Player* player = me->GetCharmerOrOwnerPlayerOrPlayerItself())
                        SetControl(player, false);
                    break;
                case EVENT_SPEAK_1:
                    Talk(EYE_TEXT_LAUNCH, me->GetCharmerOrOwnerPlayerOrPlayerItself());
                    break;
                case EVENT_LAUNCH:
                {
                    Movement::PointsArray path;
                    path.push_back(G3D::Vector3(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ()));
                    path.push_back(G3D::Vector3(me->GetPositionX()-40.0f, me->GetPositionY(), me->GetPositionZ()+10.0f));
                    path.push_back(G3D::Vector3(1768.0f, -5876.0f, 153.0f));
                    me->GetMotionMaster()->MoveSplinePath(&path);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NON_ATTACKABLE);
                    break;
                }
                case EVENT_REGAIN_CONTROL:
                    if (Player* player = me->GetCharmerOrOwnerPlayerOrPlayerItself())
                    {
                        SetControl(player, true);
                        Talk(EYE_TEXT_CONTROL, player);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NON_ATTACKABLE);
                    }
                    break;
            }
        }
    };
};

class spell_q12641_death_comes_from_on_high_summon_ghouls : public SpellScriptLoader
{
    public:
        spell_q12641_death_comes_from_on_high_summon_ghouls() : SpellScriptLoader("spell_q12641_death_comes_from_on_high_summon_ghouls") { }

        class spell_q12641_death_comes_from_on_high_summon_ghouls_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_q12641_death_comes_from_on_high_summon_ghouls_SpellScript);

            void HandleScriptEffect(SpellEffIndex effIndex)
            {
                PreventHitEffect(effIndex);
                if (Unit* target = GetHitUnit())
                    GetCaster()->CastSpell(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 54522, true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_q12641_death_comes_from_on_high_summon_ghouls_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_q12641_death_comes_from_on_high_summon_ghouls_SpellScript();
        }
};

#define GOSSIP_ACCEPT_DUEL      "I challenge you, death knight!"

enum deathsChallenge
{
    SPELL_DUEL                  = 52996,
    //SPELL_DUEL_TRIGGERED        = 52990,
    SPELL_DUEL_VICTORY          = 52994,
    SPELL_DUEL_FLAG             = 52991,

    SAY_DUEL                    = 0,

    QUEST_DEATH_CHALLENGE       = 12733,
    FACTION_HOSTILE             = 2068,

    DATA_IN_PROGRESS            = 0,

    EVENT_SPEAK                    = 1, // 1 - 6
    EVENT_DUEL_LOST             = 7, // 7 - 8
};

// 28406
#define NPC_DK_INITIATE 28406
class npc_death_knight_initiate : public CreatureScript
{
public:
    npc_death_knight_initiate() : CreatureScript("npc_death_knight_initiate") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();
        if (action == GOSSIP_ACTION_INFO_DEF)
        {
            player->CLOSE_GOSSIP_MENU();

            if (player->IsInCombat() || creature->IsInCombat() || player->IsMounted())
                return true;

            if (creature->AI()->GetData(DATA_IN_PROGRESS))
                return true;

            creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
            creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_15);

            player->CastSpell(creature, SPELL_DUEL, false);
            player->CastSpell(player, SPELL_DUEL_FLAG, true);
        }
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (player->GetQuestStatus(QUEST_DEATH_CHALLENGE) == QUEST_STATUS_INCOMPLETE && creature->IsFullHealth())
        {
            if (player->HealthBelowPct(10))
                return true;

            if (player->IsInCombat() || creature->IsInCombat())
                return true;

            if (!creature->AI()->GetData(player->GetGUIDLow()))
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ACCEPT_DUEL, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);

            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        }
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_death_knight_initiateAI(creature);
    }

    struct npc_death_knight_initiateAI : public CombatAI
    {
        npc_death_knight_initiateAI(Creature* creature) : CombatAI(creature) { }

        bool _duelInProgress;
        uint64 _duelGUID;
        EventMap events;
        std::set<uint32> playerGUIDs;

        uint32 GetData(uint32 data) const
        {
            if (data == DATA_IN_PROGRESS)
                return _duelInProgress;

            return playerGUIDs.find(data) != playerGUIDs.end();
        }

        void Reset()
        {
            _duelInProgress = false;
            _duelGUID = 0;
            me->RestoreFaction();
            CombatAI::Reset();

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_15);
        }

        void SpellHit(Unit* caster, const SpellInfo* pSpell)
        {
            if (!_duelInProgress && pSpell->Id == SPELL_DUEL)
            {
                playerGUIDs.insert(caster->GetGUIDLow());
                _duelGUID = caster->GetGUID();
                _duelInProgress = true;

                events.ScheduleEvent(EVENT_SPEAK, 3000);
                events.ScheduleEvent(EVENT_SPEAK+1, 7000);
                events.ScheduleEvent(EVENT_SPEAK+2, 8000);
                events.ScheduleEvent(EVENT_SPEAK+3, 9000);
                events.ScheduleEvent(EVENT_SPEAK+4, 10000);
                events.ScheduleEvent(EVENT_SPEAK+5, 11000);
            }
        }

       void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType, SpellSchoolMask)
        {
            if (attacker && _duelInProgress && attacker->IsControlledByPlayer())
            {
                if (attacker->GetCharmerOrOwnerOrOwnGUID() != _duelGUID)
                    damage = 0;
                else if (damage >= me->GetHealth())
                {
                    damage = 0;
                    events.ScheduleEvent(EVENT_DUEL_LOST, 2000);
                    events.ScheduleEvent(EVENT_DUEL_LOST+1, 6000);
                    _duelGUID = 0;
                    _duelInProgress = 0;

                    attacker->RemoveGameObject(SPELL_DUEL_FLAG, true);
                    attacker->AttackStop();
                    me->CombatStop(false);
                    me->RemoveAllAuras();
                    me->CastSpell(attacker, SPELL_DUEL_VICTORY, true);
                    me->RestoreFaction();
                }
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);
            switch (events.ExecuteEvent())
            {
                case EVENT_SPEAK:
                    Talk(SAY_DUEL, ObjectAccessor::GetPlayer(*me, _duelGUID));
                    me->StopMoving();
                    if (Player* player = ObjectAccessor::GetPlayer(*me, _duelGUID))
                        me->SetTarget(player->GetGUID());
                    break;
                case EVENT_SPEAK+1:
                    Talk(SAY_DUEL+1, ObjectAccessor::GetPlayer(*me, _duelGUID));
                    break;
                case EVENT_SPEAK+2:
                    Talk(SAY_DUEL+2, ObjectAccessor::GetPlayer(*me, _duelGUID));
                    break;
                case EVENT_SPEAK+3:
                    Talk(SAY_DUEL+3, ObjectAccessor::GetPlayer(*me, _duelGUID));
                    break;
                case EVENT_SPEAK+4:
                    Talk(SAY_DUEL+4, ObjectAccessor::GetPlayer(*me, _duelGUID));
                    break;
                case EVENT_SPEAK+5:
                    me->setFaction(FACTION_HOSTILE);
                    if (Player* player = ObjectAccessor::GetPlayer(*me, _duelGUID))
                        AttackStart(player);
                    return;
                case EVENT_DUEL_LOST:
                    me->CastSpell(me, 7267, true);
                    me->SetTarget(0);

                    // get spawn pos
                    float x, y, z, o; me->GetRespawnPosition(x, y, z, &o);

                    // not sure about coords @todo
                    if (Creature* newInitiate = me->SummonCreature(NPC_DK_INITIATE, 2400.821533f, -5752.854980f, 153.920349f, TEMPSUMMON_DEAD_DESPAWN))
                    {
                        newInitiate->GetMotionMaster()->MovePoint(0, x, y, z, true);
                        newInitiate->SetOrientation(y);
                        newInitiate->SetDisplayId(me->GetDisplayId()); // prevent changes
                        
                        // if the initiate was from the "moving" ones
                        if (me->GetDefaultMovementType() == 1)
                        {
                            newInitiate->SetDefaultMovementType(RANDOM_MOTION_TYPE);
                            newInitiate->GetMotionMaster()->MoveRandom(5); // might be at their actual spawn pos?
                        }
                    }
                    me->DespawnOrUnsummon(3000);
                    break;
                case EVENT_DUEL_LOST+1:
                    EnterEvadeMode();
                    return;
            }

            if (!events.Empty() || !UpdateVictim())
                return;

            if (_duelInProgress)
            {
                if (me->GetVictim()->GetGUID() == _duelGUID && me->GetVictim()->HealthBelowPct(10))
                {
                    me->GetVictim()->CastSpell(me->GetVictim(), 7267, true); // beg
                    me->GetVictim()->RemoveGameObject(SPELL_DUEL_FLAG, true);
                    EnterEvadeMode();
                    return;
                }
            }

            CombatAI::UpdateAI(diff);
        }
    };
};

enum GiftOfTheHarvester
{
    NPC_GHOUL                    = 28845,
    MAX_GHOULS                    = 5,

    SPELL_GHOUL_EMERGE            = 50142,
    SPELL_SUMMON_SCARLET_GHOST    = 52505,
    SPELL_GHOUL_SUBMERGE        = 26234,

    EVENT_GHOUL_RESTORE_STATE    = 1,
    EVENT_GHOUL_CHECK_COMBAT    = 2,
    EVENT_GHOUL_EMOTE            = 3,
    EVENT_GHOUL_MOVE_TO_PIT     = 4,

    SAY_GOTHIK_PIT              = 0
};

class spell_item_gift_of_the_harvester : public SpellScriptLoader
{
    public:
        spell_item_gift_of_the_harvester() : SpellScriptLoader("spell_item_gift_of_the_harvester") { }

        class spell_item_gift_of_the_harvester_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_item_gift_of_the_harvester_SpellScript);

            SpellCastResult CheckRequirement()
            {
                std::list<Creature*> ghouls;
                GetCaster()->GetAllMinionsByEntry(ghouls, NPC_GHOUL);
                if (ghouls.size() >= MAX_GHOULS)
                {
                    SetCustomCastResultMessage(SPELL_CUSTOM_ERROR_TOO_MANY_GHOULS);
                    return SPELL_FAILED_CUSTOM_ERROR;
                }

                return SPELL_CAST_OK;
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_item_gift_of_the_harvester_SpellScript::CheckRequirement);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_item_gift_of_the_harvester_SpellScript();
        }
};

class spell_q12698_the_gift_that_keeps_on_giving : public SpellScriptLoader
{
    public:
        spell_q12698_the_gift_that_keeps_on_giving() : SpellScriptLoader("spell_q12698_the_gift_that_keeps_on_giving") { }

        class spell_q12698_the_gift_that_keeps_on_giving_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_q12698_the_gift_that_keeps_on_giving_SpellScript);

            void HandleScriptEffect(SpellEffIndex /*effIndex*/)
            {
                if (GetOriginalCaster() && GetHitUnit())
                    GetOriginalCaster()->CastSpell(GetHitUnit(), urand(0, 1) ? GetEffectValue() : SPELL_SUMMON_SCARLET_GHOST, true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_q12698_the_gift_that_keeps_on_giving_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_q12698_the_gift_that_keeps_on_giving_SpellScript();
        }
};

class npc_scarlet_ghoul : public CreatureScript
{
public:
    npc_scarlet_ghoul() : CreatureScript("npc_scarlet_ghoul") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_scarlet_ghoulAI(creature);
    }

    struct npc_scarlet_ghoulAI : public ScriptedAI
    {
        npc_scarlet_ghoulAI(Creature* creature) : ScriptedAI(creature)
        {
        }

        EventMap events;
        uint64 gothikGUID;

        void InitializeAI()
        {
            gothikGUID = 0;
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            ScriptedAI::InitializeAI();
            me->SetReactState(REACT_PASSIVE);

            events.ScheduleEvent(EVENT_GHOUL_EMOTE, 1);
            events.ScheduleEvent(EVENT_GHOUL_RESTORE_STATE, 3500);
        }

        void OwnerAttackedBy(Unit* attacker)
        {
            if (!me->IsInCombat() && me->GetReactState() == REACT_DEFENSIVE)
                AttackStart(attacker);
        }

        void SetGUID(uint64 guid, int32)
        {
            gothikGUID = guid;
            events.ScheduleEvent(EVENT_GHOUL_MOVE_TO_PIT, 3000);
            me->GetMotionMaster()->Clear(false);
        }

        void MovementInform(uint32 type, uint32 point)
        {
            if (type == POINT_MOTION_TYPE && point == 1)
            {
                me->DespawnOrUnsummon(1500);
                me->CastSpell(me, SPELL_GHOUL_SUBMERGE, true);
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);
            switch (events.GetEvent())
            {
                case EVENT_GHOUL_MOVE_TO_PIT:
                    me->GetMotionMaster()->MovePoint(1, 2364.77f, -5776.14f, 151.36f);
                    if (Creature* gothik = ObjectAccessor::GetCreature(*me, gothikGUID))
                        gothik->AI()->DoAction(SAY_GOTHIK_PIT);
                    events.PopEvent();
                    break;
                case EVENT_GHOUL_EMOTE:
                    me->CastSpell(me, SPELL_GHOUL_EMERGE, true);
                    events.PopEvent();
                    break;
                case EVENT_GHOUL_RESTORE_STATE:
                    me->SetReactState(REACT_DEFENSIVE);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                    if (Player* owner = me->GetCharmerOrOwnerPlayerOrPlayerItself())
                        me->GetMotionMaster()->MoveFollow(owner, PET_FOLLOW_DIST, frand(0.0f, 2*M_PI));
                    events.PopEvent();
                    events.ScheduleEvent(EVENT_GHOUL_CHECK_COMBAT, 1000);
                    return;
                case EVENT_GHOUL_CHECK_COMBAT:
                    if (!me->IsInCombat())
                        if (Player* owner = me->GetCharmerOrOwnerPlayerOrPlayerItself())
                            if (owner->GetVictim())
                                AttackStart(owner->GetVictim());

                    events.RepeatEvent(1000);
                    return;
            }
            
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_dkc1_gothik : public CreatureScript
{
public:
    npc_dkc1_gothik() : CreatureScript("npc_dkc1_gothik") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_dkc1_gothikAI(creature);
    }

    struct npc_dkc1_gothikAI : public ScriptedAI
    {
        npc_dkc1_gothikAI(Creature* creature) : ScriptedAI(creature) { spoken = 0; }

        int32 spoken;

        void DoAction(int32 action)
        {
            if (action == SAY_GOTHIK_PIT && spoken <= 0)
            {
                spoken = 5000;
                Talk(SAY_GOTHIK_PIT);
            }
        }

        void MoveInLineOfSight(Unit* who)
        {
            ScriptedAI::MoveInLineOfSight(who);

            if (!who->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_NPC) && who->GetEntry() == NPC_GHOUL && me->IsWithinDistInMap(who, 10.0f))
                if (Unit* owner = who->GetOwner())
                    if (Player* player = owner->ToPlayer())
                    {
                        Creature* creature = who->ToCreature();
                        if (player->GetQuestStatus(12698) == QUEST_STATUS_INCOMPLETE)
                            creature->CastSpell(owner, 52517, true);

                        creature->AI()->SetGUID(me->GetGUID());
                        creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC|UNIT_FLAG_IMMUNE_TO_NPC);
                    }
        }

        void UpdateAI(uint32 diff)
        {
            if (spoken > 0)
                spoken -= diff;

            ScriptedAI::UpdateAI(diff);
        }
    };
};

class npc_scarlet_cannon : public CreatureScript
{
public:
    npc_scarlet_cannon() : CreatureScript("npc_scarlet_cannon") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_scarlet_cannonAI(creature);
    }

    struct npc_scarlet_cannonAI : public VehicleAI
    {
        npc_scarlet_cannonAI(Creature* creature) : VehicleAI(creature) { summonAttackers = 0; }

        uint32 summonAttackers;
        void PassengerBoarded(Unit* /*passenger*/, int8 /*seatId*/, bool apply)
        {
            summonAttackers = apply ? 8000 : 0;
        }

        void UpdateAI(uint32 diff)
        {
            VehicleAI::UpdateAI(diff);

            if (summonAttackers)
            {
                summonAttackers += diff;
                if (summonAttackers >= 15000)
                {
                    for (uint8 i = 0; i < 15; ++i)
                        if (Creature* summon = me->SummonCreature(28834 /*NPC_SCARLET_FLEET_DEFENDER*/, 2192.56f+irand(-10, 10), -6147.90f+irand(-10, 10), 5.2f, 4.7f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 45000))
                        {
                            summon->SetHomePosition(me->GetHomePosition());
                            summon->AI()->AttackStart(me);
                        }

                    summonAttackers = 1;
                }
            }
        }
    };
};


// Theirs
/*######
##Quest 12848
######*/

#define GCD_CAST    1

enum UnworthyInitiate
{
    SPELL_SOUL_PRISON_CHAIN_SELF    = 54612,
    SPELL_SOUL_PRISON_CHAIN         = 54613,
    SPELL_DK_INITIATE_VISUAL        = 51519,

    SPELL_ICY_TOUCH                 = 52372,
    SPELL_PLAGUE_STRIKE             = 52373,
    SPELL_BLOOD_STRIKE              = 52374,
    SPELL_DEATH_COIL                = 52375,

    SAY_EVENT_START                 = 0,
    SAY_EVENT_ATTACK                = 1,

    EVENT_ICY_TOUCH                 = 1,
    EVENT_PLAGUE_STRIKE             = 2,
    EVENT_BLOOD_STRIKE              = 3,
    EVENT_DEATH_COIL                = 4
};

enum UnworthyInitiatePhase
{
    PHASE_CHAINED,
    PHASE_TO_EQUIP,
    PHASE_EQUIPING,
    PHASE_TO_ATTACK,
    PHASE_ATTACKING,
};

uint32 acherus_soul_prison[12] =
{
    191577,
    191580,
    191581,
    191582,
    191583,
    191584,
    191585,
    191586,
    191587,
    191588,
    191589,
    191590
};

uint32 acherus_unworthy_initiate[5] =
{
    29519,
    29520,
    29565,
    29566,
    29567
};

class npc_unworthy_initiate : public CreatureScript
{
public:
    npc_unworthy_initiate() : CreatureScript("npc_unworthy_initiate") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_unworthy_initiateAI(creature);
    }

    struct npc_unworthy_initiateAI : public ScriptedAI
    {
        npc_unworthy_initiateAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetReactState(REACT_PASSIVE);
            if (!me->GetCurrentEquipmentId())
                me->SetCurrentEquipmentId(me->GetOriginalEquipmentId());
        }

        uint64 playerGUID;
        UnworthyInitiatePhase phase;
        uint32 wait_timer;
        float anchorX, anchorY;
        uint64 anchorGUID;

        EventMap events;

        void Reset()
        {
            anchorGUID = 0;
            phase = PHASE_CHAINED;
            events.Reset();
            me->setFaction(7);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
            me->SetUInt32Value(UNIT_FIELD_BYTES_1, 8);
            me->LoadEquipment(0, true);
        }

        void EnterCombat(Unit* /*who*/)
        {
            events.ScheduleEvent(EVENT_ICY_TOUCH, 1000, GCD_CAST);
            events.ScheduleEvent(EVENT_PLAGUE_STRIKE, 3000, GCD_CAST);
            events.ScheduleEvent(EVENT_BLOOD_STRIKE, 2000, GCD_CAST);
            events.ScheduleEvent(EVENT_DEATH_COIL, 5000, GCD_CAST);
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id == 1)
            {
                wait_timer = 5000;
                me->LoadEquipment(1);
                me->CastSpell(me, SPELL_DK_INITIATE_VISUAL, true);

                if (Player* starter = ObjectAccessor::GetPlayer(*me, playerGUID))
                    sCreatureTextMgr->SendChat(me, SAY_EVENT_ATTACK, NULL, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_NORMAL, 0, TEAM_NEUTRAL, false, starter);

                phase = PHASE_TO_ATTACK;
            }
        }

        void EventStart(Creature* anchor, Player* target)
        {
            wait_timer = 5000;
            phase = PHASE_TO_EQUIP;

            me->SetUInt32Value(UNIT_FIELD_BYTES_1, 0);
            me->RemoveAurasDueToSpell(SPELL_SOUL_PRISON_CHAIN_SELF);
            me->RemoveAurasDueToSpell(SPELL_SOUL_PRISON_CHAIN);

            float z;
            anchor->GetContactPoint(me, anchorX, anchorY, z, 1.0f);

            playerGUID = target->GetGUID();
            Talk(SAY_EVENT_START, target);
        }

        void UpdateAI(uint32 diff)
        {
            switch (phase)
            {
            case PHASE_CHAINED:
                if (!anchorGUID)
                {
                    if (Creature* anchor = me->FindNearestCreature(29521, 30))
                    {
                        anchor->AI()->SetGUID(me->GetGUID());
                        anchor->CastSpell(me, SPELL_SOUL_PRISON_CHAIN, true);
                        anchorGUID = anchor->GetGUID();
                    }

                    float dist = 99.0f;
                    GameObject* prison = NULL;

                    for (uint8 i = 0; i < 12; ++i)
                    {
                        if (GameObject* temp_prison = me->FindNearestGameObject(acherus_soul_prison[i], 30))
                        {
                            if (me->IsWithinDist(temp_prison, dist, false))
                            {
                                dist = me->GetDistance2d(temp_prison);
                                prison = temp_prison;
                            }
                        }
                    }

                    if (prison)
                        prison->ResetDoorOrButton();
                }
                break;
            case PHASE_TO_EQUIP:
                if (wait_timer)
                {
                    if (wait_timer > diff)
                        wait_timer -= diff;
                    else
                    {
                        me->GetMotionMaster()->MovePoint(1, anchorX, anchorY, me->GetPositionZ());
                        //sLog->outDebug(LOG_FILTER_TSCR, "npc_unworthy_initiateAI: move to %f %f %f", anchorX, anchorY, me->GetPositionZ());
                        phase = PHASE_EQUIPING;
                        wait_timer = 0;
                    }
                }
                break;
            case PHASE_TO_ATTACK:
                if (wait_timer)
                {
                    if (wait_timer > diff)
                        wait_timer -= diff;
                    else
                    {
                        me->setFaction(14);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                        phase = PHASE_ATTACKING;

                        if (Player* target = ObjectAccessor::GetPlayer(*me, playerGUID))
                            AttackStart(target);
                        wait_timer = 0;
                    }
                }
                break;
            case PHASE_ATTACKING:
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_ICY_TOUCH:
                        DoCastVictim(SPELL_ICY_TOUCH);
                        events.DelayEvents(1000, GCD_CAST);
                        events.ScheduleEvent(EVENT_ICY_TOUCH, 5000, GCD_CAST);
                        break;
                    case EVENT_PLAGUE_STRIKE:
                        DoCastVictim(SPELL_PLAGUE_STRIKE);
                        events.DelayEvents(1000, GCD_CAST);
                        events.ScheduleEvent(EVENT_PLAGUE_STRIKE, 5000, GCD_CAST);
                        break;
                    case EVENT_BLOOD_STRIKE:
                        DoCastVictim(SPELL_BLOOD_STRIKE);
                        events.DelayEvents(1000, GCD_CAST);
                        events.ScheduleEvent(EVENT_BLOOD_STRIKE, 5000, GCD_CAST);
                        break;
                    case EVENT_DEATH_COIL:
                        DoCastVictim(SPELL_DEATH_COIL);
                        events.DelayEvents(1000, GCD_CAST);
                        events.ScheduleEvent(EVENT_DEATH_COIL, 5000, GCD_CAST);
                        break;
                    }
                }

                DoMeleeAttackIfReady();
                break;
            default:
                break;
            }
        }
    };
};

class npc_unworthy_initiate_anchor : public CreatureScript
{
public:
    npc_unworthy_initiate_anchor() : CreatureScript("npc_unworthy_initiate_anchor") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_unworthy_initiate_anchorAI(creature);
    }

    struct npc_unworthy_initiate_anchorAI : public PassiveAI
    {
        npc_unworthy_initiate_anchorAI(Creature* creature) : PassiveAI(creature), prisonerGUID(0) {}

        uint64 prisonerGUID;

        void SetGUID(uint64 guid, int32 /*id*/)
        {
            if (!prisonerGUID)
                prisonerGUID = guid;
        }

        uint64 GetGUID(int32 /*id*/) const
        {
            return prisonerGUID;
        }
    };
};

class go_acherus_soul_prison : public GameObjectScript
{
public:
    go_acherus_soul_prison() : GameObjectScript("go_acherus_soul_prison") { }

    bool OnGossipHello(Player* player, GameObject* go)
    {
        if (Creature* anchor = go->FindNearestCreature(29521, 15))
            if (uint64 prisonerGUID = anchor->AI()->GetGUID())
                if (Creature* prisoner = ObjectAccessor::GetCreature(*player, prisonerGUID))
                    CAST_AI(npc_unworthy_initiate::npc_unworthy_initiateAI, prisoner->AI())->EventStart(anchor, player);

        return false;
    }

};

/*####
## npc_scarlet_miner_cart
####*/

enum Spells_SM
{
    SPELL_CART_CHECK       = 54173,
    SPELL_CART_DRAG        = 52465
};

class npc_scarlet_miner_cart : public CreatureScript
{
public:
    npc_scarlet_miner_cart() : CreatureScript("npc_scarlet_miner_cart") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_scarlet_miner_cartAI(creature);
    }

    struct npc_scarlet_miner_cartAI : public PassiveAI
    {
        npc_scarlet_miner_cartAI(Creature* creature) : PassiveAI(creature), minerGUID(0)
        {
            me->SetUInt32Value(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC|UNIT_FLAG_IMMUNE_TO_NPC);
            me->setFaction(35);
            me->SetDisplayId(me->GetCreatureTemplate()->Modelid1); // Modelid2 is a horse.
        }

        uint64 minerGUID;

        void SetGUID(uint64 guid, int32 /*id*/)
        {
            minerGUID = guid;
        }

        void DoAction(int32 /*param*/)
        {
            if (Creature* miner = ObjectAccessor::GetCreature(*me, minerGUID))
            {
                me->SetWalk(false);

                //Not 100% correct, but movement is smooth. Sometimes miner walks faster
                //than normal, this speed is fast enough to keep up at those times.
                me->SetSpeed(MOVE_RUN, 1.0f);

                me->GetMotionMaster()->MoveFollow(miner, 1.0f, 0);
                me->SetUInt32Value(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_IMMUNE_TO_PC|UNIT_FLAG_IMMUNE_TO_NPC);
                me->setFaction(35);
            }
        }

        void PassengerBoarded(Unit* who, int8 /*seatId*/, bool apply)
        {
            who->SetVisible(!apply);
            if (!apply)
                if (Creature* miner = ObjectAccessor::GetCreature(*me, minerGUID))
                    miner->DisappearAndDie();
        }
    };

};

/*####
## npc_scarlet_miner
####*/

enum Says_SM
{
    SAY_SCARLET_MINER_0         = 0,
    SAY_SCARLET_MINER_1         = 1
};

class npc_scarlet_miner : public CreatureScript
{
public:
    npc_scarlet_miner() : CreatureScript("npc_scarlet_miner") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_scarlet_minerAI(creature);
    }

    struct npc_scarlet_minerAI : public npc_escortAI
    {
        npc_scarlet_minerAI(Creature* creature) : npc_escortAI(creature)
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetSpeed(MOVE_RUN, 1.0f);
        }

        uint32 IntroTimer;
        uint32 IntroPhase;
        uint64 carGUID;

        void Reset()
        {
            carGUID = 0;
            IntroTimer = 0;
            IntroPhase = 0;
        }

        void InitWaypoint()
        {
            AddWaypoint(1, 2389.03f,     -5902.74f,     109.014f, 5000);
            AddWaypoint(2, 2341.812012f, -5900.484863f, 102.619743f);
            AddWaypoint(3, 2306.561279f, -5901.738281f, 91.792419f);
            AddWaypoint(4, 2300.098389f, -5912.618652f, 86.014885f);
            AddWaypoint(5, 2294.142090f, -5927.274414f, 75.316849f);
            AddWaypoint(6, 2286.984375f, -5944.955566f, 63.714966f);
            AddWaypoint(7, 2280.001709f, -5961.186035f, 54.228283f);
            AddWaypoint(8, 2259.389648f, -5974.197754f, 42.359348f);
            AddWaypoint(9, 2242.882812f, -5984.642578f, 32.827850f);
            AddWaypoint(10, 2217.265625f, -6028.959473f, 7.675705f);
            AddWaypoint(11, 2202.595947f, -6061.325684f, 5.882018f);
            AddWaypoint(12, 2188.974609f, -6080.866699f, 3.370027f);

            if (urand(0, 1))
            {
                AddWaypoint(13, 2176.483887f, -6110.407227f, 1.855181f);
                AddWaypoint(14, 2172.516602f, -6146.752441f, 1.074235f);
                AddWaypoint(15, 2138.918457f, -6158.920898f, 1.342926f);
                AddWaypoint(16, 2129.866699f, -6174.107910f, 4.380779f);
                AddWaypoint(17, 2117.709473f, -6193.830078f, 13.3542f, 10000);
            }
            else
            {
                AddWaypoint(13, 2184.190186f, -6166.447266f, 0.968877f);
                AddWaypoint(14, 2234.265625f, -6163.741211f, 0.916021f);
                AddWaypoint(15, 2268.071777f, -6158.750977f, 1.822252f);
                AddWaypoint(16, 2270.028320f, -6176.505859f, 6.340538f);
                AddWaypoint(17, 2271.739014f, -6195.401855f, 13.3542f, 10000);
            }
        }

        void InitCartQuest(Player* who)
        {
            carGUID = who->GetVehicleBase()->GetGUID();
            InitWaypoint();
            Start(false, false, who->GetGUID());
            SetDespawnAtFar(false);
        }

        void WaypointReached(uint32 waypointId)
        {
            switch (waypointId)
            {
                case 1:
                    if (Unit* car = ObjectAccessor::GetCreature(*me, carGUID))
                    {
                        me->SetFacingToObject(car);
                        // xinef: add some flags
                        car->SetUInt32Value(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_IMMUNE_TO_PC|UNIT_FLAG_IMMUNE_TO_NPC);
                        car->setFaction(35);
                    }
                    Talk(SAY_SCARLET_MINER_0);
                    SetRun(true);
                    IntroTimer = 4000;
                    IntroPhase = 1;
                    break;
                case 17:
                    if (Creature* car = ObjectAccessor::GetCreature(*me, carGUID))
                    {
                        car->SetPosition(car->GetPositionX(), car->GetPositionY(), me->GetPositionZ() + 1, car->GetOrientation());
                        car->StopMovingOnCurrentPos();
                        me->SetFacingToObject(car);
                        car->RemoveAura(SPELL_CART_DRAG);
                    }
                    Talk(SAY_SCARLET_MINER_1);
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (IntroPhase)
            {
                if (IntroTimer <= diff)
                {
                    if (IntroPhase == 1)
                    {
                        if (Creature* car = ObjectAccessor::GetCreature(*me, carGUID))
                            DoCast(car, SPELL_CART_DRAG);
                        IntroTimer = 800;
                        IntroPhase = 2;
                    }
                    else
                    {
                        if (Creature* car = ObjectAccessor::GetCreature(*me, carGUID))
                            car->AI()->DoAction(0);
                        IntroPhase = 0;
                    }
                } else IntroTimer-=diff;
            }
            npc_escortAI::UpdateAI(diff);
        }
    };

};

/*######
## go_inconspicuous_mine_car
######*/

enum Spells_Cart
{
    SPELL_CART_SUMM        = 52463
};

class go_inconspicuous_mine_car : public GameObjectScript
{
public:
    go_inconspicuous_mine_car() : GameObjectScript("go_inconspicuous_mine_car") { }

    bool OnGossipHello(Player* player, GameObject* /*go*/)
    {
        if (player->GetQuestStatus(12701) == QUEST_STATUS_INCOMPLETE)
        {
            // Hack Why Trinity Dont Support Custom Summon Location
            if (Creature* miner = player->SummonCreature(28841, 2383.869629f, -5900.312500f, 107.996086f, player->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN, 1))
            {
                player->CastSpell(player, SPELL_CART_SUMM, true);
                if (Creature* car = player->GetVehicleCreatureBase())
                {
                    if (car->GetEntry() == 28817)
                    {
                        car->AI()->SetGUID(miner->GetGUID());
                        CAST_AI(npc_scarlet_miner::npc_scarlet_minerAI, miner->AI())->InitCartQuest(player);
                    }
                }
            }
        }
        return true;
    }

};

enum Talk_Citizen
{
    TALK_CIT_AGGRESSIVE = 0,
    TALK_CIT_COWER      = 1,
    TALK_CIT_DEATH      = 2,
};

enum Talk_Peasant
{
    TALK_PEASANT = 0,
};

enum Npcs_Citizen
{
    NPC_PEASANT = 28557,
    NPC_CIT_1 = 28577,
    NPC_CIT_2 = 28576,
};

enum Events_Citizen
{
    EVENT_RANDOM_DESTINY = 1,
    EVENT_COWER_CAST,
};

enum Spells_Citizen
{
    SPELL_CORNERED_AND_ENRAGED = 52262,
    SPELL_CITIZEN_COWER = 52384,
};

class npc_citizen_of_havenshire : public CreatureScript
{
public:
    npc_citizen_of_havenshire() : CreatureScript("npc_citizen_of_havenshire") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_citizen_of_havenshireAI(creature);
    }

    struct npc_citizen_of_havenshireAI : public ScriptedAI
    {
        npc_citizen_of_havenshireAI(Creature* creature) : ScriptedAI(creature) { }
        EventMap events;
        int8 event_chance;

        void Reset()
        {
            events.Reset();
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->RemoveAura(SPELL_CORNERED_AND_ENRAGED);
        }

        void EnterCombat(Unit* /*who*/)
        {
            if (me->GetEntry() == NPC_PEASANT)
            {
                Talk(TALK_PEASANT);
                events.ScheduleEvent(EVENT_COWER_CAST, 1);
            }
            else
                events.ScheduleEvent(EVENT_RANDOM_DESTINY, 1);
        }

        void JustDied(Unit* /* killer */)
        {
            if (me->GetEntry() == NPC_PEASANT)
                Talk(TALK_PEASANT);
            else
                Talk(TALK_CIT_DEATH);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_COWER_CAST:
                        DoCast(SPELL_CITIZEN_COWER);
                            events.ScheduleEvent(EVENT_COWER_CAST, 4000);
                            me->SetAttackTime(BASE_ATTACK, 30000);
                            me->SetAttackTime(RANGED_ATTACK, 30000);
                            break;
                    case EVENT_RANDOM_DESTINY:
                        event_chance = RAND(1, 2);
                        if (me->GetEntry() == NPC_PEASANT)
                            event_chance = 0;

                        if (event_chance == 1)
                        {
                                Talk(TALK_CIT_AGGRESSIVE);
                                DoCast(SPELL_CORNERED_AND_ENRAGED);
                        }
                        else
                            events.ScheduleEvent(EVENT_COWER_CAST, 1000);
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };
};

enum MinerController
{
    NPC_ME = 6262,
    NPC_MINER = 6263,
    NPC_CART = 6264,

    EVENT_SPAWN_MINER = 1,
};

class npc_miner_controller : public CreatureScript
{
public:
    npc_miner_controller() : CreatureScript("npc_miner_controller") { }
     
    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_miner_controllerAI(creature);
    }

    struct npc_miner_controllerAI : public NullCreatureAI
    {
        npc_miner_controllerAI(Creature* creature) : NullCreatureAI(creature), summons(me) {  }

        EventMap events; 
        SummonList summons; 

        void Reset()
        {
            events.Reset();
            summons.DespawnAll(); 
            events.ScheduleEvent(EVENT_SPAWN_MINER, 1000);
        }

        void JustDied(Unit*)
        {
            summons.DespawnAll();
        }

        void JustSummoned(Creature* summon)
        {
            summons.Summon(summon);
            if (summon->GetEntry() == NPC_MINER)
            {
                summon->setActive(true);
            }
        }

        void SummonedCreatureDies(Creature* summon, Unit*)
        {
            summons.Despawn(summon); 
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff); 

            switch (events.ExecuteEvent())
            {
                case EVENT_SPAWN_MINER:
                    if (Creature* miner = me->SummonCreature(NPC_MINER, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN, 1))
                    {  
                        miner->SetSpeed(MOVE_RUN, 1.0f);
                        miner->setFaction(35);
                    }
                    events.ScheduleEvent(EVENT_SPAWN_MINER, urand(7000, 15000));
                    break;
            }
        }
    };
};


class npc_passive_scarlet_miner : public CreatureScript
{
public:
    npc_passive_scarlet_miner() : CreatureScript("npc_passive_scarlet_miner") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_passive_scarlet_minerAI(creature);
    }

    struct npc_passive_scarlet_minerAI : public npc_escortAI
    {
        npc_passive_scarlet_minerAI(Creature* creature) : npc_escortAI(creature)
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetSpeed(MOVE_RUN, 1.0f);
        }
        bool moved = false;
        uint8 path = 0;
        uint64 cartguid = 0;

        void Reset() { }

        void InitWaypoint()
        {
            uint8 point = 0;
            if (me->GetPositionX() > 2420.1f && me->GetPositionX() < 2430.1f)
            {
                path = 1;
                AddWaypoint(point++, 2423.325439f, -5914.020508f, 112.799484f);
                AddWaypoint(point++, 2417.883057f, -5918.415039f, 111.427643f);
                AddWaypoint(point++, 2412.828857f, -5921.774414f, 111.336716f);
                AddWaypoint(point++, 2403.866455f, -5922.794434f, 110.682823f);
                AddWaypoint(point++, 2398.096924f, -5919.744141f, 110.316841f);
                AddWaypoint(point++, 2392.370117f, -5915.718750f, 110.258278f);
                AddWaypoint(point++, 2386.643555f, -5911.693359f, 109.736008f);
                AddWaypoint(point++, 2380.916748f, -5907.667969f, 108.903923f);
                AddWaypoint(point++, 2378.053223f, -5905.655273f, 108.485199f);
                AddWaypoint(point++, 2370.302979f, -5901.958008f, 107.125031f);
                AddWaypoint(point++, 2359.840576f, -5901.118652f, 105.327423f);
                AddWaypoint(point++, 2352.845215f, -5900.864746f, 104.133652f);
                AddWaypoint(point++, 2345.857178f, -5900.461914f, 103.130341f);
                AddWaypoint(point++, 2338.864990f, -5900.131348f, 101.954178f);
                AddWaypoint(point++, 2331.871826f, -5899.820801f, 100.057861f);
                AddWaypoint(point++, 2321.381592f, -5899.366699f, 96.936905f);
                AddWaypoint(point++, 2314.385498f, -5899.191406f, 95.000725f);
                AddWaypoint(point++, 2307.227295f, -5900.429688f, 92.628052f);
                AddWaypoint(point++, 2300.239014f, -5905.069336f, 88.843605f);
                AddWaypoint(point++, 2296.557129f, -5910.470215f, 86.049248f);
                AddWaypoint(point++, 2294.248047f, -5918.514160f, 81.620567f);
                AddWaypoint(point++, 2292.555420f, -5925.306152f, 76.744644f);
                AddWaypoint(point++, 2290.867432f, -5932.099609f, 71.986504f);
                AddWaypoint(point++, 2288.066650f, -5942.212891f, 65.403206f);
                AddWaypoint(point++, 2285.375977f, -5948.666992f, 61.379826f);
                AddWaypoint(point++, 2280.151123f, -5957.753418f, 55.633339f);
                AddWaypoint(point++, 2274.613281f, -5963.833496f, 51.508507f);
                AddWaypoint(point++, 2269.920654f, -5967.414551f, 48.810078f);
                AddWaypoint(point++, 2260.438965f, -5971.924805f, 43.554932f);
                AddWaypoint(point++, 2250.969971f, -5976.460938f, 38.152775f);
                AddWaypoint(point++, 2243.353516f, -5982.323730f, 33.857330f);
                AddWaypoint(point++, 2237.189697f, -5990.803711f, 29.267315f);
                AddWaypoint(point++, 2233.673340f, -5996.856445f, 25.187603f);
                AddWaypoint(point++, 2228.274170f, -6005.861816f, 17.864260f);
                AddWaypoint(point++, 2222.822510f, -6014.835449f, 11.228409f);
                AddWaypoint(point++, 2219.232178f, -6020.843750f, 8.992152f);
                AddWaypoint(point++, 2214.065186f, -6029.983887f, 7.521602f);
                AddWaypoint(point++, 2207.299561f, -6042.240723f, 6.592027f);
                AddWaypoint(point++, 2203.896729f, -6049.572754f, 6.294215f);
                AddWaypoint(point++, 2201.396240f, -6056.110840f, 6.099938f);
                AddWaypoint(point++, 2195.409912f, -6071.763184f, 4.197136f);
                AddWaypoint(point++, 2190.401367f, -6084.858887f, 2.813966f);
                AddWaypoint(point++, 2185.626465f, -6097.901367f, 0.765109f);

                if (urand(0, 1))
                {
                    AddWaypoint(point++, 2176.483887f, -6110.407227f, 1.855181f);
                    AddWaypoint(point++, 2172.516602f, -6146.752441f, 1.074235f);
                    AddWaypoint(point++, 2138.918457f, -6158.920898f, 1.342926f);
                    AddWaypoint(point++, 2129.866699f, -6174.107910f, 4.380779f);
                    AddWaypoint(point++, 2117.709473f, -6193.830078f, 13.3542f);
                }
                else
                {
                    AddWaypoint(point++, 2184.190186f, -6166.447266f, 0.968877f);
                    AddWaypoint(point++, 2234.265625f, -6163.741211f, 0.916021f);
                    AddWaypoint(point++, 2268.071777f, -6158.750977f, 1.822252f);
                    AddWaypoint(point++, 2270.028320f, -6176.505859f, 6.340538f);
                    AddWaypoint(point++, 2271.739014f, -6195.401855f, 13.3542f);
                }
            } 
            else 
            {
                path = 2;
                AddWaypoint(point++, 2436.730957f, -5895.164551f, 104.700966f);
                AddWaypoint(point++, 2429.418701f, -5879.355957f, 104.693939f);
                AddWaypoint(point++, 2422.927734f, -5871.412598f, 105.293762f);
                AddWaypoint(point++, 2410.279785f, -5868.038574f, 104.608711f);
                AddWaypoint(point++, 2390.812012f, -5871.048340f, 104.668571f);
                AddWaypoint(point++, 2377.639404f, -5875.789551f, 104.774139f);
                AddWaypoint(point++, 2361.180176f, -5881.714844f, 104.889503f);
                AddWaypoint(point++, 2347.654297f, -5886.363770f, 104.893944f);
                AddWaypoint(point++, 2332.878906f, -5890.651855f, 101.368164f);
                AddWaypoint(point++, 2320.128906f, -5900.077637f, 96.398140f);
                AddWaypoint(point++, 2305.576660f, -5909.173828f, 88.894203f);
                AddWaypoint(point++, 2294.840576f, -5921.060059f, 79.923820f);
                AddWaypoint(point++, 2287.306152f, -5936.833008f, 68.186211f);
                AddWaypoint(point++, 2281.212402f, -5953.234863f, 57.905910f);
                AddWaypoint(point++, 2273.823242f, -5968.574219f, 50.057686f);
                AddWaypoint(point++, 2260.457764f, -5976.860352f, 42.240421f);
                AddWaypoint(point++, 2244.103271f, -5982.984375f, 33.667950f);
                AddWaypoint(point++, 2235.197998f, -5991.280762f, 28.713165f);
                AddWaypoint(point++, 2229.327881f, -6003.983887f, 19.510519f);
                AddWaypoint(point++, 2221.178223f, -6016.517090f, 10.316488f);
                AddWaypoint(point++, 2214.886719f, -6029.023438f, 7.629883f);
                AddWaypoint(point++, 2204.020752f, -6050.981934f, 6.254164f);
                AddWaypoint(point++, 2198.042236f, -6063.636230f, 5.673378f);
                AddWaypoint(point++, 2191.932129f, -6080.030762f, 3.150928f);
                AddWaypoint(point++, 2188.335938f, -6089.895508f, 2.164635f);
                AddWaypoint(point++, 2185.809082f, -6096.827148f, 0.921909f);
                if (urand(0, 1))
                {
                    AddWaypoint(point++, 2176.483887f, -6110.407227f, 1.855181f);
                    AddWaypoint(point++, 2172.516602f, -6146.752441f, 1.074235f);
                    AddWaypoint(point++, 2138.918457f, -6158.920898f, 1.342926f);
                    AddWaypoint(point++, 2129.866699f, -6174.107910f, 4.380779f);
                    AddWaypoint(point++, 2117.709473f, -6193.830078f, 13.3542f);
                }
                else
                {
                    AddWaypoint(point++, 2184.190186f, -6166.447266f, 0.968877f);
                    AddWaypoint(point++, 2234.265625f, -6163.741211f, 0.916021f);
                    AddWaypoint(point++, 2268.071777f, -6158.750977f, 1.822252f);
                    AddWaypoint(point++, 2270.028320f, -6176.505859f, 6.340538f);
                    AddWaypoint(point++, 2271.739014f, -6195.401855f, 13.3542f);
                }
            }
        }

        void InitMovement()
        {
            InitWaypoint();
            Start(false, false);
            SetDespawnAtFar(false);

            if (Creature* cart = me->SummonCreature(NPC_CART, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 165000))
            {
                cartguid = cart->GetGUID();
                cart->SetWalk(false); 
                cart->GetMotionMaster()->MoveFollow(me, PET_FOLLOW_DIST, M_PI/2);
                cart->SetUInt32Value(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
                cart->setFaction(35);
                me->SetSpeed(MOVE_RUN, 1.2f);
                DoCast(cart, SPELL_CART_DRAG); 
            }
        }

        void WaypointReached(uint32 waypointId)
        {
            switch (waypointId)
            { 
                case 31:
                    if (path == 1)
                    {
                        if (Creature* cart = ObjectAccessor::GetCreature(*me, cartguid))
                            cart->RemoveFromWorld();
                        me->RemoveFromWorld(); 
                    }
                    break;
                case 48: 
                    if (path == 2) 
                    {
                        if (Creature* cart = ObjectAccessor::GetCreature(*me, cartguid))
                            cart->RemoveFromWorld();
                        me->RemoveFromWorld();
                    }
                    break;
            default:
                break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (!moved)
            {
                InitMovement();
                moved = true;
            }

            npc_escortAI::UpdateAI(diff);
        }
    };

};

void AddSC_the_scarlet_enclave_c1()
{
    // Ours
    new npc_eye_of_acherus();
    new spell_q12641_death_comes_from_on_high_summon_ghouls();
    new npc_death_knight_initiate();
    new spell_item_gift_of_the_harvester();
    new spell_q12698_the_gift_that_keeps_on_giving();
    new npc_scarlet_ghoul();
    new npc_dkc1_gothik();
    new npc_scarlet_cannon();

    // Theirs
    new npc_unworthy_initiate();
    new npc_unworthy_initiate_anchor();
    new go_acherus_soul_prison();

    new npc_scarlet_miner();
    new npc_scarlet_miner_cart();
    new go_inconspicuous_mine_car();

    // PB
    new npc_runeforge_trigger();
    new npc_runebladed_sword();
    new npc_citizen_of_havenshire();

    new npc_miner_controller();
    new npc_passive_scarlet_miner();
}
