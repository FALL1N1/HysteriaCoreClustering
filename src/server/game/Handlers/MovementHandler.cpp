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

#include "AnticheatMgr.h"
#include "Common.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "Corpse.h"
#include "Player.h"
#include "SpellAuras.h"
#include "MapManager.h"
#include "Transport.h"
#include "Battleground.h"
#include "WaypointMovementGenerator.h"
#include "InstanceSaveMgr.h"
#include "ObjectMgr.h"
#include "CellImpl.h"
#include "Pet.h"
#include "ArenaSpectator.h"
#include "Chat.h"
#include "BattlegroundMgr.h"

#include "World.h" 

// Movement anticheat defines
//#define ANTICHEAT_DEBUG
#define ANTICHEAT_EXCEPTION_INFO
// End Movement anticheat defines

void WorldSession::HandleMoveWorldportAckOpcode(WorldPacket & /*recvData*/)
{
    ;//sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: got MSG_MOVE_WORLDPORT_ACK.");
    HandleMoveWorldportAckOpcode();
}

void WorldSession::HandleMoveWorldportAckOpcode()
{
    // ignore unexpected far teleports
    if (!GetPlayer()->IsBeingTeleportedFar())
        return;

    GetPlayer()->SetSemaphoreTeleportFar(0);

    // get the teleport destination
    WorldLocation const& loc = GetPlayer()->GetTeleportDest();

    // possible errors in the coordinate validity check
    if (!MapManager::IsValidMapCoord(loc))
    {
        KickPlayer();
        return;
    }

    // get the destination map entry, not the current one, this will fix homebind and reset greeting
    MapEntry const* mEntry = sMapStore.LookupEntry(loc.GetMapId());
    InstanceTemplate const* mInstance = sObjectMgr->GetInstanceTemplate(loc.GetMapId());

    Map* oldMap = GetPlayer()->GetMap();
    if (GetPlayer()->IsInWorld())
    {
        sLog->outError("Player (Name %s) is still in world when teleported from map %u to new map %u", GetPlayer()->GetName().c_str(), oldMap->GetId(), loc.GetMapId());
        oldMap->RemovePlayerFromMap(GetPlayer(), false);
    }

    // reset instance validity, except if going to an instance inside an instance
    if (GetPlayer()->m_InstanceValid == false && !mInstance)
    {
        GetPlayer()->m_InstanceValid = true;
        // pussywizard: m_InstanceValid can be false only by leaving a group in an instance => so remove temp binds that could not be removed because player was still on the map!
        if (!sInstanceSaveMgr->PlayerIsPermBoundToInstance(GetPlayer()->GetGUIDLow(), oldMap->GetId(), oldMap->GetDifficulty()))
            sInstanceSaveMgr->PlayerUnbindInstance(GetPlayer()->GetGUIDLow(), oldMap->GetId(), oldMap->GetDifficulty(), true);
    }

    // relocate the player to the teleport destination
    Map* newMap = sMapMgr->CreateMap(loc.GetMapId(), GetPlayer());
    // the CanEnter checks are done in TeleporTo but conditions may change
    // while the player is in transit, for example the map may get full
    if (!newMap || !newMap->CanEnter(GetPlayer()))
    {
        sLog->outError("Map %d could not be created for player %d, porting player to homebind", loc.GetMapId(), GetPlayer()->GetGUIDLow());
        GetPlayer()->TeleportTo(GetPlayer()->m_homebindMapId, GetPlayer()->m_homebindX, GetPlayer()->m_homebindY, GetPlayer()->m_homebindZ, GetPlayer()->GetOrientation());
        sLog->outString("TELEPORT BUG HERE #4! ");
        return;
    }

    GetPlayer()->Relocate(loc.GetPositionX(), loc.GetPositionY(), loc.GetPositionZ(), loc.GetOrientation());

    GetPlayer()->ResetMap();
    GetPlayer()->SetMap(newMap);

    GetPlayer()->SendInitialPacketsBeforeAddToMap();
    if (!GetPlayer()->GetMap()->AddPlayerToMap(GetPlayer()))
    {
        sLog->outError("WORLD: failed to teleport player %s (%d) to map %d because of unknown reason!", GetPlayer()->GetName().c_str(), GetPlayer()->GetGUIDLow(), loc.GetMapId());
        GetPlayer()->ResetMap();
        GetPlayer()->SetMap(oldMap);
        GetPlayer()->TeleportTo(GetPlayer()->m_homebindMapId, GetPlayer()->m_homebindX, GetPlayer()->m_homebindY, GetPlayer()->m_homebindZ, GetPlayer()->GetOrientation());
        sLog->outString("TELEPORT BUG HERE #5! ");
        return;
    }

    oldMap->AfterPlayerUnlinkFromMap();

    // pussywizard: transport teleport couldn't teleport us to the same map (some other teleport pending, reqs not met, etc.), but we still have transport set until player moves! clear it if map differs (crashfix)
    if (Transport* t = _player->GetTransport())
        if (!t->IsInMap(_player))
        {
            t->RemovePassenger(_player);
            _player->m_transport = NULL;
            _player->m_movementInfo.transport.Reset();
            _player->m_movementInfo.RemoveMovementFlag(MOVEMENTFLAG_ONTRANSPORT);
        }

    if (!_player->getHostileRefManager().isEmpty())
        _player->getHostileRefManager().deleteReferences(); // pussywizard: multithreading crashfix

    CellCoord pair(Trinity::ComputeCellCoord(GetPlayer()->GetPositionX(), GetPlayer()->GetPositionY()));
    Cell cell(pair);
    if (!GridCoord(cell.GridX(), cell.GridY()).IsCoordValid())
    {
        KickPlayer();
        return;
    }
    newMap->LoadGrid(GetPlayer()->GetPositionX(), GetPlayer()->GetPositionY());

    // pussywizard: player supposed to enter bg map
    if (_player->InBattleground())
    {
        // but landed on another map, cleanup data
        if (!mEntry->IsBattlegroundOrArena())
            _player->SetBattlegroundId(0, BATTLEGROUND_TYPE_NONE, PLAYER_MAX_BATTLEGROUND_QUEUES, false, false, TEAM_NEUTRAL);
        // everything ok
        else if (Battleground* bg = _player->GetBattleground())
        {
            if (_player->IsInvitedForBattlegroundInstance()) // GMs are not invited, so they are not added to participants
                bg->AddPlayer(_player);
        }
    }

    // pussywizard: arena spectator stuff
    {
        if (newMap->IsBattleArena() && ((BattlegroundMap*)newMap)->GetBG() && _player->HasPendingSpectatorForBG(((BattlegroundMap*)newMap)->GetInstanceId()))
        {
            _player->ClearReceivedSpectatorResetFor();
            _player->SetIsSpectator(true);
            ArenaSpectator::SendCommand(_player, "%sENABLE", SPECTATOR_ADDON_PREFIX);
            ((BattlegroundMap*)newMap)->GetBG()->AddSpectator(_player);
            ArenaSpectator::HandleResetCommand(_player);
        }
        else
            _player->SetIsSpectator(false);

        GetPlayer()->SetPendingSpectatorForBG(0);
        timeWhoCommandAllowed = time(NULL) + sWorld->GetNextWhoListUpdateDelaySecs() + 1; // after exiting arena Subscribe will scan for a player and cached data says he is still in arena, so disallow until next update

        if (uint32 inviteInstanceId = _player->GetPendingSpectatorInviteInstanceId())
        {
            if (Battleground* tbg = sBattlegroundMgr->GetBattleground(inviteInstanceId))
                tbg->RemoveToBeTeleported(_player->GetGUID());
            _player->SetPendingSpectatorInviteInstanceId(0);
        }
    }

    // xinef: do this again, player can be teleported inside bg->AddPlayer(_player)!!!!
    CellCoord pair2(Trinity::ComputeCellCoord(GetPlayer()->GetPositionX(), GetPlayer()->GetPositionY()));
    Cell cell2(pair2);
    if (!GridCoord(cell2.GridX(), cell2.GridY()).IsCoordValid())
    {
        KickPlayer();
        return;
    }
    newMap->LoadGrid(GetPlayer()->GetPositionX(), GetPlayer()->GetPositionY());

    GetPlayer()->SendInitialPacketsAfterAddToMap();

    // resurrect character at enter into instance where his corpse exist after add to map
    Corpse* corpse = GetPlayer()->GetCorpse();
    if (corpse && corpse->GetType() != CORPSE_BONES && corpse->GetMapId() == GetPlayer()->GetMapId())
    {
        if (mEntry->IsDungeon())
        {
            GetPlayer()->ResurrectPlayer(0.5f, false);
            GetPlayer()->SpawnCorpseBones();
        }
    }

    bool allowMount = !mEntry->IsDungeon() || mEntry->IsBattlegroundOrArena();
    if (mInstance)
    {
        Difficulty diff = GetPlayer()->GetDifficulty(mEntry->IsRaid());
        if (MapDifficulty const* mapDiff = GetMapDifficultyData(mEntry->MapID, diff))
            if (mapDiff->resetTime)
                if (time_t timeReset = sInstanceSaveMgr->GetResetTimeFor(mEntry->MapID, diff))
                {
                    uint32 timeleft = uint32(timeReset - time(NULL));
                    GetPlayer()->SendInstanceResetWarning(mEntry->MapID, diff, timeleft, true);
                }
        allowMount = mInstance->AllowMount;
    }

    // mount allow check
    if (!allowMount)
        _player->RemoveAurasByType(SPELL_AURA_MOUNTED);

    // update zone immediately, otherwise leave channel will cause crash in mtmap
    uint32 newzone, newarea;
    GetPlayer()->GetZoneAndAreaId(newzone, newarea, true);
    GetPlayer()->UpdateZone(newzone, newarea);

    // honorless target
    if (GetPlayer()->pvpInfo.IsHostile)
        GetPlayer()->CastSpell(GetPlayer(), 2479, true);

    // in friendly area
    else if (GetPlayer()->IsPvP() && !GetPlayer()->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP))
        GetPlayer()->UpdatePvP(false, false);

    // resummon pet
    GetPlayer()->ResummonPetTemporaryUnSummonedIfAny();

    //lets process all delayed operations on successful teleport
    GetPlayer()->ProcessDelayedOperations();
}

