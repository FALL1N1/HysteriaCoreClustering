/*
 * Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#include "Common.h"
#include "DBCEnums.h"
#include "ObjectMgr.h"
//#include "ArenaTeamMgr.h"
//#include "GuildMgr.h"
#include "Proxy.h"
#include "WorldPacket.h"
#include "DatabaseEnv.h"
#include "AchievementMgr.h"
#include "Opcodes.h"
//#include "ArenaTeam.h"
//#include "CellImpl.h"
//#include "GameEventMgr.h"
//#include "GridNotifiersImpl.h"
//#include "Guild.h"
#include "Language.h"
#include "Player.h"
//#include "SpellMgr.h"
//#include "DisableMgr.h"
//#include "ScriptMgr.h"
//#include "MapManager.h"
//#include "Battleground.h"
//#include "BattlegroundAB.h"

namespace Trinity
{
    class AchievementChatBuilder
    {
        public:
            AchievementChatBuilder(Player const& player, ChatMsg msgtype, int32 textId, uint32 ach_id)
                : i_player(player), i_msgtype(msgtype), i_textId(textId), i_achievementId(ach_id) {}
            void operator()(WorldPacket& data, LocaleConstant loc_idx)
            {
                char const* text = sObjectMgr->GetTrinityString(i_textId, loc_idx);

                data << uint8(i_msgtype);
                data << uint32(LANG_UNIVERSAL);
                data << uint64(i_player.GetGUID());
                data << uint32(5);
                data << uint64(i_player.GetGUID());
                data << uint32(strlen(text)+1);
                data << text;
                data << uint8(0);
                data << uint32(i_achievementId);
            }

        private:
            Player const& i_player;
            ChatMsg i_msgtype;
            int32 i_textId;
            uint32 i_achievementId;
    };
}                                                           // namespace Trinity

bool AchievementCriteriaData::IsValid(AchievementCriteriaEntry const* criteria)
{
    if (dataType >= MAX_ACHIEVEMENT_CRITERIA_DATA_TYPE)
    {
        //sLog->outErrorDb("Table `achievement_criteria_data` for criteria (Entry: %u) has wrong data type (%u), ignored.", criteria->ID, dataType);
        sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "Table `achievement_criteria_data` for criteria (Entry: %u) has wrong data type (%u), ignored.", criteria->ID, dataType);
        return false;
    }

    switch (criteria->requiredType)
    {
        case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_BG:
        case ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING:
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:          // only hardcoded list
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA:
        case ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE:
        case ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL:
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE:
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2:
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET:
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2:
        case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED_ON_LOOT:
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT:
        case ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE:
        case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL:
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST:    // only Children's Week achievements
        case ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM:                // only Children's Week achievements
        case ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS:
            break;
        default:
            if (dataType != ACHIEVEMENT_CRITERIA_DATA_TYPE_SCRIPT)
            {
                sLog->outErrorDb("Table `achievement_criteria_data` has data for non-supported criteria type (Entry: %u Type: %u), ignored.", criteria->ID, criteria->requiredType);
                return false;
            }
            break;
    }

    switch (dataType)
    {
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_NONE:
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_VALUE:
        case ACHIEVEMENT_CRITERIA_DATA_INSTANCE_SCRIPT:
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_CREATURE:
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_CLASS_RACE:
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_LESS_HEALTH:
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_DEAD:
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AURA:
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_AURA:
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AREA:
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_LEVEL:
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_GENDER:
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_SCRIPT:
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_DIFFICULTY:
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_PLAYER_COUNT:
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_TEAM:
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_DRUNK:
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_HOLIDAY:
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_BG_LOSS_TEAM_SCORE:                                 // not check correctness node indexes
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_EQUIPED_ITEM:
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_ID:
            return true;
        default:
            sLog->outErrorDb("Table `achievement_criteria_data` (Entry: %u Type: %u) has data for non-supported data type (%u), ignored.", criteria->ID, criteria->requiredType, dataType);
            return false;
    }
}

bool AchievementCriteriaData::Meets(uint32 criteria_id, Player const* source, Unit const* target, uint32 miscvalue1 /*= 0*/) const
{
    return true;
}

bool AchievementCriteriaDataSet::Meets(Player const* source, Unit const* target, uint32 miscvalue /*= 0*/) const
{
    for (Storage::const_iterator itr = storage.begin(); itr != storage.end(); ++itr)
        if (!itr->Meets(criteria_id, source, target, miscvalue))
            return false;

    return true;
}

AchievementMgr::AchievementMgr(Player* player)
{
    m_player = player;
}

AchievementMgr::~AchievementMgr()
{
}

void AchievementMgr::Reset()
{
    for (CompletedAchievementMap::const_iterator iter = m_completedAchievements.begin(); iter != m_completedAchievements.end(); ++iter)
    {
        WorldPacket data(SMSG_ACHIEVEMENT_DELETED, 4);
        data << uint32(iter->first);
        m_player->SendDirectMessage(&data);
    }

    for (CriteriaProgressMap::const_iterator iter = m_criteriaProgress.begin(); iter != m_criteriaProgress.end(); ++iter)
    {
        WorldPacket data(SMSG_CRITERIA_DELETED, 4);
        data << uint32(iter->first);
        m_player->SendDirectMessage(&data);
    }

    m_completedAchievements.clear();
    m_criteriaProgress.clear();
    DeleteFromDB(m_player->GetGUIDLow());

    // re-fill data
    CheckAllAchievementCriteria();
}

