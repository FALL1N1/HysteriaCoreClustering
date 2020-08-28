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
//Basic headers
#include "WaypointMovementGenerator.h"
//Extended headers
#include "ObjectMgr.h"
#include "Transport.h"
//Flightmaster grid preloading
#include "MapManager.h"
//Creature-specific headers
#include "Creature.h"
#include "CreatureAI.h"
#include "CreatureGroups.h"
//Player-specific
#include "Player.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"

void WaypointMovementGenerator<Creature>::LoadPath(Creature* creature)
{
    if (!path_id)
        path_id = creature->GetWaypointPath();

    i_path = sWaypointMgr->GetPath(path_id);

    if (!i_path)
    {
        // No path id found for entry
        //TC_LOG_ERROR("sql.sql", "WaypointMovementGenerator::LoadPath: creature %s (Entry: %u GUID: %u DB GUID: %u) doesn't have waypoint path id: %u", creature->GetName().c_str(), creature->GetEntry(), creature->GetGUID().GetCounter(), creature->GetSpawnId(), path_id);
        return;
    }

    if (!Stopped())
        StartMoveNow(creature);
}

void WaypointMovementGenerator<Creature>::DoInitialize(Creature* creature)
{
    LoadPath(creature);
    //creature->AddUnitState(UNIT_STATE_ROAMING|UNIT_STATE_ROAMING_MOVE);
}

void WaypointMovementGenerator<Creature>::DoFinalize(Creature* creature)
{
    creature->ClearUnitState(UNIT_STATE_ROAMING|UNIT_STATE_ROAMING_MOVE);
    creature->SetWalk(false);
}

void WaypointMovementGenerator<Creature>::DoReset(Creature* creature)
{
    creature->AddUnitState(UNIT_STATE_ROAMING|UNIT_STATE_ROAMING_MOVE);
    if (!Stopped())
        StartMoveNow(creature);
}

void WaypointMovementGenerator<Creature>::OnArrived(Creature* creature)
{
    if (!i_path || i_path->empty())
        return;

    WaypointData const waypoint = i_path->at(i_currentNode);
    if (waypoint.delay)
    {
        creature->ClearUnitState(UNIT_STATE_ROAMING_MOVE);
        Stop(waypoint.delay);
    }

    if (waypoint.event_id && urand(0, 99) < waypoint.event_chance)
    {
        //TC_LOG_DEBUG("maps.script", "Creature movement start script %u at point %u for %s.", i_path->at(i_currentNode)->event_id, i_currentNode, creature->GetGUID().ToString().c_str());
        creature->ClearUnitState(UNIT_STATE_ROAMING_MOVE);
        creature->GetMap()->ScriptsStart(sWaypointScripts, waypoint.event_id, creature, NULL);
    }

    // Inform script
    MovementInform(creature);
    creature->UpdateWaypointID(i_currentNode);

    creature->SetWalk(waypoint.move_type != WAYPOINT_MOVE_TYPE_RUN);
}

void WaypointMovementGenerator<Creature>::FormationMove(Creature* creature)
{
    bool transportPath = false;

    if (!creature->GetTransGUID())
        transportPath = true;

    WaypointData const waypoint = i_path->at(i_currentNode);

    Movement::Location formationDest(waypoint.x, waypoint.y, waypoint.z, 0.0f);

    //! If creature is on transport, we assume waypoints set in DB are already transport offsets
    if (transportPath)
    {
        if (TransportBase* trans = creature->GetDirectTransport())
            trans->CalculatePassengerPosition(formationDest.x, formationDest.y, formationDest.z, &formationDest.orientation);
    }

    // Call for creature group update
    if (creature->GetFormation() && creature->GetFormation()->getLeader() == creature)
        creature->GetFormation()->LeaderMoveTo(formationDest.x, formationDest.y, formationDest.z, false); // @todo: could be true? (run)
}