void WorldSession::HandleMoveTeleportAck(WorldPacket& recvData)
{
    ;//sLog->outDebug(LOG_FILTER_NETWORKIO, "MSG_MOVE_TELEPORT_ACK");
    uint64 guid;

    recvData.readPackGUID(guid);

    uint32 flags, time;
    recvData >> flags >> time; // unused
    ;//sLog->outStaticDebug("Guid " UI64FMTD, guid);
    ;//sLog->outStaticDebug("Flags %u, time %u", flags, time/IN_MILLISECONDS);

    Player* plMover = _player->m_mover->ToPlayer();

    if (!plMover || !plMover->IsBeingTeleportedNear())
        return;

    if (guid != plMover->GetGUID())
        return;

    plMover->SetSemaphoreTeleportNear(0);

    uint32 old_zone = plMover->GetZoneId();

    WorldLocation const& dest = plMover->GetTeleportDest();
    Position oldPos(*plMover);

    plMover->UpdatePosition(dest, true);

    // xinef: teleport pets if they are not unsummoned
    if (Pet* pet = plMover->GetPet())
    {
        if (!pet->IsWithinDist3d(plMover, plMover->GetMap()->GetVisibilityRange()-5.0f))
            pet->NearTeleportTo(plMover->GetPositionX(), plMover->GetPositionY(), plMover->GetPositionZ(), pet->GetOrientation());
    }

    if (oldPos.GetExactDist2d(plMover) > 100.0f)
    {
        uint32 newzone, newarea;
        plMover->GetZoneAndAreaId(newzone, newarea, true);
        plMover->UpdateZone(newzone, newarea);

        // new zone
        if (old_zone != newzone)
        {
            // honorless target
            if (plMover->pvpInfo.IsHostile)
                plMover->CastSpell(plMover, 2479, true);

            // in friendly area
            else if (plMover->IsPvP() && !plMover->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP))
                plMover->UpdatePvP(false, false);
        }
    }

    // resummon pet
    GetPlayer()->ResummonPetTemporaryUnSummonedIfAny();

    //lets process all delayed operations on successful teleport
    GetPlayer()->ProcessDelayedOperations();

    plMover->GetMotionMaster()->ReinitializeMovement();

    // pussywizard: client forgets about losing control, resend it
    if (plMover->HasUnitState(UNIT_STATE_FLEEING|UNIT_STATE_CONFUSED) || plMover->IsCharmed()) // only in such cases SetClientControl(self, false) is sent
        plMover->SetClientControl(plMover, false, true);
}