void AchievementMgr::ResetAchievementCriteria(AchievementCriteriaTypes type, uint32 miscvalue1, uint32 miscvalue2, bool evenIfCriteriaComplete)
{}

void AchievementMgr::DeleteFromDB(uint32 lowguid)
{
    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    trans->PAppend("DELETE FROM character_achievement WHERE guid = %u", lowguid);
    trans->PAppend("DELETE FROM character_achievement_progress WHERE guid = %u", lowguid);
    CharacterDatabase.CommitTransaction(trans);
}

void AchievementMgr::SaveToDB(SQLTransaction& trans)
{
    if (!m_completedAchievements.empty())
    {
        bool need_execute = false;
        std::ostringstream ssdel;
        std::ostringstream ssins;
        for (CompletedAchievementMap::iterator iter = m_completedAchievements.begin(); iter != m_completedAchievements.end(); ++iter)
        {
            if (!iter->second.changed)
                continue;

            /// first new/changed record prefix
            if (!need_execute)
            {
                ssdel << "DELETE FROM character_achievement WHERE guid = " << GetPlayer()->GetGUIDLow() << " AND achievement IN (";
                ssins << "INSERT INTO character_achievement (guid, achievement, date) VALUES ";
                need_execute = true;
            }
            /// next new/changed record prefix
            else
            {
                ssdel << ", ";
                ssins << ", ";
            }

            // new/changed record data
            ssdel << iter->first;
            ssins << "("<<GetPlayer()->GetGUIDLow() << ", " << iter->first << ", " << uint64(iter->second.date) << ")";

            /// mark as saved in db
            iter->second.changed = false;
        }

        if (need_execute)
            ssdel << ")";

        if (need_execute)
        {
            trans->Append(ssdel.str().c_str());
            trans->Append(ssins.str().c_str());
        }
    }

    if (!m_criteriaProgress.empty())
    {
        /// prepare deleting and insert
        bool need_execute_del = false;
        bool need_execute_ins = false;
        std::ostringstream ssdel;
        std::ostringstream ssins;
        for (CriteriaProgressMap::iterator iter = m_criteriaProgress.begin(); iter != m_criteriaProgress.end(); ++iter)
        {
            if (!iter->second.changed)
                continue;

            // deleted data (including 0 progress state)
            {
                /// first new/changed record prefix (for any counter value)
                if (!need_execute_del)
                {
                    ssdel << "DELETE FROM character_achievement_progress WHERE guid = " << GetPlayer()->GetGUIDLow() << " AND criteria IN (";
                    need_execute_del = true;
                }
                /// next new/changed record prefix
                else
                    ssdel << ", ";

                // new/changed record data
                ssdel << iter->first;
            }

            // store data only for real progress
            if (iter->second.counter != 0)
            {
                /// first new/changed record prefix
                if (!need_execute_ins)
                {
                    ssins << "INSERT INTO character_achievement_progress (guid, criteria, counter, date) VALUES ";
                    need_execute_ins = true;
                }
                /// next new/changed record prefix
                else
                    ssins << ", ";

                // new/changed record data
                ssins << "(" << GetPlayer()->GetGUIDLow() << ", " << iter->first << ", " << iter->second.counter << ", " << iter->second.date << ")";
            }

            /// mark as updated in db
            iter->second.changed = false;
        }

        if (need_execute_del)                                // DELETE ... IN (.... _)_
            ssdel << ")";

        if (need_execute_del || need_execute_ins)
        {
            if (need_execute_del)
                trans->Append(ssdel.str().c_str());
            if (need_execute_ins)
                trans->Append(ssins.str().c_str());
        }
    }
}

void AchievementMgr::LoadFromDB(PreparedQueryResult achievementResult, PreparedQueryResult criteriaResult)
{
    if (achievementResult)
    {
        do
        {
            Field* fields = achievementResult->Fetch();
            uint32 achievementid = fields[0].GetUInt16();

            // don't must happen: cleanup at server startup in sAchievementMgr->LoadCompletedAchievements()
            AchievementEntry const* achievement = sAchievementStore.LookupEntry(achievementid);
            if (!achievement)
                continue;

            CompletedAchievementData& ca = m_completedAchievements[achievementid];
            ca.date = time_t(fields[1].GetUInt32());
            ca.changed = false;

            // title achievement rewards are retroactive
/*            if (AchievementReward const* reward = sAchievementMgr->GetAchievementReward(achievement))
                if (uint32 titleId = reward->titleId[GetPlayer()->GetTeam() == ALLIANCE ? 0 : 1])
                    if (CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(titleId))
                        if (!GetPlayer()->HasTitle(titleEntry))
                            GetPlayer()->SetTitle(titleEntry);
*/
        } while (achievementResult->NextRow());
    }

    if (criteriaResult)
    {
        do
        {
            Field* fields = criteriaResult->Fetch();
            uint32 id      = fields[0].GetUInt16();
            uint32 counter = fields[1].GetUInt32();
            time_t date    = time_t(fields[2].GetUInt32());

            AchievementCriteriaEntry const* criteria = sAchievementCriteriaStore.LookupEntry(id);
            if (!criteria)
            {
                // we will remove not existed criteria for all characters
                sLog->outError("Non-existing achievement criteria %u data removed from table `character_achievement_progress`.", id);
                CharacterDatabase.PExecute("DELETE FROM character_achievement_progress WHERE criteria = %u", id);
                continue;
            }

            if (criteria->timeLimit && time_t(date + criteria->timeLimit) < time(NULL))
                continue;

            CriteriaProgress& progress = m_criteriaProgress[id];
            progress.counter = counter;
            progress.date    = date;
            progress.changed = false;
        } while (criteriaResult->NextRow());
    }
}

