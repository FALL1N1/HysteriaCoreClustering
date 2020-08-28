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

#include "ArenaTeam.h"
#include "ArenaTeamMgr.h"
#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "Chat.h"
#include "Formulas.h" 
#include "ObjectMgr.h"
#include "Player.h"
#include "Util.h"
#include "Proxy.h"
#include "WorldPacket.h"

template<class Do>
void Battleground::BroadcastWorker(Do& _do)
{
 
}

Battleground::Battleground()
{
 
}

Battleground::~Battleground()
{
 
}

void Battleground::Update(uint32 diff)
{
  
}

inline void Battleground::_CheckSafePositions(uint32 diff)
{
    float maxDist = GetStartMaxDist();
    if (!maxDist)
        return; 
}
 
TeamId Battleground::GetPrematureWinner()
{
    if (GetPlayersCountByTeam(TEAM_ALLIANCE) >= GetMinPlayersPerTeam())
        return TEAM_ALLIANCE;
    else if (GetPlayersCountByTeam(TEAM_HORDE) >= GetMinPlayersPerTeam())
        return TEAM_HORDE;
        
    return TEAM_NEUTRAL;
}

void Battleground::SetTeamStartLoc(TeamId teamId, float X, float Y, float Z, float O)
{
    m_TeamStartLocX[teamId] = X;
    m_TeamStartLocY[teamId] = Y;
    m_TeamStartLocZ[teamId] = Z;
    m_TeamStartLocO[teamId] = O;
}

