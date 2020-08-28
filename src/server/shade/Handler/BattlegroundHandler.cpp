/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
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

#include "Language.h"
#include "WorldPacket.h"
#include "Common.h"
#include "ObjectMgr.h" 
#include "Player.h"
#include "Util.h"
#include "ClientSessionMgr.h"
#include "BattlegroundMgr.h"

void ClientSession::HandleBattlemasterJoinOpcode(WorldPacket & recvData)
{  

}


void ClientSession::HandleBattlefieldListOpcode(WorldPacket &recvData)
{
    ;//sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd CMSG_BATTLEFIELD_LIST Message");

    uint32 bgTypeId;
    recvData >> bgTypeId;                                  // id from DBC

    uint8 fromWhere;
    recvData >> fromWhere;                                 // 0 - battlemaster (lua: ShowBattlefieldList), 1 - UI (lua: RequestBattlegroundInstanceInfo)

    uint8 canGainXP;
    recvData >> canGainXP;                                 // players with locked xp have their own bg queue on retail

    BattlemasterListEntry const* bl = sBattlemasterListStore.LookupEntry(bgTypeId);
    if (!bl)
        return;

    WorldPacket data;
    sBattlegroundMgr->BuildBattlegroundListPacket(&data, 0, _player, BattlegroundTypeId(bgTypeId), fromWhere);
    SendPacket(&data);
}

void ClientSession::HandleBattleFieldPortOpcode(WorldPacket &recvData)
{
    uint8 arenaType;                                        // arenatype if arena
    uint8 unk2;                                             // unk, can be 0x0 (may be if was invited?) and 0x1
    uint32 bgTypeId_;                                       // type id from dbc
    uint16 unk;                                             // 0x1F90 constant?
    uint8 action;                                           // enter battle 0x1, leave queue 0x0

    recvData >> arenaType >> unk2 >> bgTypeId_ >> unk >> action;

    // bgTypeId not valid
    if (!sBattlemasterListStore.LookupEntry(bgTypeId_))
        return;

    // player not in any queue, so can't really answer
    if (!_player->InBattlegroundQueue())
        return;

    // get BattlegroundQueue for received 
    BattlegroundTypeId bgTypeId = BattlegroundTypeId(bgTypeId_);
    BattlegroundQueueTypeId bgQueueTypeId = BattlegroundMgr::BGQueueTypeId(bgTypeId, arenaType);
    BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId);

    // get group info from queue
    GroupQueueInfo ginfo;
    if (!bgQueue.GetPlayerGroupInfoData(_player->GetGUID(), &ginfo))
        return;

    // to accept, player must be invited to particular battleground id
    if (!ginfo.IsInvitedToBGInstanceGUID && action == 1)
        return;

    Battleground* bg = sBattlegroundMgr->GetBattleground(ginfo.IsInvitedToBGInstanceGUID);

    // use template if leaving queue (instance might not be created yet)
    if (!bg && action == 0)
        bg = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId);

    if (!bg)
        return;

    // expected bracket entry
    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bg->GetMapId(), _player->getLevel());
    if (!bracketEntry)
        return;

    // safety checks
    if (action == 1 && ginfo.ArenaType == 0)
    {
        // can't join with deserter, check it here right before joining to be sure
        if (!_player->CanJoinToBattleground())
        {
            WorldPacket data;
            sBattlegroundMgr->BuildGroupJoinedBattlegroundPacket(&data, ERR_GROUP_JOIN_BATTLEGROUND_DESERTERS);
            SendPacket(&data);
            action = 0;
        }

        if (_player->getLevel() > bg->GetMaxLevel())
            action = 0;
    }

    // get player queue slot index for this bg (can be in up to 2 queues at the same time)
    uint32 queueSlot = _player->GetBattlegroundQueueIndex(bgQueueTypeId);

    WorldPacket data;
    switch (action)
    {
    case 1: // accept
    {
        // set entry point if not in battleground
        //if (!_player->InBattleground())
            //_player->SetEntryPoint();

        // resurrect the player
        //if (!_player->IsAlive())
        //{
        //    _player->ResurrectPlayer(1.0f);
        //    _player->SpawnCorpseBones();
        //}

        TeamId teamId = ginfo.teamId;

        if (!bg->HasPlayerJoinPremade(_player->GetGUID())) // Premade players have already defined team and cf settings
            if (bgQueueTypeId == BATTLEGROUND_QUEUE_RB && bg->isBattleground())// && sWorld->getBoolConfig(CONFIG_BATTLEGROUND_RANDOM_CROSSFACTION))
            {
                uint32 allyCount = bg->GetPlayersCountByTeam(TEAM_ALLIANCE) + bg->GetPremadeCount(TEAM_ALLIANCE);
                uint32 hordeCount = bg->GetPlayersCountByTeam(TEAM_HORDE) + bg->GetPremadeCount(TEAM_HORDE);

                if (allyCount == hordeCount)
                {
                    if (roll_chance_i(50))
                        teamId = _player->GetTeamId() == TEAM_ALLIANCE ? TEAM_HORDE : TEAM_ALLIANCE;
                }
                else if (allyCount < hordeCount)
                    teamId = TEAM_ALLIANCE;
                else
                    teamId = TEAM_HORDE;
            }

        //_player->setTeamId(teamId);
        //_player->setFaction(teamId == TEAM_ALLIANCE ? 1 : 2);
        // remove player from all bg queues
        for (uint32 qslot = 0; qslot < PLAYER_MAX_BATTLEGROUND_QUEUES; ++qslot)
            if (BattlegroundQueueTypeId q = _player->GetBattlegroundQueueTypeId(qslot))
            {
                BattlegroundQueue& queue = sBattlegroundMgr->GetBattlegroundQueue(q);
                queue.RemovePlayer(_player->GetGUID(), (bgQueueTypeId == q), qslot);
                _player->RemoveBattlegroundQueueId(q);
            }

        // send status packet
        sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, queueSlot, STATUS_IN_PROGRESS, 0, bg->GetStartTime(), bg->GetArenaType(), teamId);
        SendPacket(&data);

        //_player->SetBattlegroundId(bg->GetInstanceID(), bg->GetBgTypeID(), queueSlot, true, bgTypeId == BATTLEGROUND_RB, teamId);

        sBattlegroundMgr->SendToBattleground(_player, ginfo.IsInvitedToBGInstanceGUID, bgTypeId);
    }
    break;
    case 0: // leave queue
    {
        bgQueue.RemovePlayer(_player->GetGUID(), false, queueSlot);
        _player->RemoveBattlegroundQueueId(bgQueueTypeId);
    }
    break;
    default:
        break;
    }
}