void WorldSession::HandleMovementOpcodes(WorldPacket & recvData)
{
    uint16 opcode = recvData.GetOpcode();

    Unit* mover = _player->m_mover;

    ASSERT(mover != NULL);                      // there must always be a mover

    Player* plrMover = mover->GetTypeId() == TYPEID_PLAYER ? (Player*)mover : NULL;
    Vehicle* vehMover = mover->GetVehicleKit();
    if (vehMover)
        if (mover->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED))
            if (Unit* charmer = mover->GetCharmer())
                if (charmer->GetTypeId() == TYPEID_PLAYER)
                    plrMover = (Player*)charmer;

    // ignore, waiting processing in WorldSession::HandleMoveWorldportAckOpcode and WorldSession::HandleMoveTeleportAck
    if (plrMover && plrMover->IsBeingTeleported())
    {
        recvData.rfinish();                     // prevent warnings spam
        return;
    }

    /* extract packet */
    uint64 guid;

    recvData.readPackGUID(guid);

    MovementInfo movementInfo;
    movementInfo.guid = guid;
    ReadMovementInfo(recvData, &movementInfo);

    recvData.rfinish();                         // prevent warnings spam

    // pussywizard: typical check for incomming movement packets
    if (!mover || !mover->IsInWorld() || mover->IsDuringRemoveFromWorld() || guid != mover->GetGUID())
        return;

    if (!movementInfo.pos.IsPositionValid())
    {
        recvData.rfinish();                     // prevent warnings spam
        return;
    }

    if (movementInfo.flags & MOVEMENTFLAG_ONTRANSPORT)
    {
        // T_POS ON VEHICLES!
        if (mover->GetVehicle())
            movementInfo.transport.pos = mover->m_movementInfo.transport.pos;

        // transports size limited
        // (also received at zeppelin leave by some reason with t_* as absolute in continent coordinates, can be safely skipped)
        if (movementInfo.transport.pos.GetPositionX() > 75.0f || movementInfo.transport.pos.GetPositionY() > 75.0f || movementInfo.transport.pos.GetPositionZ() > 75.0f ||
            movementInfo.transport.pos.GetPositionX() < -75.0f || movementInfo.transport.pos.GetPositionY() < -75.0f || movementInfo.transport.pos.GetPositionZ() < -75.0f)
        {
            recvData.rfinish();                   // prevent warnings spam
            return;
        }

        if (!Trinity::IsValidMapCoord(movementInfo.pos.GetPositionX() + movementInfo.transport.pos.GetPositionX(), movementInfo.pos.GetPositionY() + movementInfo.transport.pos.GetPositionY(),
            movementInfo.pos.GetPositionZ() + movementInfo.transport.pos.GetPositionZ(), movementInfo.pos.GetOrientation() + movementInfo.transport.pos.GetOrientation()))
        {
            recvData.rfinish();                   // prevent warnings spam
            return;
        }

        // if we boarded a transport, add us to it
        if (plrMover && !plrMover->m_transport && !plrMover->m_temp_transport)
        {
            if (!plrMover->GetTransport())
            {
                if (Transport* transport = plrMover->GetMap()->GetTransport(movementInfo.transport.guid))
                {
                    plrMover->m_transport = transport;
                    transport->AddPassenger(plrMover);
                }
            }
            else if (plrMover->GetTransport()->GetGUID() != movementInfo.transport.guid)
            {
                bool foundNewTransport = false;
                plrMover->m_transport->RemovePassenger(plrMover);
                if (Transport* transport = plrMover->GetMap()->GetTransport(movementInfo.transport.guid))
                {
                    foundNewTransport = true;
                    plrMover->m_transport = transport;
                    transport->AddPassenger(plrMover);
                }

                if (!foundNewTransport)
                {
                    plrMover->m_transport = NULL;
                    movementInfo.transport.Reset();
                }
            }
        }

        if(plrMover)
            if (!plrMover->m_transport)
                if (Map *tempMap = mover->GetMap())
                    if (GameObject *tempTransport = tempMap->GetGameObject(movementInfo.transport.guid))
                        if (tempTransport->IsTransport())
                            plrMover->m_temp_transport = tempTransport;

        if ((!plrMover && !mover->GetTransport() && !mover->GetVehicle()) || (plrMover && !plrMover->m_vehicle && !plrMover->m_transport && !plrMover->m_temp_transport)) // Not sure if the first part is needed. Just added it for verbosity.
            movementInfo.flags &= ~MOVEMENTFLAG_ONTRANSPORT;
    }
    else if (plrMover && (plrMover->m_transport || plrMover->m_temp_transport)) // if we were on a transport, leave
    {
        if (plrMover->m_transport)
        {
            plrMover->m_transport->RemovePassenger(plrMover);
            plrMover->m_transport = NULL;
        }
        plrMover->m_temp_transport = NULL;
        plrMover->m_transport = NULL;
        movementInfo.transport.Reset();
    }

    if (plrMover && ((movementInfo.flags & MOVEMENTFLAG_SWIMMING) != 0) != plrMover->IsInWater())
    {
        // now client not include swimming flag in case jumping under water
        plrMover->SetInWater(!plrMover->IsInWater() || plrMover->GetBaseMap()->IsUnderWater(movementInfo.pos.GetPositionX(), movementInfo.pos.GetPositionY(), movementInfo.pos.GetPositionZ()));
    }
    
    if (plrMover)
        sAnticheatMgr->StartHackDetection(plrMover, movementInfo, opcode);

    // Dont allow to turn on walking if charming other player
    if (mover->GetGUID() != _player->GetGUID())
        movementInfo.flags &= ~MOVEMENTFLAG_WALKING;

    // Xinef: do not allow to move with UNIT_FLAG_DISABLE_MOVE
    if (mover->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE))
    {
        // Xinef: skip moving packets
        if (movementInfo.HasMovementFlag(MOVEMENTFLAG_MASK_MOVING))
            return;
        movementInfo.pos.Relocate(mover->GetPositionX(), mover->GetPositionY(), mover->GetPositionZ());

        if (mover->GetTypeId() == TYPEID_UNIT)
        {
            movementInfo.transport.guid = mover->m_movementInfo.transport.guid;
            movementInfo.transport.pos.Relocate(mover->m_movementInfo.transport.pos.GetPositionX(), mover->m_movementInfo.transport.pos.GetPositionY(), mover->m_movementInfo.transport.pos.GetPositionZ());
            movementInfo.transport.seat = mover->m_movementInfo.transport.seat;
        }
    }