bool WaypointMovementGenerator<Creature>::StartMove(Creature* creature)
{
    if (!creature || !creature->IsAlive())
        return false;

    if (!i_path || i_path->empty())
        return false;

    bool transportPath = false;
    if (creature->HasUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT) && !creature->GetTransGUID())
        transportPath = true;

    if (IsArrivalDone)
    {
        if ((i_currentNode == i_path->size() - 1) && !repeating) // If that's our last waypoint
        {
            WaypointData const waypoint = i_path->at(i_currentNode);

            float x = waypoint.x;
            float y = waypoint.y;
            float z = waypoint.z;
            float o = creature->GetOrientation();

            if (!transportPath)
                creature->SetHomePosition(x, y, z, o);
            else
            {
                if (Transport* trans = creature->GetTransport())
                {
                    o -= trans->GetOrientation();
                    creature->SetTransportHomePosition(x, y, z, o);
                    trans->CalculatePassengerPosition(x, y, z, &o);
                    creature->SetHomePosition(x, y, z, o);
                }
                else
                    transportPath = false;
                // else if (vehicle) - this should never happen, vehicle offsets are const
            }
            
            return false;
        }

        i_currentNode = (i_currentNode + 1) % i_path->size();
    }

    float finalOrient = 0.0f;
    uint8 finalMove = WAYPOINT_MOVE_TYPE_WALK;
    int32 currentMaxPoints = i_path->size();
    bool pathLengthValid = false;

    Movement::PointsArray pathing;
    while (!pathLengthValid)
    {
        pathing.clear();

        pathing.push_back(G3D::Vector3(creature->GetPositionX(), creature->GetPositionY(), creature->GetPositionZ()));

        for (int32 i = i_currentNode; i < currentMaxPoints; ++i)
        {
            WaypointData const& waypoint = i_path->at(i);

            pathing.push_back(G3D::Vector3(waypoint.x, waypoint.y, waypoint.z));

            finalOrient = waypoint.orientation;
            finalMove = waypoint.move_type;

            if (waypoint.delay)
                break;
        }

        pathLengthValid = true;

        if (pathing.size() > 2)
        {
            G3D::Vector3 middle = (pathing[0] + pathing[pathing.size() - 1]) / 2.f;

            for (uint32 i = 1; i < pathing.size(); ++i)
            {
                G3D::Vector3 offset = middle - pathing[i];
                if (std::fabs(offset.x) >= 0xFF || std::fabs(offset.y) >= 0xFF || std::fabs(offset.z) >= 0x7F)
                {
                    --currentMaxPoints;
                    // if currentMaxPoints <= i_currentNode, then we have no more valid points in path - return immediately
                    if (currentMaxPoints < 0 || currentMaxPoints <= int32(i_currentNode))
                    {
                        sLog->outError("WaypointMovementGenerator::StartMove: creature %s (%u) has invalid point (%u), which is too far away from other points in path. Creature stops pathing!",
                            creature->GetName().c_str(), creature->GetGUID(), pathing[i]);
                        return false;
                    }

                    pathLengthValid = false;
                    break;
                }
            }
        }
    }

    // if we have only 1 point, only current position, we shall return
    if (pathing.size() < 2)
        return false;

    IsArrivalDone = false;
    i_recalculateSpeed = false;

    creature->AddUnitState(UNIT_STATE_ROAMING_MOVE);

    Movement::MoveSplineInit init(creature);
    Movement::Location formationDest(i_path->at(i_currentNode).x, i_path->at(i_currentNode).y, i_path->at(i_currentNode).z, 0.0f);
    init.SetSmooth();

    //! If creature is on transport, we assume waypoints set in DB are already transport offsets
    if (transportPath)
    {
        init.DisableTransportPathTransformations();
        if (TransportBase* trans = creature->GetDirectTransport())
            trans->CalculatePassengerPosition(formationDest.x, formationDest.y, formationDest.z, &formationDest.orientation);
    }

    init.MovebyPath(pathing, i_currentNode);
    
    switch (finalMove)
    {
        case WAYPOINT_MOVE_TYPE_LAND:
            init.SetAnimation(Movement::ToGround);
            break;
        case WAYPOINT_MOVE_TYPE_TAKEOFF:
            init.SetAnimation(Movement::ToFly);
            break;
        case WAYPOINT_MOVE_TYPE_RUN:
            init.SetWalk(false);
            break;
        case WAYPOINT_MOVE_TYPE_WALK:
            init.SetWalk(true);
            break;
    }
    
    if (finalOrient != 0.0f)
        init.SetFacing(finalOrient);

    init.Launch();

    //Call for creature group update
    if (creature->GetFormation() && creature->GetFormation()->getLeader() == creature)
        creature->GetFormation()->LeaderMoveTo(formationDest.x, formationDest.y, formationDest.z, finalMove);

    return true;
}