void Battleground::SendPacketToAll(WorldPacket* packet)
{
    for (BattlegroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
        itr->second->GetSession()->SendPacket(packet);
}

void Battleground::SendPacketToTeam(TeamId teamId, WorldPacket* packet, Player* sender, bool self)
{

}

void Battleground::PlaySoundToAll(uint32 soundID)
{
    WorldPacket data;
    sBattlegroundMgr->BuildPlaySoundPacket(&data, soundID);
    SendPacketToAll(&data);
}

void Battleground::CastSpellOnTeam(uint32 spellId, TeamId teamId)
{

}

void Battleground::RemoveAuraOnTeam(uint32 spellId, TeamId teamId)
{

}

void Battleground::YellToAll(Creature* creature, char const* text, uint32 language)
{

}

void Battleground::RewardHonorToTeam(uint32 honor, TeamId teamId)
{
}

void Battleground::RewardReputationToTeam(uint32 factionId, uint32 reputation, TeamId teamId)
{

}

uint32 Battleground::GetRealRepFactionForPlayer(uint32 factionId, Player* player)
{
    return 0;
}


void Battleground::UpdateWorldState(uint32 Field, uint32 Value)
{
    WorldPacket data;
    sBattlegroundMgr->BuildUpdateWorldStatePacket(&data, Field, Value);
    SendPacketToAll(&data);
}

void Battleground::UpdateWorldStateForPlayer(uint32 Field, uint32 Value, Player* player)
{
    WorldPacket data;
    sBattlegroundMgr->BuildUpdateWorldStatePacket(&data, Field, Value);
    player->GetSession()->SendPacket(&data);
}

void Battleground::EndBattleground(TeamId winnerTeamId)
{
 
}

uint32 Battleground::GetBonusHonorFromKill(uint32 kills) const
{
    return 0;
}

void Battleground::BlockMovement(Player* player)
{

}

void Battleground::RemovePlayerAtLeave(Player* player)
{
 
}

// this method is called when creating bg
void Battleground::Init()
{

}

void Battleground::StartBattleground()
{

}

void Battleground::AddPlayer(Player* player)
{

}

// this method adds player to his team's bg group, or sets his correct group if player is already in bg group
void Battleground::AddOrSetPlayerToCorrectBgGroup(Player* player, TeamId teamId)
{
 
}

uint32 Battleground::GetFreeSlotsForTeam(TeamId teamId) const
{
    return 0;
}

uint32 Battleground::GetMaxFreeSlots() const
{
    return std::max<uint32>(GetFreeSlotsForTeam(TEAM_ALLIANCE), GetFreeSlotsForTeam(TEAM_HORDE));
}

bool Battleground::HasFreeSlots() const
{
    if (GetStatus() != STATUS_WAIT_JOIN && GetStatus() != STATUS_IN_PROGRESS)
        return false;
    for (uint8 i=0; i<BG_TEAMS_COUNT; ++i)
        if (GetFreeSlotsForTeam((TeamId)i) > 0)
            return true;
    return false;
}

void Battleground::SpectatorsSendPacket(WorldPacket& data)
{

}

void Battleground::ReadyMarkerClicked(Player* p)
{

}

void Battleground::UpdatePlayerScore(Player* player, uint32 type, uint32 value, bool doAddHonor)
{
  
}

void Battleground::AddPlayerToResurrectQueue(uint64 npc_guid, uint64 player_guid)
{

}

void Battleground::RemovePlayerFromResurrectQueue(Player* player)
{

}

void Battleground::RelocateDeadPlayers(uint64 queueIndex)
{

}

bool Battleground::AddObject(uint32 type, uint32 entry, float x, float y, float z, float o, float rotation0, float rotation1, float rotation2, float rotation3, uint32 /*respawnTime*/)
{
    return false;
}

// Some doors aren't despawned so we cannot handle their closing in gameobject::update()
// It would be nice to correctly implement GO_ACTIVATED state and open/close doors in gameobject code
void Battleground::DoorClose(uint32 type)
{

}

void Battleground::DoorOpen(uint32 type)
{

}

void Battleground::SpawnBGObject(uint32 type, uint32 respawntime)
{

}

bool Battleground::DelCreature(uint32 type)
{
    return false;
}

bool Battleground::DelObject(uint32 type)
{
    return false;
}

bool Battleground::AddSpiritGuide(uint32 type, float x, float y, float z, float o, TeamId teamId)
{
    return false;
}

void Battleground::SendMessageToAll(int32 entry, ChatMsg type, Player const* source)
{

}

void Battleground::PSendMessageToAll(int32 entry, ChatMsg type, Player const* source, ...)
{

}

void Battleground::SendWarningToAll(int32 entry, ...)
{

}

void Battleground::SendMessage2ToAll(int32 entry, ChatMsg type, Player const* source, int32 arg1, int32 arg2)
{

}

void Battleground::EndNow()
{
    SetStatus(STATUS_WAIT_LEAVE);
    SetEndTime(0);
}

// To be removed
char const* Battleground::GetTrinityString(int32 entry)
{
    // FIXME: now we have different DBC locales and need localized message for each target client
    return sObjectMgr->GetTrinityStringForDBCLocale(entry);
}

void Battleground::HandleTriggerBuff(GameObject* gameObject)
{

}

void Battleground::HandleKillPlayer(Player* victim, Player* killer)
{

}

TeamId Battleground::GetOtherTeamId(TeamId teamId)
{
    return teamId != TEAM_NEUTRAL ? (teamId == TEAM_ALLIANCE ? TEAM_HORDE : TEAM_ALLIANCE) : TEAM_NEUTRAL;
}

bool Battleground::IsPlayerInBattleground(uint64 guid) const
{
    BattlegroundPlayerMap::const_iterator itr = m_Players.find(guid);
    if (itr != m_Players.end())
        return true;
    return false;
}

void Battleground::PlayerAddedToBGCheckIfBGIsRunning(Player* player)
{

}

uint32 Battleground::GetAlivePlayersCountByTeam(TeamId teamId) const
{
    return 0;
}

void Battleground::SetHoliday(bool is_holiday)
{
    m_HonorMode = is_holiday ? BG_HOLIDAY : BG_NORMAL;
}

int32 Battleground::GetObjectType(uint64 guid)
{
    for (uint32 i = 0; i < BgObjects.size(); ++i)
        if (BgObjects[i] == guid)
            return i;
    sLog->outError("Battleground::GetObjectType: player used gameobject (GUID: %u) which is not in internal data for BG (map: %u, instance id: %u), cheating?",
        GUID_LOPART(guid), m_MapId, m_InstanceID);
    return -1;
}

void Battleground::HandleKillUnit(Creature* /*victim*/, Player* /*killer*/)
{
}

void Battleground::CheckArenaAfterTimerConditions()
{
    EndBattleground(TEAM_NEUTRAL);
}

void Battleground::CheckArenaWinConditions()
{
    if (isArena() && GetStatus() <= STATUS_WAIT_JOIN) // pussywizard
        return;
    if (!GetAlivePlayersCountByTeam(TEAM_ALLIANCE) && GetPlayersCountByTeam(TEAM_HORDE))
        EndBattleground(TEAM_HORDE);
    else if (GetPlayersCountByTeam(TEAM_ALLIANCE) && !GetAlivePlayersCountByTeam(TEAM_HORDE))
        EndBattleground(TEAM_ALLIANCE);
}

void Battleground::UpdateArenaWorldState()
{
    UpdateWorldState(0xe10, GetAlivePlayersCountByTeam(TEAM_HORDE));
    UpdateWorldState(0xe11, GetAlivePlayersCountByTeam(TEAM_ALLIANCE));
}

void Battleground::SetBgRaid(TeamId teamId, Group* bg_raid)
{

}

void Battleground::StartTimedAchievement(AchievementCriteriaTimedTypes type, uint32 entry)
{

}

uint32 Battleground::GetTeamScore(TeamId teamId) const
{
    if (teamId == TEAM_ALLIANCE || teamId == TEAM_HORDE)
        return m_TeamScores[teamId];

    sLog->outError("GetTeamScore with wrong Team %u for BG %u", teamId, GetBgTypeID());
    return 0;
}

void Battleground::RewardXPAtKill(Player* killer, Player* victim)
{

}

uint8 Battleground::GetUniqueBracketId() const
{
    return GetMinLevel() / 10;
}

bool Battleground::HasPlayerJoinPremade(uint64 guid)
{
    for (std::unordered_map<uint64, bool>::iterator itr = m_hasPlayerJoinedPremade.begin(); itr != m_hasPlayerJoinedPremade.end(); itr++)
        if (itr->first == guid)
            return itr->second;

    return false;
}