/////////////////////////////////////////////////////////
    // begin anti cheat
    bool check_passed = true;
    //sLog->outString("AC2-%s > time: %d fall-time: %d | xyzo: %f, %f, %fo(%f) flags[%X] opcode[%s] | transport (xyzo): %f, %f, %fo(%f)", plrMover->GetName(), movementInfo.time, movementInfo.fallTime, movementInfo.pos.m_positionX, movementInfo.pos.m_positionY, movementInfo.pos.m_positionZ, movementInfo.pos.m_orientation, movementInfo.flags, LookupOpcodeName(opcode), movementInfo.transport.pos.m_positionX, movementInfo.transport.pos.m_positionY, movementInfo.transport.pos.m_positionZ, movementInfo.transport.pos.m_orientation);
    //sLog->outString("AC2-%s Transport > GUID: (low)%d - (high)%d", plrMover->GetName(), GUID_LOPART(movementInfo.transport.guid), GUID_HIPART(movementInfo.transport.guid));

    if (plrMover)
    {
        if (Pet* tmpPet = plrMover->GetPet())
            if(!tmpPet->IsInCombat() && !plrMover->IsInCombat())
            {
                tmpPet->GetMotionMaster()->UpdateMotion(10);
            }
        if (World::GetEnableMvAnticheat() && !plrMover->IsGameMaster() && !plrMover->GetCharmerOrOwnerPlayerOrPlayerItself()->IsGameMaster() && !plrMover->GetCharmerOrOwnerPlayerOrPlayerItself()->GetVehicle())
        {
            // calc time deltas
            int32 cClientTimeDelta = 1500;
            if (plrMover->m_anti_LastClientTime != 0)
            {
                cClientTimeDelta = movementInfo.time - plrMover->m_anti_LastClientTime;
                plrMover->m_anti_DeltaClientTime += cClientTimeDelta;
                plrMover->m_anti_LastClientTime = movementInfo.time;
            }
            else
                plrMover->m_anti_LastClientTime = movementInfo.time;

            const uint64 cServerTime = getMSTime();
            uint32 cServerTimeDelta = 1500;
            if (plrMover->m_anti_LastServerTime != 0)
            {
                cServerTimeDelta = cServerTime - plrMover->m_anti_LastServerTime;
                plrMover->m_anti_DeltaServerTime += cServerTimeDelta;
                plrMover->m_anti_LastServerTime = cServerTime;
            }
            else
                plrMover->m_anti_LastServerTime = cServerTime;

            // resync times on client login (first 15 sec for heavy areas)
            if (plrMover->m_anti_DeltaServerTime < 15000 && plrMover->m_anti_DeltaClientTime < 15000)
                plrMover->m_anti_DeltaClientTime = plrMover->m_anti_DeltaServerTime;

            const int32 sync_time = plrMover->m_anti_DeltaClientTime - plrMover->m_anti_DeltaServerTime;
 
            //sLog->outString("AC2-%s Time > cClientTimeDelta: %d, cServerTime: %d | deltaC: %d - deltaS: %d | SyncTime: %d", plrMover->GetName(), cClientTimeDelta, cServerTime, plrMover->m_anti_DeltaClientTime, plrMover->m_anti_DeltaServerTime, sync_time);

            // mistiming checks
            const int32 GetMistimingDelta = abs(int32(World::GetMistimingDelta()));
            if (sync_time > GetMistimingDelta)
            {
                cClientTimeDelta = cServerTimeDelta;
                ++(plrMover->m_anti_MistimingCount);

                const bool bMistimingModulo = plrMover->m_anti_MistimingCount % 50 == 0;

                if (bMistimingModulo)
                {
                    check_passed = false;
                }
                // Tell the player "Sure, you can fly!"
                {
                    WorldPacket data(SMSG_MOVE_SET_CAN_FLY, 12);
                    data << uint64(plrMover->GetGUID());
                    data << uint32(0);
                    SendPacket(&data);
                }
                // Then tell the player "Wait, no, you can't."
                {
                    WorldPacket data(SMSG_MOVE_UNSET_CAN_FLY, 12);
                    data << uint64(plrMover->GetGUID());
                    data << uint32(0);
                    SendPacket(&data);
                }
                //plrMover->FallGround(2);
            }
            // end mistiming checks

            const uint32 curDest = plrMover->m_taxi.GetTaxiDestination(); // check taxi flight
            if (!curDest)
            {
                //sLog->outString("     work      ");
                UnitMoveType move_type;

                // calculating section
                // current speed
                if (movementInfo.flags & MOVEMENTFLAG_FLYING)
                    move_type = movementInfo.flags & MOVEMENTFLAG_BACKWARD ? MOVE_FLIGHT_BACK : MOVE_FLIGHT;
                else if (movementInfo.flags & MOVEMENTFLAG_SWIMMING)
                    move_type = movementInfo.flags & MOVEMENTFLAG_BACKWARD ? MOVE_SWIM_BACK : MOVE_SWIM;
                else if (movementInfo.flags & MOVEMENTFLAG_WALKING)
                    move_type = MOVE_WALK;
                // hmm... in first time after login player has MOVE_SWIMBACK instead MOVE_WALKBACK
                else
                    move_type = movementInfo.flags & MOVEMENTFLAG_BACKWARD ? MOVE_SWIM_BACK : MOVE_RUN;

                const float current_speed = mover->GetSpeed(move_type);
                // end current speed

                // movement distance
                const float delta_x = plrMover->m_transport || plrMover->m_temp_transport ? 0 : plrMover->GetPositionX() - movementInfo.pos.GetPositionX();
                const float delta_y = plrMover->m_transport || plrMover->m_temp_transport ? 0 : plrMover->GetPositionY() - movementInfo.pos.GetPositionY();
                const float delta_z = plrMover->m_transport || plrMover->m_temp_transport ? 0 : plrMover->GetPositionZ() - movementInfo.pos.GetPositionZ();
                const float real_delta = plrMover->m_transport || plrMover->m_temp_transport ? 0 : pow(delta_x, 2) + pow(delta_y, 2);
                // end movement distance

                const bool no_fly_auras = !(plrMover->HasAuraType(SPELL_AURA_FLY) || plrMover->HasAuraType(SPELL_AURA_MOD_INCREASE_VEHICLE_FLIGHT_SPEED)
                    || plrMover->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED) || plrMover->HasAuraType(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED)
                    || plrMover->HasAuraType(SPELL_AURA_MOD_MOUNTED_FLIGHT_SPEED_ALWAYS) || plrMover->HasAuraType(SPELL_AURA_MOD_FLIGHT_SPEED_NOT_STACK));
                const bool no_fly_flags = (movementInfo.flags & (MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_FLYING)) == 0;

                const bool no_swim_flags = (movementInfo.flags & MOVEMENTFLAG_SWIMMING) == 0;
                const bool no_swim_in_water = !mover->IsInWater();
                const bool no_swim_above_water = movementInfo.pos.GetPositionZ() - 7.0f >= mover->GetBaseMap()->GetWaterLevel(movementInfo.pos.GetPositionX(), movementInfo.pos.GetPositionY());
                const bool no_swim_water = no_swim_in_water && no_swim_above_water;

                const bool no_waterwalk_flags = (movementInfo.flags & MOVEMENTFLAG_WATERWALKING) == 0;
                const bool no_waterwalk_auras = !(plrMover->HasAuraType(SPELL_AURA_WATER_WALK) || plrMover->HasAuraType(SPELL_AURA_GHOST));

                if (cClientTimeDelta < 0)
                    cClientTimeDelta = 0;
                const float time_delta = cClientTimeDelta < 1500 ? float(cClientTimeDelta) / 1000.0f : 1.5f; // normalize time - 1.5 second allowed for heavy loaded server

                bool areMapsOK = true;
                if (plrMover->GetMapId() != 548 || plrMover->GetMapId() != 369)
                    areMapsOK = false;

                const float tg_z = (real_delta != 0 && no_fly_auras && no_swim_flags) ? (pow(delta_z, 2) / real_delta) : -99999; // movement distance tangents

                if (current_speed < plrMover->m_anti_Last_HSpeed && plrMover->m_anti_LastSpeedChangeTime == 0)
                    plrMover->m_anti_LastSpeedChangeTime = movementInfo.time + uint32(floor(((plrMover->m_anti_Last_HSpeed / current_speed) * 1500)) + 100); // 100ms above for random fluctuation

                const float allowed_delta = plrMover->m_transport || plrMover->m_temp_transport ? 2 : // movement distance allowed delta
                    pow(std::max(current_speed, plrMover->m_anti_Last_HSpeed) * time_delta, 2)
                    + 2                                                                             // minimum allowed delta
                    + (tg_z > 2.2 ? pow(delta_z, 2) / 2.37f : 0);                                     // mountain fall allowed delta

                if (movementInfo.time > plrMover->m_anti_LastSpeedChangeTime)
                {
                    plrMover->m_anti_Last_HSpeed = current_speed;                                    // store current speed
                    plrMover->m_anti_Last_VSpeed = -2.3f;
                    plrMover->m_anti_LastSpeedChangeTime = 0;
                }
                // end calculating section

                // AntiGravity (thanks to Meekro)
                const float JumpHeight = plrMover->m_anti_JumpBaseZ - movementInfo.pos.GetPositionZ();
                if (areMapsOK && !plrMover->HasAura(51690) && !plrMover->IsGameMaster() && no_fly_auras && no_swim_in_water && plrMover->m_anti_JumpBaseZ != 0 && JumpHeight < plrMover->m_anti_Last_VSpeed && !plrMover->m_transport && !plrMover->HasUnitState(UNIT_STATE_CHARGING)
                    && !plrMover->isFeared() && !plrMover->IsInWater() && !plrMover->IsUnderWater())
                {
                    //sLog->outString("     555      ");
                    check_passed = false;

                    // Tell the player "Sure, you can fly!"
                    {
                        WorldPacket data(SMSG_MOVE_SET_CAN_FLY, 12);
                        data << uint64(plrMover->GetGUID());
                        data << uint32(0);
                        SendPacket(&data);
                    }
                    // Then tell the player "Wait, no, you can't."
                    {
                        WorldPacket data(SMSG_MOVE_UNSET_CAN_FLY, 12);
                        data << uint64(plrMover->GetGUID());
                        data << uint32(0);
                        SendPacket(&data);
                    }
                }

                // multi jump checks
                if (opcode == MSG_MOVE_JUMP)
                {
                    //sLog->outString("     444      ");
                    if (areMapsOK && !plrMover->IsGameMaster() && (no_fly_auras && no_swim_water) && !plrMover->HasAura(51690) && !plrMover->m_transport && !plrMover->HasUnitState(UNIT_STATE_CHARGING) && !plrMover->isFeared() && !plrMover->IsInWater() && !plrMover->IsUnderWater())
                    {
                        if (plrMover->m_anti_JumpCount >= 1)
                        {
                            // don't process new jump packet
                            check_passed = false;

                            // Tell the player "Sure, you can fly!"
                            {
                                WorldPacket data(SMSG_MOVE_SET_CAN_FLY, 12);
                                data << uint64(plrMover->GetGUID());
                                data << uint32(0);
                                SendPacket(&data);
                            }
                            // Then tell the player "Wait, no, you can't."
                            {
                                WorldPacket data(SMSG_MOVE_UNSET_CAN_FLY, 12);
                                data << uint64(plrMover->GetGUID());
                                data << uint32(0);
                                SendPacket(&data);
                            }
                            //plrMover->FallGround(2);
                            plrMover->m_anti_JumpCount = 0;
                        }
                        else
                        {
                            plrMover->m_anti_JumpCount += 1;
                            plrMover->m_anti_JumpBaseZ = movementInfo.pos.GetPositionZ();
                        }
                    }
                    else
                        plrMover->m_anti_JumpCount = 0;
                }

                // PB: "Freeze Z coord check"
                if (plrMover)
                    if (!plrMover->IsFlying() && !plrMover->IsGameMaster() && !plrMover->isFeared() && no_fly_auras && no_fly_flags && !plrMover->IsInWater() && !plrMover->IsFalling() && !plrMover->m_transport &&
                        !plrMover->HasUnitState(UNIT_STATE_CHARGING) && areMapsOK && !plrMover->IsInWater() && !plrMover->IsUnderWater() && !plrMover->HasAura(51690))
                    {
                        //sLog->outString("      111     ");
                        float playerZ = plrMover->GetPositionZ();
                        float floorZ = plrMover->GetMap()->GetHeight(plrMover->GetPhaseMask(), plrMover->GetPositionX(), plrMover->GetPositionY(), plrMover->GetPositionZ());
                        float absolute_pos = fabs(playerZ - floorZ);
                        if (floorZ < -1000 && floorZ < 1000)
                            if (absolute_pos >= 4.0f && absolute_pos <= 1000) // 4 could be less? higher than 1k is a missing floorZ
                            {
                                ++(plrMover->m_anti_FreezeZ_Count);
                                sLog->outString("[FreezeZ Failed] AreaID: %u | playerZ: %f | mapZ: %f | absolute_pos: %f", plrMover->GetAreaId(), playerZ, floorZ, absolute_pos);
                            }
                        if (_player->GetSession()->GetSecurity() <= 0)
                        {
                            if (plrMover->m_anti_FreezeZ_Count >= 15) // if they rotate with rmb they will do insane amount of checks so >=15 should be ok i guess?
                                plrMover->GetSession()->KickPlayer();
                        }
                    }
                // PB: custom fly / under map checks UNIT_STATE_CHARGING
                if (plrMover)
                    if ((no_fly_auras && !no_fly_flags) && !plrMover->IsGameMaster() && !plrMover->IsFalling() &&
                        !plrMover->m_transport && !plrMover->isFeared() && !plrMover->HasUnitState(UNIT_STATE_CHARGING) && areMapsOK && !plrMover->IsInWater() && !plrMover->IsUnderWater() && !plrMover->IsGameMaster()
                         && !plrMover->HasAura(51690))
                    {
                        //sLog->outString("      222     ");
                        float playerZ = plrMover->GetPositionZ();
                        float floorZ = plrMover->GetMap()->GetHeight(plrMover->GetPhaseMask(), plrMover->GetPositionX(), plrMover->GetPositionY(), plrMover->GetPositionZ());
                        if (floorZ < -1000 && floorZ < 1000)
                        if (fabs(playerZ - floorZ) < 7.0f) // fly hack
                        {
                            //sLog->outString("fabs: %f mapZ: %f",fabs(playerZ - mapZ), mapZ);
                            WorldPacket set_fly(SMSG_MOVE_SET_CAN_FLY, 12);
                            set_fly << uint64(plrMover->GetGUID());
                            set_fly << uint32(0);
                            SendPacket(&set_fly);

                            WorldPacket unset_fly(SMSG_MOVE_UNSET_CAN_FLY, 12);
                            unset_fly << uint64(plrMover->GetGUID());
                            unset_fly << uint32(0);
                            SendPacket(&unset_fly);

                            check_passed = false;
                        }
                        //if (fabs(playerZ - mapZ) < -2.0f) // fell underground
                            //plrMover->TeleportTo(plrMover->GetMapId(), plrMover->GetPositionX(), plrMover->GetPositionY(), mapZ + 1.0f, plrMover->GetOrientation());
                    }

                // speed and teleport hack checks
                if (areMapsOK && !plrMover->IsGameMaster() && (real_delta > (allowed_delta * 2)) && !plrMover->m_transport && !plrMover->HasUnitState(UNIT_STATE_CHARGING) && !plrMover->isFeared()  && !plrMover->HasAura(51690)
                    && !plrMover->IsInWater() && !plrMover->IsUnderWater())
                    check_passed = false;

                // mountain hack checks // 1.56f (delta_z < GetPlayer()->m_anti_Last_VSpeed))
                if (areMapsOK && !plrMover->IsGameMaster() && (delta_z < plrMover->m_anti_Last_VSpeed && plrMover->m_anti_JumpCount == 0 && tg_z > 2.37f) && !plrMover->m_transport && !plrMover->HasUnitState(UNIT_STATE_CHARGING) && !plrMover->isFeared()
                    && !plrMover->IsInWater() && !plrMover->IsUnderWater())
                    check_passed = false;
                
                // Fly hack checks
                if (areMapsOK && !plrMover->IsGameMaster() && (no_fly_auras && !no_fly_flags) && !plrMover->m_transport && !plrMover->HasUnitState(UNIT_STATE_CHARGING) && !plrMover->isFeared()  && !plrMover->HasAura(51690)
                    && !plrMover->IsInWater() && !plrMover->IsUnderWater())
                {
                    //sLog->outString("     333      ");
                    check_passed = false;
                    // Tell the player "Sure, you can fly!"
                    {
                        WorldPacket data(SMSG_MOVE_SET_CAN_FLY, 12);
                        data << uint64(plrMover->GetGUID());
                        data << uint32(0);
                        SendPacket(&data);
                    }
                    // Then tell the player "Wait, no, you can't."
                    {
                        WorldPacket data(SMSG_MOVE_UNSET_CAN_FLY, 12);
                        data << uint64(plrMover->GetGUID());
                        data << uint32(0);
                        SendPacket(&data);
                    }
                }

                // Waterwalk checks
                if (areMapsOK && !plrMover->IsGameMaster() && no_waterwalk_auras && !no_waterwalk_flags && !plrMover->IsInWater() && !plrMover->IsUnderWater())
                {
                    check_passed = false;
                    // Tell the player "Sure, you can fly!"
                    {
                        WorldPacket data(SMSG_MOVE_SET_CAN_FLY, 12);
                        data << uint64(plrMover->GetGUID());
                        data << uint32(0);
                        SendPacket(&data);
                    }
                    // Then tell the player "Wait, no, you can't."
                    {
                        WorldPacket data(SMSG_MOVE_UNSET_CAN_FLY, 12);
                        data << uint64(plrMover->GetGUID());
                        data << uint32(0);
                        SendPacket(&data);
                    }
                }

                // Teleport To Plane checks
                if (areMapsOK && !plrMover->IsGameMaster() && no_swim_in_water && movementInfo.pos.GetPositionZ() < 0.0001f && movementInfo.pos.GetPositionZ() > -0.0001f)
                {
                    if (const Map *map = plrMover->GetMap())
                    {
                        float plane_z = map->GetHeight(movementInfo.pos.GetPositionX(), movementInfo.pos.GetPositionY(), MAX_HEIGHT) - movementInfo.pos.GetPositionZ();
                        plane_z = (plane_z < -500.0f) ? 0.0f : plane_z; // check holes in height map
                        if (plane_z > 0.1f || plane_z < -0.1f)
                        {
                            ++(plrMover->m_anti_TeleToPlane_Count);
                            check_passed = false;
                        }
                    }
                }
                else
                    plrMover->m_anti_TeleToPlane_Count = 0;
            }
        }
    }
     /* process position-change */
    if (check_passed)
    {
        WorldPacket data(opcode, recvData.size());
        movementInfo.time = getMSTime();

        movementInfo.guid = mover->GetGUID();
        WriteMovementInfo(&data, &movementInfo);
        mover->SendMessageToSet(&data, _player);

        mover->m_movementInfo = movementInfo;

        // this is almost never true (pussywizard: only one packet when entering vehicle), normally use mover->IsVehicle()
        if (mover->GetVehicle())
        {
            mover->SetOrientation(movementInfo.pos.GetOrientation());
            mover->UpdatePosition(movementInfo.pos);
            return;
        }

        // pussywizard: previously always mover->UpdatePosition(movementInfo.pos);
        if (movementInfo.flags & MOVEMENTFLAG_ONTRANSPORT && mover->GetTransport())
        {
            float x, y, z, o;
            movementInfo.transport.pos.GetPosition(x, y, z, o);
            mover->GetTransport()->CalculatePassengerPosition(x, y, z, &o);
            mover->UpdatePosition(x, y, z, o);
        }
        else
            mover->UpdatePosition(movementInfo.pos);

        // fall damage generation (ignore in flight case that can be triggered also at lags in moment teleportation to another map).
        // Xinef: moved it here, previously StopMoving function called when player died relocated him to last saved coordinates (which were in air)
        if (opcode == MSG_MOVE_FALL_LAND && plrMover && !plrMover->IsInFlight() && (!plrMover->GetTransport() || plrMover->GetTransport()->IsStaticTransport()))
        {
            // movement anticheat
            plrMover->m_anti_JumpCount = 0;
            plrMover->m_anti_JumpBaseZ = 0;
            if (!vehMover)
                plrMover->HandleFall(movementInfo);
        }
        // Xinef: interrupt parachutes upon falling or landing in water
        if (opcode == MSG_MOVE_FALL_LAND || opcode == MSG_MOVE_START_SWIM)
            mover->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_LANDING); // Parachutes
        
        if (plrMover && !vehMover)                               // nothing is charmed, or player charmed
        {
            if (plrMover->IsSitState() && (movementInfo.flags & (MOVEMENTFLAG_MASK_MOVING | MOVEMENTFLAG_MASK_TURNING)))
                plrMover->SetStandState(UNIT_STAND_STATE_STAND);

            plrMover->UpdateFallInformationIfNeed(movementInfo, opcode);

            float underMapValueZ;

            switch (plrMover->GetMapId())
             {
            case 617: underMapValueZ = 3.0f; break; // Dalaran Sewers
            case 618: underMapValueZ = 28.0f; break; // Ring of Valor
            default: underMapValueZ = -500.0f; break;
            }

            if (movementInfo.pos.GetPositionZ() < underMapValueZ)
                if (!plrMover->GetBattleground() || !plrMover->GetBattleground()->HandlePlayerUnderMap(_player))
                 {
                     if (plrMover->IsAlive())
                    {
                        plrMover->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_IS_OUT_OF_BOUNDS);
                        plrMover->EnvironmentalDamage(DAMAGE_FALL_TO_VOID, GetPlayer()->GetMaxHealth());
                        // player can be alive if GM
                        if (plrMover->IsAlive())
                            plrMover->KillPlayer();
                    }
                    plrMover->StopMovingOnCurrentPos(); // pussywizard: moving corpse can't release spirit
                 }
            // movement anticheat
            if (plrMover->m_anti_AlarmCount > 0)
            {
                //TC_LOG_WARN("cheat", "AC2-%s produce %d anticheat alarms.", plrMover->GetName().c_str(), plrMover->m_anti_AlarmCount);
                plrMover->m_anti_AlarmCount = 0;
             }
            // end movement anticheat
        }
    }
    else if (plrMover)
    {
        if (plrMover->m_transport)
        {
            plrMover->m_transport->RemovePassenger(plrMover);
            plrMover->m_transport = NULL;
        }
        plrMover->m_temp_transport = NULL;
        ++(plrMover->m_anti_AlarmCount);
        WorldPacket data;
        plrMover->SetUnitMovementFlags(0);
        plrMover->SendTeleportAckPacket();
        plrMover->BuildHeartBeatMsg(&data);
        plrMover->SendMessageToSet(&data, true);
     }
}