bool WaypointMovementGenerator<Creature>::DoUpdate(Creature* creature, uint32 diff)
{
    // unnecesary? 
    if (!creature || !creature->IsAlive())
        return false;

    // Waypoint movement can be switched on/off
    // This is quite handy for escort quests and other stuff
    if (creature->HasUnitState(UNIT_STATE_NOT_MOVE))
    {
        creature->ClearUnitState(UNIT_STATE_ROAMING_MOVE);
        return true;
    }
    // prevent a crash at empty waypoint path.
    if (!i_path || i_path->empty())
        return false;

    if (Stopped())
    {
        if (CanMove(diff))
            return StartMoveNow(creature);
    }
    else
    {
        // Set home position at place on waypoint movement.
        if (!creature->HasUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT) || !creature->GetTransGUID())
            creature->SetHomePosition(creature->GetPosition());

        if (creature->IsStopped())
            Stop(3 * MINUTE * IN_MILLISECONDS);
        //Checking just before reaching waypoint gives smother movement than using FinalDestination
        else if (creature->movespline->timeElapsed() < 150)
        {
            OnArrived(creature);
            IsArrivalDone = true;

            if (!Stopped())
            {
                if (creature->IsStopped())
                    Stop(3 * MINUTE * IN_MILLISECONDS);
                else
                    return StartMove(creature);
            }
        }
        else
        {
            // speed changed during path execution, calculate remaining path and launch it once more
            if (i_recalculateSpeed)
            {
                i_recalculateSpeed = false;

                if (!Stopped())
                    return StartMove(creature);
            }
            else
            {
                uint32 pointId = uint32(creature->movespline->currentPathIdx());
                if (pointId > i_currentNode)
                {
                    OnArrived(creature);
                    i_currentNode = pointId;
                    FormationMove(creature);
                }
            }
        }
    }
     return true;
 }

void WaypointMovementGenerator<Creature>::MovementInform(Creature* creature)
{
    if (creature->AI())
        creature->AI()->MovementInform(WAYPOINT_MOTION_TYPE, i_path->at(i_currentNode).id);
}

bool WaypointMovementGenerator<Creature>::GetResetPos(Creature*, float& x, float& y, float& z)
{
    // prevent a crash at empty waypoint path.
    if (!i_path || i_path->empty())
        return false;

    WaypointData const waypoint = i_path->at(i_currentNode);

    x = waypoint.x;
    y = waypoint.y;
    z = waypoint.z;
    return true;
}


//----------------------------------------------------//

uint32 FlightPathMovementGenerator::GetPathAtMapEnd() const
{
    if (i_currentNode >= i_path.size())
        return i_path.size();

    uint32 curMapId = i_path[i_currentNode]->mapid;
    for (uint32 i = i_currentNode; i < i_path.size(); ++i)
        if (i_path[i]->mapid != curMapId)
            return i;

    return i_path.size();
}

void FlightPathMovementGenerator::LoadPath(Player* player)
{
    _pointsForPathSwitch.clear();
    std::vector<uint32> const& taxi = player->m_taxi.GetPath();
    for (uint32 src = player->m_taxi.GetTaxiSegment(), dst = player->m_taxi.GetTaxiSegment()+1; dst < taxi.size(); src = dst++)
    {
        uint32 path, cost;
        sObjectMgr->GetTaxiPath(taxi[src], taxi[dst], path, cost);
        if (path > sTaxiPathNodesByPath.size())
            return;

        TaxiPathNodeList const& nodes = sTaxiPathNodesByPath[path];
        if (!nodes.empty())
        {
            TaxiPathNodeEntry const* start = nodes[0];
            TaxiPathNodeEntry const* end = nodes[nodes.size() - 1];
            for (uint32 i = 0; i < nodes.size(); ++i)
                i_path.push_back(nodes[i]);
        }

        _pointsForPathSwitch.push_back(uint32(i_path.size() - 1));
    }
}

void FlightPathMovementGenerator::DoInitialize(Player* player)
{
    Reset(player);
    InitEndGridInfo();
}

void FlightPathMovementGenerator::DoFinalize(Player* player)
{
    // remove flag to prevent send object build movement packets for flight state and crash (movement generator already not at top of stack)
    player->ClearUnitState(UNIT_STATE_IN_FLIGHT);

    player->Dismount();
    player->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_TAXI_FLIGHT);

    if (player->m_taxi.empty())
    {
        player->getHostileRefManager().setOnlineOfflineState(true);
        // update z position to ground and orientation for landing point
        // this prevent cheating with landing  point at lags
        // when client side flight end early in comparison server side
        player->StopMoving();
    }

    player->RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_TAXI_BENCHMARK);
}

#define PLAYER_FLIGHT_SPEED 30.0f

void FlightPathMovementGenerator::DoReset(Player* player)
{
    player->getHostileRefManager().setOnlineOfflineState(false);
    player->AddUnitState(UNIT_STATE_IN_FLIGHT);
    player->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_TAXI_FLIGHT);

    Movement::MoveSplineInit init(player);
    uint32 end = GetPathAtMapEnd();
    for (uint32 i = GetCurrentNode(); i != end; ++i)
    {
        G3D::Vector3 vertice(i_path[i]->x, i_path[i]->y, i_path[i]->z);
        init.Path().push_back(vertice);
    }
    init.SetFirstPointId(GetCurrentNode());
    init.SetFly();
    //init.SetSmooth();
    init.SetWalk(true);
    init.SetVelocity(PLAYER_FLIGHT_SPEED);
    init.Launch();
}