void AchievementMgr::SendAchievementEarned(AchievementEntry const* achievement) const
{
/*    if (GetPlayer()->GetSession()->PlayerLoading())
        return;

    // Don't send for achievements with ACHIEVEMENT_FLAG_TRACKING
    if (achievement->flags & ACHIEVEMENT_FLAG_HIDDEN)
        return;

    #ifdef TRINITY_DEBUG
        sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "AchievementMgr::SendAchievementEarned(%u)", achievement->ID);
    #endif

    if (Guild* guild = sGuildMgr->GetGuildById(GetPlayer()->GetGuildId()))
    {
        Trinity::AchievementChatBuilder say_builder(*GetPlayer(), CHAT_MSG_GUILD_ACHIEVEMENT, LANG_ACHIEVEMENT_EARNED, achievement->ID);
        Trinity::LocalizedPacketDo<Trinity::AchievementChatBuilder> say_do(say_builder);
        guild->BroadcastWorker(say_do, GetPlayer());
    }

    if (achievement->flags & (ACHIEVEMENT_FLAG_REALM_FIRST_KILL|ACHIEVEMENT_FLAG_REALM_FIRST_REACH))
    {
        // broadcast realm first reached
        WorldPacket data(SMSG_SERVER_FIRST_ACHIEVEMENT, strlen(GetPlayer()->GetName())+1+8+4+4);
        data << GetPlayer()->GetName();
        data << uint64(GetPlayer()->GetGUID());
        data << uint32(achievement->ID);
        data << uint32(0);                                  // 1=link supplied string as player name, 0=display plain string
        sWorld->SendGlobalMessage(&data);
    }
    // if player is in world he can tell his friends about new achievement
    else if (GetPlayer()->IsInWorld())
    {
        CellPair p = Trinity::ComputeCellPair(GetPlayer()->GetPositionX(), GetPlayer()->GetPositionY());

        Cell cell(p);
        cell.data.Part.reserved = ALL_DISTRICT;
        cell.SetNoCreate();

        Trinity::AchievementChatBuilder say_builder(*GetPlayer(), CHAT_MSG_ACHIEVEMENT, LANG_ACHIEVEMENT_EARNED, achievement->ID);
        Trinity::LocalizedPacketDo<Trinity::AchievementChatBuilder> say_do(say_builder);
        Trinity::PlayerDistWorker<Trinity::LocalizedPacketDo<Trinity::AchievementChatBuilder> > say_worker(GetPlayer(), sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_SAY), say_do);
        TypeContainerVisitor<Trinity::PlayerDistWorker<Trinity::LocalizedPacketDo<Trinity::AchievementChatBuilder> >, WorldTypeMapContainer > message(say_worker);
        cell.Visit(p, message, *GetPlayer()->GetMap(), *GetPlayer(), sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_SAY));
    }

    WorldPacket data(SMSG_ACHIEVEMENT_EARNED, 8+4+8);
    data.append(GetPlayer()->GetPackGUID());
    data << uint32(achievement->ID);
    data << uint32(secsToTimeBitFields(time(NULL)));
    data << uint32(0);
    GetPlayer()->SendMessageToSetInRange(&data, sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_SAY), true);*/
}

void AchievementMgr::SendCriteriaUpdate(AchievementCriteriaEntry const* entry, CriteriaProgress const* progress, uint32 timeElapsed, bool timedCompleted) const
{
    WorldPacket data(SMSG_CRITERIA_UPDATE, 8 + 4 + 8);
    data << uint32(entry->ID);

    // the counter is packed like a packed Guid
    data.appendPackGUID(progress->counter);

    data.append(GetPlayer()->GetPackGUID());
    if (!entry->timeLimit)
        data << uint32(0);
    else
        data << uint32(/* xinef: timedCompleted ? 0 : 1*/ 0); // this are some flags, 1 is for keeping the counter at 0 in client
    data.AppendPackedTime(progress->date);
    data << uint32(timeElapsed);    // time elapsed in seconds
    data << uint32(0);              // unk
    GetPlayer()->SendDirectMessage(&data);
}

/**
 * called at player login. The player might have fulfilled some achievements when the achievement system wasn't working yet
 */
void AchievementMgr::CheckAllAchievementCriteria()
{
    // suppress sending packets
}

static const uint32 achievIdForDungeon[][4] =
{
    // ach_cr_id, is_dungeon, is_raid, is_heroic_dungeon
    { 321,       true,      true,   true  },
    { 916,       false,     true,   false },
    { 917,       false,     true,   false },
    { 918,       true,      false,  false },
    { 2219,      false,     false,  true  },
    { 0,         false,     false,  false }
};

static const uint32 achievIdByClass[MAX_CLASSES] = { 0, 459, 465 , 462, 458, 464, 461, 467, 460, 463, 0, 466 };
static const uint32 achievIdByRace[MAX_RACES]    = { 0, 1408, 1410, 1407, 1409, 1413, 1411, 1404, 1412, 0, 1405, 1406 };

void AchievementMgr::CompletedCriteriaFor(AchievementEntry const* achievement)
{
    // counter can never complete
    if (achievement->flags & ACHIEVEMENT_FLAG_COUNTER)
        return;

    // already completed and stored
    if (HasAchieved(achievement))
        return;

    if (IsCompletedAchievement(achievement))
        CompletedAchievement(achievement);
}