void WorldSession::HandleForceSpeedChangeAck(WorldPacket &recvData)
{
    uint32 opcode = recvData.GetOpcode();
    ;//sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd %s (%u, 0x%X) opcode", LookupOpcodeName(opcode), opcode, opcode);

    /* extract packet */
    uint64 guid;
    uint32 unk1;
    float  newspeed;

    recvData.readPackGUID(guid);

    // pussywizard: special check, only player mover allowed here
    if (guid != _player->m_mover->GetGUID() || guid != _player->GetGUID())
    {
        recvData.rfinish(); // prevent warnings spam
        return;
    }

    // continue parse packet

    recvData >> unk1;                                      // counter or moveEvent

    MovementInfo movementInfo;
    movementInfo.guid = guid;
    ReadMovementInfo(recvData, &movementInfo);

    recvData >> newspeed;
    /*----------------*/

    // client ACK send one packet for mounted/run case and need skip all except last from its
    // in other cases anti-cheat check can be fail in false case
    UnitMoveType move_type;
    UnitMoveType force_move_type;

    static char const* move_type_name[MAX_MOVE_TYPE] = {  "Walk", "Run", "RunBack", "Swim", "SwimBack", "TurnRate", "Flight", "FlightBack", "PitchRate" };

    switch (opcode)
    {
        case CMSG_FORCE_WALK_SPEED_CHANGE_ACK:          move_type = MOVE_WALK;          force_move_type = MOVE_WALK;        break;
        case CMSG_FORCE_RUN_SPEED_CHANGE_ACK:           move_type = MOVE_RUN;           force_move_type = MOVE_RUN;         break;
        case CMSG_FORCE_RUN_BACK_SPEED_CHANGE_ACK:      move_type = MOVE_RUN_BACK;      force_move_type = MOVE_RUN_BACK;    break;
        case CMSG_FORCE_SWIM_SPEED_CHANGE_ACK:          move_type = MOVE_SWIM;          force_move_type = MOVE_SWIM;        break;
        case CMSG_FORCE_SWIM_BACK_SPEED_CHANGE_ACK:     move_type = MOVE_SWIM_BACK;     force_move_type = MOVE_SWIM_BACK;   break;
        case CMSG_FORCE_TURN_RATE_CHANGE_ACK:           move_type = MOVE_TURN_RATE;     force_move_type = MOVE_TURN_RATE;   break;
        case CMSG_FORCE_FLIGHT_SPEED_CHANGE_ACK:        move_type = MOVE_FLIGHT;        force_move_type = MOVE_FLIGHT;      break;
        case CMSG_FORCE_FLIGHT_BACK_SPEED_CHANGE_ACK:   move_type = MOVE_FLIGHT_BACK;   force_move_type = MOVE_FLIGHT_BACK; break;
        case CMSG_FORCE_PITCH_RATE_CHANGE_ACK:          move_type = MOVE_PITCH_RATE;    force_move_type = MOVE_PITCH_RATE;  break;
        default:
            sLog->outError("WorldSession::HandleForceSpeedChangeAck: Unknown move type opcode: %u", opcode);
            return;
    }

    // skip all forced speed changes except last and unexpected
    // in run/mounted case used one ACK and it must be skipped.m_forced_speed_changes[MOVE_RUN} store both.
    if (_player->m_forced_speed_changes[force_move_type] > 0)
    {
        --_player->m_forced_speed_changes[force_move_type];
        if (_player->m_forced_speed_changes[force_move_type] > 0)
            return;
    }

    if (!_player->GetTransport() && fabs(_player->GetSpeed(move_type) - newspeed) > 0.01f)
    {
        if (_player->GetSpeed(move_type) > newspeed)         // must be greater - just correct
        {
            sLog->outError("%sSpeedChange player %s is NOT correct (must be %f instead %f), force set to correct value",
                move_type_name[move_type], _player->GetName().c_str(), _player->GetSpeed(move_type), newspeed);
            _player->SetSpeed(move_type, _player->GetSpeedRate(move_type), true);
        }
        else                                                // must be lesser - cheating
        {
            if (_player->GetSession()->GetSecurity() <= 0)
            {
                sLog->outBasic("Player %s from account id %u kicked for incorrect speed (must be %f instead %f)",
                    _player->GetName().c_str(), GetAccountId(), _player->GetSpeed(move_type), newspeed);
                KickPlayer();
            }
        }
    }
}

