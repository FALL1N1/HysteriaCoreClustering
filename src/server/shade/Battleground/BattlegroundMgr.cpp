/*
 * Copyright (C) 
 * Copyright (C) 
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
#include "ObjectMgr.h"
#include "ArenaTeamMgr.h"
#include "Proxy.h"
#include "WorldPacket.h"
#include "ArenaTeam.h"
#include "BattlegroundMgr.h"
#include "Chat.h"
#include "MapHolder.h"
#include "Player.h"
#include "SharedDefines.h"
#include "Formulas.h"
#include "Opcodes.h"
#include "BattlegroundQueue.h"
#include "ClusterDefines.h"

/*********************************************************/
/***            BATTLEGROUND MANAGER                   ***/
/*********************************************************/

BattlegroundMgr::BattlegroundMgr() : m_ArenaTesting(false), m_Testing(false), 
    m_lastClientVisibleInstanceId(0), m_NextAutoDistributionTime(0), m_NextPeriodicQueueUpdateTime(5*IN_MILLISECONDS), randomBgDifficultyEntry(999, 0, 80, 80, 0)
{
    for (uint32 qtype = BATTLEGROUND_QUEUE_NONE; qtype < MAX_BATTLEGROUND_QUEUE_TYPES; ++qtype)
        m_BattlegroundQueues[qtype].SetBgTypeIdAndArenaType(BGTemplateId(BattlegroundQueueTypeId(qtype)), BGArenaType(BattlegroundQueueTypeId(qtype)));
}

BattlegroundMgr::~BattlegroundMgr()
{
    DeleteAllBattlegrounds();
}

void BattlegroundMgr::DeleteAllBattlegrounds()
{
    while (!m_Battlegrounds.empty())
        delete m_Battlegrounds.begin()->second;
    m_Battlegrounds.clear();

    while (!m_BattlegroundTemplates.empty())
        delete m_BattlegroundTemplates.begin()->second;
    m_BattlegroundTemplates.clear();
}

// used to update running battlegrounds, and delete finished ones
void BattlegroundMgr::Update(uint32 diff)
{
    // update all battlegrounds and delete if needed
    for (BattlegroundContainer::iterator itr = m_Battlegrounds.begin(), itrDelete; itr != m_Battlegrounds.end(); )
    {
        itrDelete = itr++;
        Battleground* bg = itrDelete->second;
        bg->Update(diff);
        if (bg->ToBeDeleted())
        {
            itrDelete->second = NULL;
            m_Battlegrounds.erase(itrDelete);
            delete bg;
        }
    }

    // update to change current bg type the random system is trying to create
    RandomSystem.Update(diff);

    // update events
    for (int qtype = BATTLEGROUND_QUEUE_NONE; qtype < MAX_BATTLEGROUND_QUEUE_TYPES; ++qtype)
        m_BattlegroundQueues[qtype].UpdateEvents(diff);

    // update using scheduled tasks (used only for rated arenas, initial opponent search works differently than periodic queue update)
    if (!m_ArenaQueueUpdateScheduler.empty())
    {
        std::vector<uint64> scheduled;
        std::swap(scheduled, m_ArenaQueueUpdateScheduler);
        for (uint8 i = 0; i < scheduled.size(); i++)
        {
            uint32 arenaRatedTeamId = scheduled[i] >> 32;
            BattlegroundQueueTypeId bgQueueTypeId = BattlegroundQueueTypeId(scheduled[i] >> 16 & 255);
            BattlegroundBracketId bracket_id = BattlegroundBracketId(scheduled[i] & 255);
            m_BattlegroundQueues[bgQueueTypeId].BattlegroundQueueUpdate(bracket_id, 0x03, true, arenaRatedTeamId); // pussywizard: looking for opponents only for this team
        }
    }

    // periodic queue update
    if (m_NextPeriodicQueueUpdateTime < diff)
    {
        // for rated arenas
        for (uint32 qtype = BATTLEGROUND_QUEUE_2v2; qtype <= BATTLEGROUND_QUEUE_5v5; ++qtype)
            for (uint32 bracket = BG_BRACKET_ID_FIRST; bracket < MAX_BATTLEGROUND_BRACKETS; ++bracket)
                m_BattlegroundQueues[qtype].BattlegroundQueueUpdate(BattlegroundBracketId(bracket), 0x03, true, 0); // pussywizard: 0 for rated means looking for opponents for every team

        // for battlegrounds and not rated arenas
        // in first loop try to fill already running battlegrounds, then in a second loop try to create new battlegrounds
        for (uint8 action = 1; action <= 2; ++action)
            for (uint32 qtype = BATTLEGROUND_QUEUE_AV; qtype <= BATTLEGROUND_QUEUE_5v5; ++qtype)
                for (uint32 bracket = BG_BRACKET_ID_FIRST; bracket < MAX_BATTLEGROUND_BRACKETS; ++bracket)
                    m_BattlegroundQueues[qtype].BattlegroundQueueUpdate(BattlegroundBracketId(bracket), action, false, 0);

        m_NextPeriodicQueueUpdateTime = 5*IN_MILLISECONDS;
    }
    else
        m_NextPeriodicQueueUpdateTime -= diff;
 
}