bool AchievementMgr::IsCompletedCriteria(AchievementCriteriaEntry const* achievementCriteria, AchievementEntry const* achievement)
{
    // counter can never complete
    if (achievement->flags & ACHIEVEMENT_FLAG_COUNTER)
        return false;

    if (achievement->flags & (ACHIEVEMENT_FLAG_REALM_FIRST_REACH | ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
    {
        // someone on this realm has already completed that achievement
        if (sAchievementMgr->IsRealmCompleted(achievement))
            return false;
    }

    CriteriaProgress const* progress = GetCriteriaProgress(achievementCriteria);
    if (!progress)
        return false;

    switch (achievementCriteria->requiredType)
    {
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_BG:
            return progress->counter >= achievementCriteria->win_bg.winCount;
        case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
            return progress->counter >= achievementCriteria->kill_creature.creatureCount;
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL:
            return progress->counter >= achievementCriteria->reach_level.level;
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL:
            return progress->counter >= achievementCriteria->reach_skill_level.skillLevel;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT:
            return progress->counter >= 1;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST_COUNT:
            return progress->counter >= achievementCriteria->complete_quest_count.totalQuestCount;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST_DAILY:
            return progress->counter >= achievementCriteria->complete_daily_quest_daily.numberOfDays;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE:
            return progress->counter >= achievementCriteria->complete_quests_in_zone.questCount;
        case ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE:
        case ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE:
            return progress->counter >= achievementCriteria->healing_done.count;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST:
            return progress->counter >= achievementCriteria->complete_daily_quest.questCount;
        case ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING:
            return progress->counter >= achievementCriteria->fall_without_dying.fallHeight;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:
            return progress->counter >= 1;
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET:
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2:
            return progress->counter >= achievementCriteria->be_spell_target.spellCount;
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL:
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2:
            return progress->counter >= achievementCriteria->cast_spell.castCount;
        case ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE:
            return progress->counter >= achievementCriteria->bg_objective.completeCount;
        case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL_AT_AREA:
            return progress->counter >= achievementCriteria->honorable_kill_at_area.killCount;
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SPELL:
            return progress->counter >= 1;
        case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL:
        case ACHIEVEMENT_CRITERIA_TYPE_EARN_HONORABLE_KILL:
            return progress->counter >= achievementCriteria->honorable_kill.killCount;
        case ACHIEVEMENT_CRITERIA_TYPE_OWN_ITEM:
            return progress->counter >= achievementCriteria->own_item.itemCount;
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA:
            return progress->counter >= achievementCriteria->win_rated_arena.count;
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_PERSONAL_RATING:
            return progress->counter >= achievementCriteria->highest_personal_rating.PersonalRating;
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LEVEL:
            return progress->counter >= (achievementCriteria->learn_skill_level.skillLevel * 75);
        case ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM:
            return progress->counter >= achievementCriteria->use_item.itemCount;
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM:
            return progress->counter >= achievementCriteria->loot_item.itemCount;
        case ACHIEVEMENT_CRITERIA_TYPE_EXPLORE_AREA:
            return progress->counter >= 1;
        case ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT:
            return progress->counter >= achievementCriteria->buy_bank_slot.numberOfSlots;
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REPUTATION:
            return progress->counter >= achievementCriteria->gain_reputation.reputationAmount;
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_EXALTED_REPUTATION:
            return progress->counter >= achievementCriteria->gain_exalted_reputation.numberOfExaltedFactions;
        case ACHIEVEMENT_CRITERIA_TYPE_VISIT_BARBER_SHOP:
            return progress->counter >= achievementCriteria->visit_barber.numberOfVisits;
        case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM:
            return progress->counter >= achievementCriteria->equip_epic_item.count;
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED_ON_LOOT:
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT:
            return progress->counter >= achievementCriteria->roll_greed_on_loot.count;
        case ACHIEVEMENT_CRITERIA_TYPE_HK_CLASS:
            return progress->counter >= achievementCriteria->hk_class.count;
        case ACHIEVEMENT_CRITERIA_TYPE_HK_RACE:
            return progress->counter >= achievementCriteria->hk_race.count;
        case ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE:
            return progress->counter >= achievementCriteria->do_emote.count;
        case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_ITEM:
            return progress->counter >= achievementCriteria->equip_item.count;
        case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD:
            return progress->counter >= achievementCriteria->quest_reward_money.goldInCopper;
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY:
            return progress->counter >= achievementCriteria->loot_money.goldInCopper;
        case ACHIEVEMENT_CRITERIA_TYPE_USE_GAMEOBJECT:
            return progress->counter >= achievementCriteria->use_gameobject.useCount;
        case ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL:
            return progress->counter >= achievementCriteria->special_pvp_kill.killCount;
        case ACHIEVEMENT_CRITERIA_TYPE_FISH_IN_GAMEOBJECT:
            return progress->counter >= achievementCriteria->fish_in_gameobject.lootCount;
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS:
            return progress->counter >= achievementCriteria->learn_skillline_spell.spellCount;
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL:
            return progress->counter >= achievementCriteria->win_duel.duelCount;
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE:
            return progress->counter >= achievementCriteria->loot_type.lootTypeCount;
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LINE:
            return progress->counter >= achievementCriteria->learn_skill_line.spellCount;
        case ACHIEVEMENT_CRITERIA_TYPE_EARN_ACHIEVEMENT_POINTS:
            return progress->counter >= 9000;
        case ACHIEVEMENT_CRITERIA_TYPE_USE_LFD_TO_GROUP_WITH_PLAYERS:
            return progress->counter >= achievementCriteria->use_lfg.dungeonsComplete;
        case ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS:
            return progress->counter >= achievementCriteria->get_killing_blow.killCount;
        // handle all statistic-only criteria here
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_BATTLEGROUND:
        case ACHIEVEMENT_CRITERIA_TYPE_DEATH_AT_MAP:
        case ACHIEVEMENT_CRITERIA_TYPE_DEATH:
        case ACHIEVEMENT_CRITERIA_TYPE_DEATH_IN_DUNGEON:
        case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_CREATURE:
        case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_PLAYER:
        case ACHIEVEMENT_CRITERIA_TYPE_DEATHS_FROM:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_TEAM_RATING:
        case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_VENDORS:
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_TALENTS:
        case ACHIEVEMENT_CRITERIA_TYPE_NUMBER_OF_TALENT_RESETS:
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_AT_BARBER:
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_MAIL:
        case ACHIEVEMENT_CRITERIA_TYPE_LOSE_DUEL:
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_EARNED_BY_AUCTIONS:
        case ACHIEVEMENT_CRITERIA_TYPE_CREATE_AUCTION:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_BID:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_SOLD:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_GOLD_VALUE_OWNED:
        case ACHIEVEMENT_CRITERIA_TYPE_WON_AUCTIONS:
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REVERED_REPUTATION:
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_HONORED_REPUTATION:
        case ACHIEVEMENT_CRITERIA_TYPE_KNOWN_FACTIONS:
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_EPIC_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_RECEIVE_EPIC_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED:
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEALTH:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_SPELLPOWER:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_ARMOR:
        case ACHIEVEMENT_CRITERIA_TYPE_QUEST_ABANDONED:
        case ACHIEVEMENT_CRITERIA_TYPE_FLIGHT_PATHS_TAKEN:
        case ACHIEVEMENT_CRITERIA_TYPE_ACCEPTED_SUMMONINGS:
        default:
            break;
    }
    return false;
}

bool AchievementMgr::IsCompletedAchievement(AchievementEntry const* entry)
{
    return false;
}

CriteriaProgress* AchievementMgr::GetCriteriaProgress(AchievementCriteriaEntry const* entry)
{
    CriteriaProgressMap::iterator iter = m_criteriaProgress.find(entry->ID);

    if (iter == m_criteriaProgress.end())
        return NULL;

    return &(iter->second);
}

void AchievementMgr::SetCriteriaProgress(AchievementCriteriaEntry const* entry, uint32 changeValue, uint8 ptype)
{
    // Don't allow to cheat - doing timed achievements without timer active
    TimedAchievementMap::iterator timedIter = m_timedAchievements.find(entry->ID);
    if (entry->timeLimit && timedIter == m_timedAchievements.end())
        return;

    sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "AchievementMgr::SetCriteriaProgress(%u, %u) for (GUID:%u)", entry->ID, changeValue, m_player->GetGUIDLow());

    CriteriaProgress* progress = GetCriteriaProgress(entry);
    if (!progress)
    {
        // not create record for 0 counter but allow it for timed achievements
        // we will need to send 0 progress to client to start the timer
        if (changeValue == 0 && !entry->timeLimit)
            return;

        progress = &m_criteriaProgress[entry->ID];
        progress->counter = changeValue;
    }
    else
    {
        uint32 newValue = 0;
        switch (ptype)
        {
            case PROGRESS_SET:
                newValue = changeValue;
                break;
            case PROGRESS_ACCUMULATE:
            {
                // avoid overflow
                uint32 max_value = std::numeric_limits<uint32>::max();
                newValue = max_value - progress->counter > changeValue ? progress->counter + changeValue : max_value;
                break;
            }
            case PROGRESS_HIGHEST:
                newValue = progress->counter < changeValue ? changeValue : progress->counter;
                break;
        }

        // not update (not mark as changed) if counter will have same value
        if (progress->counter == newValue && !entry->timeLimit)
            return;

        progress->counter = newValue;
    }

    progress->changed = true;
    progress->date = time(NULL); // set the date to the latest update.

    uint32 timeElapsed = 0;
    bool timedCompleted = false;

    if (entry->timeLimit)
    {
        // has to exist else we wouldn't be here
        timedCompleted = IsCompletedCriteria(entry, sAchievementStore.LookupEntry(entry->referredAchievement));
        // Client expects this in packet
        timeElapsed = entry->timeLimit - (timedIter->second/IN_MILLISECONDS);

        // Remove the timer, we wont need it anymore
        if (timedCompleted)
            m_timedAchievements.erase(timedIter);
    }

    //SendCriteriaUpdate(entry, progress, timeElapsed, timedCompleted);
}

void AchievementMgr::RemoveCriteriaProgress(const AchievementCriteriaEntry *entry)
{
    CriteriaProgressMap::iterator criteriaProgress = m_criteriaProgress.find(entry->ID);
    if (criteriaProgress == m_criteriaProgress.end())
        return;

    WorldPacket data(SMSG_CRITERIA_DELETED, 4);
    data << uint32(entry->ID);
    m_player->SendDirectMessage(&data);

    m_criteriaProgress.erase(criteriaProgress);
}

void AchievementMgr::UpdateTimedAchievements(uint32 timeDiff)
{
    if (!m_timedAchievements.empty())
    {
        for (TimedAchievementMap::iterator itr = m_timedAchievements.begin(); itr != m_timedAchievements.end();)
        {
            // Time is up, remove timer and reset progress
            if (itr->second <= timeDiff)
            {
                AchievementCriteriaEntry const *entry = sAchievementCriteriaStore.LookupEntry(itr->first);
                RemoveCriteriaProgress(entry);
                m_timedAchievements.erase(itr++);
            }
            else
            {
                itr->second -= timeDiff;
                ++itr;
            }
        }
    }
}

void AchievementMgr::StartTimedAchievement(AchievementCriteriaTimedTypes type, uint32 entry, uint32 timeLost /*= 0*/)
{}

void AchievementMgr::RemoveTimedAchievement(AchievementCriteriaTimedTypes type, uint32 entry)
{
    AchievementCriteriaEntryList const& achievementCriteriaList = sAchievementMgr->GetTimedAchievementCriteriaByType(type);
    for (AchievementCriteriaEntryList::const_iterator i = achievementCriteriaList.begin(); i!=achievementCriteriaList.end(); ++i)
    {
        if ((*i)->timerStartEvent != entry)
            continue;

        TimedAchievementMap::iterator timedIter = m_timedAchievements.find((*i)->ID);
        // We don't have timer for this achievement
        if (timedIter == m_timedAchievements.end())
            continue;

        // remove progress
        RemoveCriteriaProgress(*i);

        // Remove the timer
        m_timedAchievements.erase(timedIter);
    }
}

void AchievementMgr::CompletedAchievement(AchievementEntry const* achievement)
{
    sLog->outDetail("AchievementMgr::CompletedAchievement(%u)", achievement->ID);

    // disable for gamemasters with GM-mode enabled
    //if (m_player->isGameMaster())
    //    return;

    if (achievement->flags & ACHIEVEMENT_FLAG_COUNTER || HasAchieved(achievement))
        return;

    //SendAchievementEarned(achievement);
    CompletedAchievementData& ca =  m_completedAchievements[achievement->ID];
    ca.date = time(NULL);
    ca.changed = true;
}

void AchievementMgr::SendAllAchievementData() const
{
    WorldPacket data(SMSG_ALL_ACHIEVEMENT_DATA, m_completedAchievements.size()*8+4+m_criteriaProgress.size()*38+4);
    BuildAllDataPacket(&data, false);
    GetPlayer()->GetSession()->SendPacket(&data);
}

void AchievementMgr::SendRespondInspectAchievements(Player* player) const
{
    WorldPacket data(SMSG_RESPOND_INSPECT_ACHIEVEMENTS, 9+m_completedAchievements.size()*8+4+m_criteriaProgress.size()*38+4);
    data.append(GetPlayer()->GetPackGUID());    
    BuildAllDataPacket(&data, false);
    player->GetSession()->SendPacket(&data);
}

/**
 * used by SMSG_RESPOND_INSPECT_ACHIEVEMENT and SMSG_ALL_ACHIEVEMENT_DATA
 */
void AchievementMgr::BuildAllDataPacket(WorldPacket* data, bool inspect) const
{
    if (!m_completedAchievements.empty())
    {
        AchievementEntry const* achievement = NULL;
        for (CompletedAchievementMap::const_iterator iter = m_completedAchievements.begin(); iter != m_completedAchievements.end(); ++iter)
        {
            // Skip hidden achievements
            achievement = sAchievementStore.LookupEntry(iter->first);
            if (!achievement || achievement->flags & ACHIEVEMENT_FLAG_HIDDEN)
                continue;

            *data << uint32(iter->first);
            data->AppendPackedTime(iter->second.date);
        }
    }
    *data << int32(-1);

    if (!inspect && !m_criteriaProgress.empty())
        for (CriteriaProgressMap::const_iterator iter = m_criteriaProgress.begin(); iter != m_criteriaProgress.end(); ++iter)
        {
            *data << uint32(iter->first);
            data->appendPackGUID(iter->second.counter);
            data->append(GetPlayer()->GetPackGUID());
            *data << uint32(0);
            data->AppendPackedTime(iter->second.date);
            *data << uint32(0);
            *data << uint32(0);
        }

    *data << int32(-1);
}

bool AchievementMgr::HasAchieved(AchievementEntry const* achievement) const
{
    return m_completedAchievements.find(achievement->ID) != m_completedAchievements.end();
}

//==========================================================
void AchievementGlobalMgr::LoadAchievementCriteriaList()
{
    uint32 oldMSTime = getMSTime();

    if (sAchievementCriteriaStore.GetNumRows() == 0)
    {
        sLog->outErrorDb(">> Loaded 0 achievement criteria.");
        sLog->outString();
        return;
    }

    for (uint32 entryId = 0; entryId < sAchievementCriteriaStore.GetNumRows(); ++entryId)
    {

        AchievementCriteriaEntry const* criteria = sAchievementCriteriaStore.LookupEntry(entryId);
        if (!criteria)
            continue;

        m_AchievementCriteriasByType[criteria->requiredType].push_back(criteria);
        m_AchievementCriteriaListByAchievement[criteria->referredAchievement].push_back(criteria);

        if (criteria->timeLimit)
            m_AchievementCriteriasByTimedType[criteria->timedType].push_back(criteria);
    }

    sLog->outString(">> Loaded %lu achievement criteria in %u ms", (unsigned long)m_AchievementCriteriasByType->size(), GetMSTimeDiffToNow(oldMSTime));
    sLog->outString();
}

void AchievementGlobalMgr::LoadAchievementReferenceList()
{
    uint32 oldMSTime = getMSTime();

    if (sAchievementStore.GetNumRows() == 0)
    {
        sLog->outString(">> Loaded 0 achievement references.");
        sLog->outString();
        return;
    }

    uint32 count = 0;

    for (uint32 entryId = 0; entryId < sAchievementStore.GetNumRows(); ++entryId)
    {

        AchievementEntry const* achievement = sAchievementStore.LookupEntry(entryId);
        if (!achievement || !achievement->refAchievement)
            continue;

        m_AchievementListByReferencedId[achievement->refAchievement].push_back(achievement);
        ++count;
    }

    sLog->outString(">> Loaded %u achievement references in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    sLog->outString();
}

void AchievementGlobalMgr::LoadAchievementCriteriaData()
{
    uint32 oldMSTime = getMSTime();

    m_criteriaDataMap.clear();                              // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT criteria_id, type, value1, value2, ScriptName FROM achievement_criteria_data");

    if (!result)
    {
        sLog->outString(">> Loaded 0 additional achievement criteria data. DB table `achievement_criteria_data` is empty.");
        sLog->outString();
        return;
    }

    uint32 count = 0;

    do
    {
        Field *fields = result->Fetch();
        uint32 criteria_id = fields[0].GetUInt32();

        AchievementCriteriaEntry const* criteria = sAchievementCriteriaStore.LookupEntry(criteria_id);

        if (!criteria)
        {
            sLog->outErrorDb("Table `achievement_criteria_data` has data for non-existing criteria (Entry: %u), ignore.", criteria_id);
            continue;
        }

        uint32 dataType = fields[1].GetUInt32();
        const char* scriptName = fields[4].GetCString();
        uint32 scriptId = 0;
        if (strcmp(scriptName, "")) // not empty
        {
            if (dataType != ACHIEVEMENT_CRITERIA_DATA_TYPE_SCRIPT)
                sLog->outErrorDb("Table `achievement_criteria_data` has ScriptName set for non-scripted data type (Entry: %u, type %u), useless data.", criteria_id, dataType);
        }

        AchievementCriteriaData data(dataType, fields[2].GetUInt32(), fields[3].GetUInt32(), scriptId);

        if (!data.IsValid(criteria))
            continue;

        // this will allocate empty data set storage
        AchievementCriteriaDataSet& dataSet = m_criteriaDataMap[criteria_id];
        dataSet.SetCriteriaId(criteria_id);

        // add real data only for not NONE data types
        if (data.dataType != ACHIEVEMENT_CRITERIA_DATA_TYPE_NONE)
            dataSet.Add(data);

        // counting data by and data types
        ++count;
    }
    while (result->NextRow());

    // post loading checks
    for (uint32 entryId = 0; entryId < sAchievementCriteriaStore.GetNumRows(); ++entryId)
    {
        AchievementCriteriaEntry const* criteria = sAchievementCriteriaStore.LookupEntry(entryId);
        if (!criteria)
            continue;

        switch (criteria->requiredType)
        {
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_BG:          // any cases
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
                break;                                      // any cases
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:
            {
                AchievementEntry const* achievement = sAchievementStore.LookupEntry(criteria->referredAchievement);
                if (!achievement)
                    continue;

                // exist many achievements with this criteria, use at this moment hardcoded check to skil simple case
                if (achievement->ID == 1282)
                    break;

                continue;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING:
                break;                                      // any cases
            case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET: // any cases
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL:      // any cases
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA: // need skip generic cases
                if (criteria->additionalRequirements[0].additionalRequirement_type != ACHIEVEMENT_CRITERIA_CONDITION_NO_LOSE)
                    continue;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM: // any cases
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED_ON_LOOT:
                break;                                      // any cases
            case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT:
                break;                                      // any cases
            case ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE:        // need skip generic cases
                if (criteria->do_emote.count == 0)
                    continue;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2:
                break;                                      // any cases
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL:        // skip statistics
                if (criteria->win_duel.duelCount == 0)
                    continue;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2:     // any cases
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE:       // need skip generic cases
                if (criteria->loot_type.lootTypeCount != 1)
                    continue;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE:
                break;                                      // any cases
            case ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL:
                break;                                      // any cases
            case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL:  // any cases
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST:
            case ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM:        // only Children's Week achievements
            {
                AchievementEntry const* achievement = sAchievementStore.LookupEntry(criteria->referredAchievement);
                if (!achievement)
                    continue;
                if (achievement->categoryId != CATEGORY_CHILDRENS_WEEK)
                    continue;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS:
                break;
            default:                                        // type not use DB data, ignore
                continue;
        }

        if (!GetCriteriaDataSet(criteria) /*&& !sDisableMgr->IsDisabledFor(DISABLE_TYPE_ACHIEVEMENT_CRITERIA, entryId, NULL)*/)
            sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "Table `achievement_criteria_data` does not have expected data for criteria (Entry: %u Type: %u) for achievement %u.", criteria->ID, criteria->requiredType, criteria->referredAchievement);
            //sLog->outErrorDb("Table `achievement_criteria_data` does not have expected data for criteria (Entry: %u Type: %u) for achievement %u.", criteria->ID, criteria->requiredType, criteria->referredAchievement);
    }

    sLog->outString(">> Loaded %u additional achievement criteria data in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    sLog->outString();
}

void AchievementGlobalMgr::LoadCompletedAchievements()
{
    uint32 oldMSTime = getMSTime();
    bool deleteOnlyOnce = false;

    QueryResult result = CharacterDatabase.Query("SELECT achievement FROM character_achievement GROUP BY achievement");

    if (!result)
    {
        sLog->outString(">> Loaded 0 completed achievements. DB table `character_achievement` is empty.");
        sLog->outString();
        return;
    }

    do
    {
        Field *fields = result->Fetch();

        uint32 achievement_id = fields[0].GetUInt32();
        if (!sAchievementStore.LookupEntry(achievement_id))
        {
            // we will remove not existed achievement for all characters
            sLog->outError("Non-existing achievement %u data removed from table `character_achievement`.", achievement_id);
            if (!deleteOnlyOnce && achievement_id == 0)
            {
                CharacterDatabase.PExecute("DELETE FROM character_achievement WHERE achievement = %u", achievement_id);
                deleteOnlyOnce = true;
            }
            continue;
        }

        m_allCompletedAchievements.insert(achievement_id);
    } while (result->NextRow());

    sLog->outString(">> Loaded %lu completed achievements in %u ms", (unsigned long)m_allCompletedAchievements.size(), GetMSTimeDiffToNow(oldMSTime));
    sLog->outString();
}

void AchievementGlobalMgr::LoadRewards()
{
    uint32 oldMSTime = getMSTime();

    m_achievementRewards.clear();                           // need for reload case

    //                                                0      1        2        3     4       5        6
    QueryResult result = WorldDatabase.Query("SELECT entry, title_A, title_H, item, sender, subject, text FROM achievement_reward");

    if (!result)
    {
        sLog->outErrorDb(">> Loaded 0 achievement rewards. DB table `achievement_reward` is empty.");
        sLog->outString();
        return;
    }

    uint32 count = 0;

    do
    {

        Field *fields = result->Fetch();
        uint32 entry = fields[0].GetUInt32();
        const AchievementEntry* pAchievement = sAchievementStore.LookupEntry(entry);
        if (!pAchievement)
        {
            sLog->outErrorDb("Table `achievement_reward` has wrong achievement (Entry: %u), ignore", entry);
            continue;
        }

        AchievementReward reward;
        reward.titleId[0] = fields[1].GetUInt32();
        reward.titleId[1] = fields[2].GetUInt32();
        reward.itemId     = fields[3].GetUInt32();
        reward.sender     = fields[4].GetUInt32();
        reward.subject    = fields[5].GetString();
        reward.text       = fields[6].GetString();

        // must be title or mail at least
        if (!reward.titleId[0] && !reward.titleId[1] && !reward.sender)
        {
            sLog->outErrorDb("Table `achievement_reward` (Entry: %u) not have title or item reward data, ignore.", entry);
            continue;
        }

        if (pAchievement->requiredFaction == ACHIEVEMENT_FACTION_ANY && ((reward.titleId[0] == 0) != (reward.titleId[1] == 0)))
            sLog->outErrorDb("Table `achievement_reward` (Entry: %u) has title (A: %u H: %u) for only one team.", entry, reward.titleId[0], reward.titleId[1]);

        if (reward.titleId[0])
        {
            CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(reward.titleId[0]);
            if (!titleEntry)
            {
                sLog->outErrorDb("Table `achievement_reward` (Entry: %u) has invalid title id (%u) in `title_A`, set to 0", entry, reward.titleId[0]);
                reward.titleId[0] = 0;
            }
        }

        if (reward.titleId[1])
        {
            CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(reward.titleId[1]);
            if (!titleEntry)
            {
                sLog->outErrorDb("Table `achievement_reward` (Entry: %u) has invalid title id (%u) in `title_H`, set to 0", entry, reward.titleId[1]);
                reward.titleId[1] = 0;
            }
        }

        m_achievementRewards[entry] = reward;
        ++count;

    }
    while (result->NextRow());

    sLog->outString(">> Loaded %u achievement rewards in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    sLog->outString();
}

void AchievementGlobalMgr::LoadRewardLocales()
{
    uint32 oldMSTime = getMSTime();

    m_achievementRewardLocales.clear();                       // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT entry, subject_loc1, text_loc1, subject_loc2, text_loc2, subject_loc3, text_loc3, subject_loc4, text_loc4, "
                                             "subject_loc5, text_loc5, subject_loc6, text_loc6, subject_loc7, text_loc7, subject_loc8, text_loc8"
                                             " FROM locales_achievement_reward");

    if (!result)
    {
        sLog->outString(">> Loaded 0 achievement reward locale strings.  DB table `locales_achievement_reward` is empty");
        sLog->outString();
        return;
    }

    do
    {
        Field *fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        if (m_achievementRewards.find(entry) == m_achievementRewards.end())
        {
            sLog->outErrorDb("Table `locales_achievement_reward` (Entry: %u) has locale strings for non-existing achievement reward.", entry);
            continue;
        }

        AchievementRewardLocale& data = m_achievementRewardLocales[entry];

        for (int i = 1; i < TOTAL_LOCALES; ++i)
        {
            LocaleConstant locale = (LocaleConstant) i;
            std::string str = fields[1 + 2 * (i - 1)].GetString();
            sObjectMgr->AddLocaleString(str, locale, data.subject);

            str = fields[1 + 2 * (i - 1) + 1].GetString();
            sObjectMgr->AddLocaleString(str, locale, data.text);
        }
    } while (result->NextRow());

    sLog->outString(">> Loaded %lu achievement reward locale strings in %u ms", (unsigned long)m_achievementRewardLocales.size(), GetMSTimeDiffToNow(oldMSTime));
    sLog->outString();
}