void WorldSession::HandleSetActiveMoverOpcode(WorldPacket &recvData)
{
    ;//sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd CMSG_SET_ACTIVE_MOVER");

    uint64 guid;
    recvData >> guid;

    if (GetPlayer()->IsInWorld() && _player->m_mover && _player->m_mover->IsInWorld())
    {
        if (_player->m_mover->GetGUID() != guid)
            sLog->outError("HandleSetActiveMoverOpcode: incorrect mover guid: mover is " UI64FMTD " (%s - Entry: %u) and should be " UI64FMTD, guid, GetLogNameForGuid(guid), GUID_ENPART(guid), _player->m_mover->GetGUID());
    }
}

void WorldSession::HandleMoveNotActiveMover(WorldPacket &recvData)
{
    ;//sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd CMSG_MOVE_NOT_ACTIVE_MOVER");

    uint64 old_mover_guid;
    recvData.readPackGUID(old_mover_guid);

    // pussywizard: typical check for incomming movement packets
    if (!_player->m_mover || !_player->m_mover->IsInWorld() || _player->m_mover->IsDuringRemoveFromWorld() || old_mover_guid != _player->m_mover->GetGUID())
    {
        recvData.rfinish(); // prevent warnings spam
        return;
    }

    MovementInfo mi;
    mi.guid = old_mover_guid;
    ReadMovementInfo(recvData, &mi);

    _player->m_mover->m_movementInfo = mi;
}