void BattlegroundMgr::BuildBattlegroundStatusPacket(WorldPacket* data, Battleground* bg, uint8 QueueSlot, uint8 StatusID, uint32 Time1, uint32 Time2, uint8 arenatype, TeamId teamId, bool isRated, BattlegroundTypeId forceBgTypeId)
{
    // pussywizard:
    //ASSERT(QueueSlot < PLAYER_MAX_BATTLEGROUND_QUEUES);
    sLog->outString("%u < %u", QueueSlot, PLAYER_MAX_BATTLEGROUND_QUEUES);
    if (StatusID == STATUS_NONE || !bg)
    {
        data->Initialize(SMSG_BATTLEFIELD_STATUS, 4+8);
        *data << uint32(QueueSlot);
        *data << uint64(0);
        return;
    }

    data->Initialize(SMSG_BATTLEFIELD_STATUS, (4+8+1+1+4+1+4+4+4));
    *data << uint32(QueueSlot);
    // The following segment is read as uint64 in client but can be appended as their original type.
    *data << uint8(arenatype);
    *data << uint8(bg->isArena() ? 0xE : 0x0);
    *data << uint32(forceBgTypeId != BATTLEGROUND_TYPE_NONE ? forceBgTypeId : bg->GetBgTypeID());
    *data << uint16(0x1F90);
    // End of uint64 segment, decomposed this way for simplicity
    *data << uint8(bg->GetMinLevel());
    *data << uint8(bg->GetMaxLevel());
    *data << uint32(bg->GetClientInstanceID());

    // following displays the minimap icon. 0 = faction icon, 1 = arenaicon
    *data << uint8(bg->isRated() || isRated); // 1 for rated match, 0 for bg or non rated match

    *data << uint32(StatusID);                                  // status
    switch (StatusID)
    {
        case STATUS_WAIT_QUEUE:                                 // status_in_queue
            *data << uint32(Time1);                             // average wait time, milliseconds
            *data << uint32(Time2);                             // time in queue, updated every minute!, milliseconds
            break;
        case STATUS_WAIT_JOIN:                                  // status_invite
            *data << uint32(bg->GetMapId());                    // map id
            *data << uint64(0);                                 // 3.3.5, unknown
            *data << uint32(Time1);                             // time to remove from queue, milliseconds
            break;
        case STATUS_IN_PROGRESS:                                // status_in_progress
            *data << uint32(bg->GetMapId());                    // map id
            *data << uint64(0);                                 // 3.3.5, unknown
            *data << uint32(Time1);                             // time to bg auto leave, 0 at bg start, 120000 after bg end, milliseconds
            *data << uint32(Time2);                             // time from bg start, milliseconds
            *data << uint8(teamId == TEAM_ALLIANCE ? 1 : 0);   // arenafaction (0 for horde, 1 for alliance)
            break;
        default:
            break;
    }
}

void BattlegroundMgr::BuildPvpLogDataPacket(WorldPacket* data, Battleground* bg)
{
    return;
}

void BattlegroundMgr::BuildGroupJoinedBattlegroundPacket(WorldPacket* data, GroupJoinBattlegroundResult result)
{
    data->Initialize(SMSG_GROUP_JOINED_BATTLEGROUND, 4);
    *data << int32(result);
    if (result == ERR_BATTLEGROUND_JOIN_TIMED_OUT || result == ERR_BATTLEGROUND_JOIN_FAILED)
        *data << uint64(0);                                 // player guid
}

void BattlegroundMgr::BuildUpdateWorldStatePacket(WorldPacket* data, uint32 field, uint32 value)
{
    data->Initialize(SMSG_UPDATE_WORLD_STATE, 4+4);
    *data << uint32(field);
    *data << uint32(value);
}