bool FlightPathMovementGenerator::DoUpdate(Player* player, uint32 /*diff*/)
{
    if (_mapSwitch)
    {
        DoInitialize(player);
        _mapSwitch = false;
        return true;
    }

    uint32 pointId = (uint32)player->movespline->currentPathIdx();
    if (pointId > i_currentNode)
    {
        bool departureEvent = true;
        do
        {
            if (i_currentNode >= i_path.size())
            {
                sLog->outMisc("TAXI NODE WAS GREATER THAN PATH SIZE, GUID: %u, POINTID: %u, NODESIZE: %u, CURRENT: %u", player->GetGUIDLow(), pointId, i_path.size(), i_currentNode);
                player->CleanupAfterTaxiFlight();
                return false;
            }

            if (i_path[i_currentNode]->mapid != player->GetMapId())
            {
                sLog->outMisc("Player on different map, curmap: %u, pointmap: %u, nodesize: %u, currentnode: %u", player->GetMapId(), i_path[i_currentNode]->mapid, i_path.size(), i_currentNode);
                player->CleanupAfterTaxiFlight();
                return false;
            }

            DoEventIfAny(player, i_path[i_currentNode], departureEvent);
            uint32 curSize = _pointsForPathSwitch.size();
            while (!_pointsForPathSwitch.empty() && _pointsForPathSwitch.front() <= i_currentNode)
                _pointsForPathSwitch.pop_front();
            if (curSize != _pointsForPathSwitch.size())
                player->m_taxi.NextTaxiDestination();

            if (pointId == i_currentNode)
                break;

            if (i_currentNode == _preloadTargetNode && player->GetMapId() == _endMapId)
                PreloadEndGrid();
            i_currentNode += (uint32)departureEvent;
            departureEvent = !departureEvent;
            if (i_currentNode + 1 < i_path.size() && i_path[i_currentNode+1]->mapid != player->GetMapId())
            {
                ++i_currentNode;
                _mapSwitch = true;
                player->TeleportTo(i_path[i_currentNode]->mapid, i_path[i_currentNode]->x, i_path[i_currentNode]->y, i_path[i_currentNode]->z, player->GetOrientation(), TELE_TO_NOT_LEAVE_TAXI);
                return true;
            }

            if (i_currentNode >= i_path.size() - 1)
            {
                player->CleanupAfterTaxiFlight();
                player->SetFallInformation(time(NULL), player->GetPositionZ());
                if (player->pvpInfo.IsHostile)
                    player->CastSpell(player, 2479, true);

                return false;
            }
        }
        while (true);
    }

    return i_currentNode < (i_path.size() - 1);
}

void FlightPathMovementGenerator::SetCurrentNodeAfterTeleport()
{
    if (i_path.empty() || i_currentNode >= i_path.size())
        return;

    uint32 map0 = i_path[i_currentNode]->mapid;
    for (size_t i = i_currentNode + 1; i < i_path.size(); ++i)
    {
        if (i_path[i]->mapid != map0)
        {
            i_currentNode = i;
            return;
        }
    }
}

void FlightPathMovementGenerator::DoEventIfAny(Player* player, TaxiPathNodeEntry const* node, bool departure)
{
    if (uint32 eventid = departure ? node->departureEventID : node->arrivalEventID)
        player->GetMap()->ScriptsStart(sEventScripts, eventid, player, player);
}

void FlightPathMovementGenerator::InitEndGridInfo()
{
    /*! Storage to preload flightmaster grid at end of flight. For multi-stop flights, this will
       be reinitialized for each flightmaster at the end of each spline (or stop) in the flight. */
    uint32 nodeCount = i_path.size();        //! Number of nodes in path.
    _endMapId = i_path[nodeCount - 1]->mapid; //! MapId of last node

    {
        _preloadTargetNode = nodeCount - 1;
        for (uint8 i = 3; i > 0; --i)
            if (nodeCount >= i && _endMapId == i_path[nodeCount - i]->mapid)
            {
                _preloadTargetNode = nodeCount - i;
                break;
            }
    }

    _endGridX = i_path[nodeCount - 1]->x;
    _endGridY = i_path[nodeCount - 1]->y;
}

void FlightPathMovementGenerator::PreloadEndGrid()
{
    // used to preload the final grid where the flightmaster is
    Map* endMap = sMapMgr->FindBaseNonInstanceMap(_endMapId);

    // Load the grid
    if (endMap)
        endMap->LoadGrid(_endGridX, _endGridY);
}