void WorldSession::HandleMountSpecialAnimOpcode(WorldPacket& /*recvData*/)
{
    WorldPacket data(SMSG_MOUNTSPECIAL_ANIM, 8);
    data << uint64(GetPlayer()->GetGUID());

    GetPlayer()->SendMessageToSet(&data, false);
}

void WorldSession::HandleMoveKnockBackAck(WorldPacket & recvData)
{
    ;//sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_MOVE_KNOCK_BACK_ACK");

    uint64 guid;
    recvData.readPackGUID(guid);

    // pussywizard: typical check for incomming movement packets
    if (!_player->m_mover || !_player->m_mover->IsInWorld() || _player->m_mover->IsDuringRemoveFromWorld() || guid != _player->m_mover->GetGUID())
    {
        recvData.rfinish(); // prevent warnings spam
        return;
    }

    recvData.read_skip<uint32>();                          // unk

    MovementInfo movementInfo;
    movementInfo.guid = guid;
    ReadMovementInfo(recvData, &movementInfo);

    
    // Save movement flags
    _player->SetUnitMovementFlags(movementInfo.flags);
    #ifdef ANTICHEAT_DEBUG
    //TC_LOG_WARN("cheat", "%s CMSG_MOVE_KNOCK_BACK_ACK: time: %d, fall time: %d | xyzo: %f,%f,%fo(%f) flags[%X] Vspeed: %f, Hspeed: %f", GetPlayer()->GetName(), movementInfo.time, movementInfo.fallTime, movementInfo.pos.m_positionX, movementInfo.pos.m_positionY, movementInfo.pos.m_positionZ, movementInfo.pos.m_orientation, movementInfo.flags, movementInfo.jump.zspeed, movementInfo.jump.xyspeed);
    #endif
    
    _player->m_mover->m_movementInfo = movementInfo;

    _player->m_anti_Last_HSpeed = movementInfo.jump.xyspeed;
    _player->m_anti_Last_VSpeed = movementInfo.jump.zspeed < 3.2f ? movementInfo.jump.zspeed - 1.0f : 3.2f;

    const uint32 dt = (_player->m_anti_Last_VSpeed < 0) ? int(ceil(_player->m_anti_Last_VSpeed/-25)*1000) : int(ceil(_player->m_anti_Last_VSpeed/25)*1000);
    _player->m_anti_LastSpeedChangeTime = movementInfo.time + dt + 1000;
    
    WorldPacket data(MSG_MOVE_KNOCK_BACK, 66);
    data.appendPackGUID(guid);
    _player->m_mover->BuildMovementPacket(&data);

    // knockback specific info
    data << movementInfo.jump.sinAngle;
    data << movementInfo.jump.cosAngle;
    data << movementInfo.jump.xyspeed;
    data << movementInfo.jump.zspeed;

    _player->SendMessageToSet(&data, false);
}