void BattlegroundMgr::BuildPlaySoundPacket(WorldPacket* data, uint32 soundid)
{
    data->Initialize(SMSG_PLAY_SOUND, 4);
    *data << uint32(soundid);
}

void BattlegroundMgr::BuildPlayerLeftBattlegroundPacket(WorldPacket* data, uint64 guid)
{
    data->Initialize(SMSG_BATTLEGROUND_PLAYER_LEFT, 8);
    *data << uint64(guid);
}

void BattlegroundMgr::BuildPlayerJoinedBattlegroundPacket(WorldPacket* data, Player* player)
{
    data->Initialize(SMSG_BATTLEGROUND_PLAYER_JOINED, 8);
    *data << uint64(player->GetGUID());
}

Battleground* BattlegroundMgr::GetBattleground(uint32 instanceId)
{
    if (!instanceId)
        return NULL;

    BattlegroundContainer::const_iterator itr = m_Battlegrounds.find(instanceId);
    if (itr != m_Battlegrounds.end())
       return itr->second;

    return NULL;
}

Battleground* BattlegroundMgr::GetBattlegroundTemplate(BattlegroundTypeId bgTypeId)
{
    BattlegroundTemplateContainer::const_iterator itr = m_BattlegroundTemplates.find(bgTypeId);
    if (itr != m_BattlegroundTemplates.end()) {
        sLog->outString("+");
        return itr->second;
    }

    return NULL;
}

uint32 BattlegroundMgr::GetNextClientVisibleInstanceId()
{
    return ++m_lastClientVisibleInstanceId;
}

// create a new battleground that will really be used to play
Battleground* BattlegroundMgr::CreateNewBattleground(BattlegroundTypeId bgTypeId, uint32 minLevel, uint32 maxLevel, uint8 arenaType, bool isRated)
{
    return nullptr;
}

// used to create the BG templates
bool BattlegroundMgr::CreateBattleground(CreateBattlegroundData& data)
{
    // Create the BG
    Battleground* bg = new Battleground();

    bg->SetMapId(data.bgTypeId == BATTLEGROUND_RB ? randomBgDifficultyEntry.mapId : data.MapID);
    bg->SetBgTypeID(data.bgTypeId);
    bg->SetInstanceID(0);
    bg->SetArenaorBGType(data.IsArena);
    bg->SetMinPlayersPerTeam(data.MinPlayersPerTeam);
    bg->SetMaxPlayersPerTeam(data.MaxPlayersPerTeam);
    bg->SetName(data.BattlegroundName);
    bg->SetTeamStartLoc(TEAM_ALLIANCE, data.Team1StartLocX, data.Team1StartLocY, data.Team1StartLocZ, data.Team1StartLocO);
    bg->SetTeamStartLoc(TEAM_HORDE, data.Team2StartLocX, data.Team2StartLocY, data.Team2StartLocZ, data.Team2StartLocO);
    bg->SetStartMaxDist(data.StartMaxDist);
    bg->SetLevelRange(data.LevelMin, data.LevelMax);
    bg->SetScriptId(data.scriptId);
    AddBattleground(bg);
    return true;
}