void WorldSession::HandleMoveHoverAck(WorldPacket& recvData)
{
    ;//sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_MOVE_HOVER_ACK");

    uint64 guid;                                            // guid - unused
    recvData.readPackGUID(guid);

    recvData.read_skip<uint32>();                          // unk

    MovementInfo movementInfo;
    movementInfo.guid = guid;
    ReadMovementInfo(recvData, &movementInfo);

    recvData.read_skip<uint32>();                          // unk2
}

void WorldSession::HandleMoveWaterWalkAck(WorldPacket& recvData)
{
    ;//sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_MOVE_WATER_WALK_ACK");

    uint64 guid;                                            // guid - unused
    recvData.readPackGUID(guid);

    recvData.read_skip<uint32>();                          // unk

    MovementInfo movementInfo;
    movementInfo.guid = guid;
    ReadMovementInfo(recvData, &movementInfo);

    recvData.read_skip<uint32>();                          // unk2
}

void WorldSession::HandleSummonResponseOpcode(WorldPacket& recvData)
{
    if (!_player->IsAlive() || _player->IsInCombat())
        return;

    uint64 summoner_guid;
    bool agree;
    recvData >> summoner_guid;
    recvData >> agree;

    if (agree && _player->IsSummonAsSpectator())
    {
        ChatHandler chc(this);
        if (Player* summoner = ObjectAccessor::FindPlayer(summoner_guid))
            ArenaSpectator::HandleSpectatorSpectateCommand(&chc, summoner->GetName().c_str());
        else
            chc.PSendSysMessage("Requested player not found.");

        agree = false;
    }
    _player->SetSummonAsSpectator(false);
    _player->SummonIfPossible(agree);
}