void BattlegroundMgr::CreateInitialBattlegrounds()
{
    uint32 oldMSTime = getMSTime();
    //                                               0   1                  2                  3       4       5                 6               7              8            9             10      11
    QueryResult result = WorldDatabase.Query("SELECT id, MinPlayersPerTeam, MaxPlayersPerTeam, MinLvl, MaxLvl, AllianceStartLoc, AllianceStartO, HordeStartLoc, HordeStartO, StartMaxDist, Weight, ScriptName FROM battleground_template");

    if (!result)
    {
        sLog->outError(">> Loaded 0 battlegrounds. DB table `battleground_template` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 bgTypeId = fields[0].GetUInt32();

        // pussywizard: currently not supported
        //if (DisableMgr::IsDisabledFor(DISABLE_TYPE_BATTLEGROUND, bgTypeId, NULL))
        //    continue;

        // can be overwrite by values from DB
        BattlemasterListEntry const* bl = sBattlemasterListStore.LookupEntry(bgTypeId);
        if (!bl)
        {
            sLog->outError("Battleground ID %u not found in BattlemasterList.dbc. Battleground not created.", bgTypeId);
            continue;
        }

        CreateBattlegroundData data;
        data.bgTypeId = BattlegroundTypeId(bgTypeId);
        data.IsArena = (bl->type == TYPE_ARENA);
        data.MinPlayersPerTeam = fields[1].GetUInt16();
        data.MaxPlayersPerTeam = fields[2].GetUInt16();
        data.LevelMin = fields[3].GetUInt8();
        data.LevelMax = fields[4].GetUInt8();
        float dist = fields[9].GetFloat();
        data.StartMaxDist = dist * dist;

        data.scriptId = sObjectMgr->GetScriptId(fields[11].GetCString());
        data.BattlegroundName = bl->name[sLogon->GetDefaultDbcLocale()];
        data.MapID = bl->mapid[0];

        if (data.MaxPlayersPerTeam == 0 || data.MinPlayersPerTeam > data.MaxPlayersPerTeam)
        {
            sLog->outError("Table `battleground_template` for id %u has bad values for MinPlayersPerTeam (%u) and MaxPlayersPerTeam(%u)",
                data.bgTypeId, data.MinPlayersPerTeam, data.MaxPlayersPerTeam);
            continue;
        }

        if (data.LevelMin == 0 || data.LevelMax == 0 || data.LevelMin > data.LevelMax)
        {
            sLog->outError("Table `battleground_template` for id %u has bad values for LevelMin (%u) and LevelMax(%u)",
                data.bgTypeId, data.LevelMin, data.LevelMax);
            continue;
        }

        if (data.bgTypeId == BATTLEGROUND_AA || data.bgTypeId == BATTLEGROUND_RB)
        {
            data.Team1StartLocX = 0;
            data.Team1StartLocY = 0;
            data.Team1StartLocZ = 0;
            data.Team1StartLocO = fields[6].GetFloat();
            data.Team2StartLocX = 0;
            data.Team2StartLocY = 0;
            data.Team2StartLocZ = 0;
            data.Team2StartLocO = fields[8].GetFloat();
        }
        else
        {
            uint32 startId = fields[5].GetUInt32();
            if (WorldSafeLocsEntry const* start = sWorldSafeLocsStore.LookupEntry(startId))
            {
                data.Team1StartLocX = start->x;
                data.Team1StartLocY = start->y;
                data.Team1StartLocZ = start->z;
                data.Team1StartLocO = fields[6].GetFloat();
            }
            else
            {
                sLog->outError("Table `battleground_template` for id %u have non-existed WorldSafeLocs.dbc id %u in field `AllianceStartLoc`. BG not created.", data.bgTypeId, startId);
                continue;
            }

            startId = fields[7].GetUInt32();
            if (WorldSafeLocsEntry const* start = sWorldSafeLocsStore.LookupEntry(startId))
            {
                data.Team2StartLocX = start->x;
                data.Team2StartLocY = start->y;
                data.Team2StartLocZ = start->z;
                data.Team2StartLocO = fields[8].GetFloat();
            }
            else
            {
                sLog->outError("Table `battleground_template` for id %u have non-existed WorldSafeLocs.dbc id %u in field `HordeStartLoc`. BG not created.", data.bgTypeId, startId);
                continue;
            }
        }

        if (!CreateBattleground(data))
            continue;

        ++count;
    } while (result->NextRow());

    sLog->outString(">> Loaded %u battlegrounds in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void BattlegroundMgr::InitAutomaticArenaPointDistribution()
{
    return;
}

void BattlegroundMgr::BuildBattlegroundListPacket(WorldPacket* data, uint64 guid, Player* player, BattlegroundTypeId bgTypeId, uint8 fromWhere)
{
    return;
}

void BattlegroundMgr::SendToBattleground(Player* player, uint32 instanceId, BattlegroundTypeId bgTypeId)
{
    return;
    /*
    if (Battleground* bg = GetBattleground(instanceId))
    {
        float x, y, z, o;
        bg->GetTeamStartLoc(player->GetBgTeamId(), x, y, z, o);
        player->TeleportTo(bg->GetMapId(), x, y, z, o);
    }*/
}

void BattlegroundMgr::SendAreaSpiritHealerQueryOpcode(Player* player, Battleground* bg, uint64 guid)
{
    return;
}

bool BattlegroundMgr::IsArenaType(BattlegroundTypeId bgTypeId)
{
    return bgTypeId == BATTLEGROUND_AA
            || bgTypeId == BATTLEGROUND_BE
            || bgTypeId == BATTLEGROUND_NA
            || bgTypeId == BATTLEGROUND_DS
            || bgTypeId == BATTLEGROUND_RV
            || bgTypeId == BATTLEGROUND_RL;
}

BattlegroundQueueTypeId BattlegroundMgr::BGQueueTypeId(BattlegroundTypeId bgTypeId, uint8 arenaType)
{
    switch (bgTypeId)
    {
        case BATTLEGROUND_AB:
            return BATTLEGROUND_QUEUE_AB;
        case BATTLEGROUND_AV:
            return BATTLEGROUND_QUEUE_AV;
        case BATTLEGROUND_EY:
            return BATTLEGROUND_QUEUE_EY;
        case BATTLEGROUND_IC:
            return BATTLEGROUND_QUEUE_IC;
        case BATTLEGROUND_RB:
            return BATTLEGROUND_QUEUE_RB;
        case BATTLEGROUND_SA:
            return BATTLEGROUND_QUEUE_SA;
        case BATTLEGROUND_WS:
            return BATTLEGROUND_QUEUE_WS;
        case BATTLEGROUND_AA:
        case BATTLEGROUND_BE:
        case BATTLEGROUND_DS:
        case BATTLEGROUND_NA:
        case BATTLEGROUND_RL:
        case BATTLEGROUND_RV:
            switch (arenaType)
            {
                case ARENA_TYPE_2v2:
                    return BATTLEGROUND_QUEUE_2v2;
                case ARENA_TYPE_3v3:
                    return BATTLEGROUND_QUEUE_3v3;
                case ARENA_TYPE_5v5:
                    return BATTLEGROUND_QUEUE_5v5;
                default:
                    return BATTLEGROUND_QUEUE_NONE;
            }
        default:
            return BATTLEGROUND_QUEUE_NONE;
    }
}

BattlegroundTypeId BattlegroundMgr::BGTemplateId(BattlegroundQueueTypeId bgQueueTypeId)
{
    switch (bgQueueTypeId)
    {
        case BATTLEGROUND_QUEUE_WS:
            return BATTLEGROUND_WS;
        case BATTLEGROUND_QUEUE_AB:
            return BATTLEGROUND_AB;
        case BATTLEGROUND_QUEUE_AV:
            return BATTLEGROUND_AV;
        case BATTLEGROUND_QUEUE_EY:
            return BATTLEGROUND_EY;
        case BATTLEGROUND_QUEUE_SA:
            return BATTLEGROUND_SA;
        case BATTLEGROUND_QUEUE_IC:
            return BATTLEGROUND_IC;
        case BATTLEGROUND_QUEUE_RB:
            return BATTLEGROUND_RB;
        case BATTLEGROUND_QUEUE_2v2:
        case BATTLEGROUND_QUEUE_3v3:
        case BATTLEGROUND_QUEUE_5v5:
            return BATTLEGROUND_AA;
        default:
            return BattlegroundTypeId(0);                   // used for unknown template (it existed and do nothing)
    }
}

uint8 BattlegroundMgr::BGArenaType(BattlegroundQueueTypeId bgQueueTypeId)
{
    switch (bgQueueTypeId)
    {
        case BATTLEGROUND_QUEUE_2v2:
            return ARENA_TYPE_2v2;
        case BATTLEGROUND_QUEUE_3v3:
            return ARENA_TYPE_3v3;
        case BATTLEGROUND_QUEUE_5v5:
            return ARENA_TYPE_5v5;
        default:
            return 0;
    }
}

void BattlegroundMgr::ToggleTesting()
{
    m_Testing = !m_Testing; 
}

void BattlegroundMgr::ToggleArenaTesting()
{
    m_ArenaTesting = !m_ArenaTesting; 
}

void BattlegroundMgr::SetHolidayWeekends(uint32 mask)
{
    for (uint32 bgtype = 1; bgtype < MAX_BATTLEGROUND_TYPE_ID; ++bgtype)
    {
        if (bgtype == BATTLEGROUND_RB)
            continue;
        if (Battleground* bgt = GetBattlegroundTemplate(BattlegroundTypeId(bgtype)))
            bgt->SetHoliday(mask & (1 << bgtype));
    }
} 

uint32 BattlegroundMgr::GetRatingDiscardTimer() const
{
    return 0;
}

uint32 BattlegroundMgr::GetPrematureFinishTime() const
{
    return 0;
}

void BattlegroundMgr::LoadBattleMastersEntry()
{
    return;
}

void BattlegroundMgr::CheckBattleMasters()
{
    return;
}

HolidayIds BattlegroundMgr::BGTypeToWeekendHolidayId(BattlegroundTypeId bgTypeId)
{
    switch (bgTypeId)
    {
        case BATTLEGROUND_AV: return HOLIDAY_CALL_TO_ARMS_AV;
        case BATTLEGROUND_EY: return HOLIDAY_CALL_TO_ARMS_EY;
        case BATTLEGROUND_WS: return HOLIDAY_CALL_TO_ARMS_WS;
        case BATTLEGROUND_SA: return HOLIDAY_CALL_TO_ARMS_SA;
        case BATTLEGROUND_AB: return HOLIDAY_CALL_TO_ARMS_AB;
        case BATTLEGROUND_IC: return HOLIDAY_CALL_TO_ARMS_IC;
        default: return HOLIDAY_NONE;
    }
}

BattlegroundTypeId BattlegroundMgr::WeekendHolidayIdToBGType(HolidayIds holiday)
{
    switch (holiday)
    {
        case HOLIDAY_CALL_TO_ARMS_AV: return BATTLEGROUND_AV;
        case HOLIDAY_CALL_TO_ARMS_EY: return BATTLEGROUND_EY;
        case HOLIDAY_CALL_TO_ARMS_WS: return BATTLEGROUND_WS;
        case HOLIDAY_CALL_TO_ARMS_SA: return BATTLEGROUND_SA;
        case HOLIDAY_CALL_TO_ARMS_AB: return BATTLEGROUND_AB;
        case HOLIDAY_CALL_TO_ARMS_IC: return BATTLEGROUND_IC;
        default: return BATTLEGROUND_TYPE_NONE;
    }
}

bool BattlegroundMgr::IsBGWeekend(BattlegroundTypeId bgTypeId)
{
    return false;
}

void BattlegroundMgr::AddBattleground(Battleground* bg)
{
    if (bg->GetInstanceID() == 0)
        m_BattlegroundTemplates[bg->GetBgTypeID()] = bg;
    else
        m_Battlegrounds[bg->GetInstanceID()] = bg;
}

void BattlegroundMgr::RemoveBattleground(BattlegroundTypeId bgTypeId, uint32 instanceId)
{
    if (instanceId == 0)
        m_BattlegroundTemplates.erase(bgTypeId);
    else
        m_Battlegrounds.erase(instanceId);
}

void BattlegroundMgr::InviteGroupToBG(GroupQueueInfo* ginfo, Battleground* bg, TeamId teamId)
{
    return;
}

RandomBattlegroundSystem::RandomBattlegroundSystem() : m_CurrentRandomBg(BATTLEGROUND_TYPE_NONE), m_SwitchTimer(0)
{
}

void RandomBattlegroundSystem::Update(uint32 diff)
{
    if (m_SwitchTimer <= diff)
    {
        if (m_BgOrder.empty())
        {
            // order it like: big, small, big, small, small, small (stored backwards, actually)
            std::vector<BattlegroundTypeId> big, small;
            small.push_back(BATTLEGROUND_WS);
            small.push_back(BATTLEGROUND_EY);
            small.push_back(BATTLEGROUND_AB);
            small.push_back(BATTLEGROUND_SA);
            std::random_shuffle(small.begin(), small.end());
            m_BgOrder.push_back(small.back()); small.pop_back();
            m_BgOrder.push_back(small.back()); small.pop_back();
            m_BgOrder.push_back(small.back()); small.pop_back();
            m_BgOrder.push_back(small.back()); small.pop_back();
        }

        m_CurrentRandomBg = m_BgOrder.back();
        m_BgOrder.pop_back();

        switch (m_CurrentRandomBg)
        {
            case BATTLEGROUND_AV: m_SwitchTimer = 180*IN_MILLISECONDS; break; // max 40 per team
            case BATTLEGROUND_WS: m_SwitchTimer = 30*IN_MILLISECONDS; break; // max 10 per team
            case BATTLEGROUND_IC: m_SwitchTimer = 180*IN_MILLISECONDS; break; // max 40 per team
            case BATTLEGROUND_EY: m_SwitchTimer = 40*IN_MILLISECONDS; break; // max 15 per team
            case BATTLEGROUND_AB: m_SwitchTimer = 40*IN_MILLISECONDS; break; // max 15 per team
            case BATTLEGROUND_SA: m_SwitchTimer = 40*IN_MILLISECONDS; break; // max 15 per team
            default: ASSERT(false); break;
        }
    }
    else
        m_SwitchTimer -= diff;
}

void RandomBattlegroundSystem::BattlegroundCreated(BattlegroundTypeId bgTypeId)
{
    // if created current random bg, set current to another one
    if (bgTypeId == m_CurrentRandomBg)
        Update(0xffffffff);
}
