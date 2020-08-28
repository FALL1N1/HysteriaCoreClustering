#include "MovementChecker.h"
#include "Unit.h"
#include "Creature.h"
#include "World.h"
#include "Player.h"
#include "Unit.h"

void PlayerMovementChecker::prepare(MovementInfo *movementInfo, uint16 opcode)
{
    if(_player->GetSession()->GetSecurity() >= 1) //skip unneeded calculations
        return;

    _movementInfo = movementInfo;
    _opcode = opcode;

    _currServerTime = getMSTime();

    if(_lastServerTime == 0)
        _lastServerTime = _currServerTime;

    _timediff = sWorld->GetUpdateTime();

    _mover = _player->m_mover;

    _vehicle = (_mover->GetVehicleKit()) ? _mover->GetVehicleKit()->GetBase() : NULL;

    _lastXCoord = _mover->GetPositionX();
    _lastYCoord = _mover->GetPositionY();
    _lastZCoord = _mover->GetPositionZ();

    _mapZ = _mover->GetMap()->GetHeight(_lastXCoord, _lastYCoord, MAX_HEIGHT);
    _floorZ = _mover->GetMap()->GetHeight(_lastXCoord, _lastYCoord, _lastZCoord, true, MAX_FALL_DISTANCE);

    _distance2D = _mover->GetExactDist2d(_movementInfo->pos.m_positionX, _movementInfo->pos.m_positionY);
    _distance3D = _mover->GetExactDist(_movementInfo->pos.m_positionX, _movementInfo->pos.m_positionY, _movementInfo->pos.m_positionZ);

    _currSpeed = _mover->GetSpeed(GetMovementType());

    if(_lastClientTime == 0)
        _lastClientTime = _movementInfo->time;

    if(_opcode == MSG_MOVE_FALL_LAND)
    {
        _alreadyJumped = false;
    }

    _moverSize = _mover->GetObjectSize();

    _hasFlyingFlags = (_movementInfo->flags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_FLYING));

    _moverHeight = 0.0f;

    _justTurn = false;

    if(_movementInfo->flags & (MOVEMENTFLAG_MASK_TURNING))
    {
        float deltaX = abs(_movementInfo->pos.m_positionX - _lastXCoord);
        float deltaY = abs(_movementInfo->pos.m_positionY - _lastYCoord);
        float deltaZ = abs(_movementInfo->pos.m_positionZ - _lastZCoord);

        if(deltaX <= 0.1f && deltaY <= 0.1f && deltaZ <= 0.1f)
            _justTurn = true;
        else
            _justTurn = false;
    }

    _isInWater = _player->IsInWater();

    uint32 waterTimeDelta = _currServerTime - _lastInWaterTime;

    _justWasInWater = waterTimeDelta < _latency * 2 + _timediff * 2 + 1000;

    if(_isInWater)
        _lastInWaterTime = _currServerTime;

    if(Player* plMover = _mover->ToPlayer())
        _latency = plMover->GetSession()->GetLatency();
    else
        _latency = _player->GetSession()->GetLatency();
}

bool PlayerMovementChecker::canMove()
{
    if ((_player->GetSession()->GetSecurity() >= 1) || _player->IsPolymorphed() && _player->GetMap()->IsBattlegroundOrArena())
        return true;

    if (!isFlyingOk() || !isPlaneOk() || !isJumpOk() || !isFeatherFallOk() || !isWaterWalkOk() || !isSpeedOk() || !isZCoordOk())
    {
        _lastSpeed = _currSpeed;
         return false;
    }

    _lastSpeed = _currSpeed;

    return true;
}

bool PlayerMovementChecker::isFlyingOk()
{
    if(!(sWorld->getIntConfig(CONFIG_MOVEMENT_CHECKS_MASK) & HACK_TYPE_FLYING))
        return true;

    const bool flying = (_hasFlyingFlags && !(_movementInfo->flags & (MOVEMENTFLAG_SWIMMING))) || _movementInfo->flags & (MOVEMENTFLAG_DISABLE_GRAVITY); // allow X button in water

    if(!flying)
        return true;

    _justWasInAir = true;

    // are we posessing something that can fly?
    // bool controlledCanFly = false;
    /*for (Unit::ControlList::const_iterator itr = _player->m_Posessing.begin(); itr != _player->m_Posessing.end(); ++itr)
    {
        if (controlledCanFly) // we don't need to keep iterating through our controlled list
            break;

        if ((*itr) && (*itr)->GetTypeId() == TYPEID_UNIT)
        {
            Unit* possessed = (*itr);
            if (possessed->GetCharmerGUID() == _player->GetGUID())
            {
                controlledCanFly =
                    (possessed->CanFly() ||
                    possessed->HasAuraType(SPELL_AURA_FLY) ||
                    possessed->HasAuraType(SPELL_AURA_MOD_INCREASE_VEHICLE_FLIGHT_SPEED) ||
                    possessed->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED) ||
                    possessed->HasAuraType(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED) ||
                    possessed->HasAuraType(SPELL_AURA_MOD_MOUNTED_FLIGHT_SPEED_ALWAYS));
            }
        }
    }
    */

    // can our vehicle fly?
    bool vehicleCanFly = false;
    if(_vehicle)
    {
       if(Creature* pCreature = _vehicle->ToCreature())
           vehicleCanFly = (pCreature->CanFly() || pCreature->HasAuraType(SPELL_AURA_FLY) || pCreature->HasAuraType(SPELL_AURA_MOD_INCREASE_VEHICLE_FLIGHT_SPEED)
                             || pCreature->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED) || pCreature->HasAuraType(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED)
                             || pCreature->HasAuraType(SPELL_AURA_MOD_MOUNTED_FLIGHT_SPEED_ALWAYS));
    }

    // can the player fly dead?
    bool deadCanFly = false;
    if (_player->isDead())
    {
        uint32 map = GetVirtualMapForMapAndZone(_player->GetMapId(), _player->GetZoneId());
        if (map == 530 || map == 571)
            deadCanFly = true;
    }

    const bool canFly = (_player->HasAuraType(SPELL_AURA_FLY) || _player->HasAuraType(SPELL_AURA_MOD_INCREASE_VEHICLE_FLIGHT_SPEED)
                || _player->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED) || _player->HasAuraType(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED)
                || _player->HasAuraType(SPELL_AURA_MOD_MOUNTED_FLIGHT_SPEED_ALWAYS) || vehicleCanFly || /*controlledCanFly ||*/ deadCanFly);

    uint32 flyTimeDelta = _currServerTime - _lastFlyingTime;

    const bool justHadFlyer = flyTimeDelta < _latency * 2 + _timediff * 2 + 1000;

    const bool CanDisableGrav = _movementInfo->flags & (MOVEMENTFLAG_DISABLE_GRAVITY) ? (canFly || justHadFlyer) : true;
    const bool passed = (canFly || (_player->HasUnitState(UNIT_STATE_ROAMING|UNIT_STATE_ROAMING_MOVE|UNIT_STATE_CHARGING) || _justCharged || justHadFlyer) && CanDisableGrav);

    if(canFly)
        _lastFlyingTime = _currServerTime;

    if(!passed)
    {
        SendGMWarning(HACK_TYPE_FLYING);
    }

    return passed;
}

bool PlayerMovementChecker::isPlaneOk()
{
    if(!(sWorld->getIntConfig(CONFIG_MOVEMENT_CHECKS_MASK) & HACK_TYPE_TELEPORT) || _movementInfo->flags & MOVEMENTFLAG_ONTRANSPORT)
        return true;

    if (_player->GetVehicle())
        return true;

    if (_player->GetVehicleBase() && (_player->GetVehicleBase()->GetCharmerGUID() != _player->GetGUID())) // player is in a vehicle that they don't control
        return true;

    float clientTimeDelta = ((float)_movementInfo->time - (float)_lastClientTime + ((float)_latency * 2.0f)) / 1000; // time between last movement info and now (in seconds)
    float speedToCheck = (_currSpeed < _lastSpeed) ? _lastSpeed : _currSpeed;

    if (_distance2D > 50.0f) // a dist of more than 50 between your last 2 movements is suspicious
    {
        if ((_distance2D / clientTimeDelta) >= (speedToCheck * 2))
        {
            SendGMWarning(HACK_TYPE_TELEPORT);
            return false;
        }
    }
    return true;
}

bool PlayerMovementChecker::isJumpOk()
{
    if(!(sWorld->getIntConfig(CONFIG_MOVEMENT_CHECKS_MASK) & HACK_TYPE_JUMP))
        return true;

    if(_opcode != MSG_MOVE_JUMP || _isInWater)
    {
        _alreadyJumped = false;
        return true;
    }

    if(_alreadyJumped)
    {
        SendGMWarning(HACK_TYPE_JUMP);
        return false;
    }

    _alreadyJumped = true;

    return true;
}

bool PlayerMovementChecker::isFeatherFallOk()
{
    if (!(sWorld->getIntConfig(CONFIG_MOVEMENT_CHECKS_MASK) & HACK_TYPE_HOVER))
        return true;

    if(_justTurn || !(_movementInfo->flags & (MOVEMENTFLAG_FALLING_SLOW)))
        return true;

    const bool hasFeatherFall = _mover->HasAuraType(SPELL_AURA_FEATHER_FALL);

    uint32 featherFallTimeDelta = _currServerTime - _lastFeatherFallTime;

    const bool justHadFeaterFall = featherFallTimeDelta < _latency * 2 + _timediff * 2 + 1000;

    if(_isInWater)
        _lastInWaterTime = _currServerTime;

    if(!hasFeatherFall && !justHadFeaterFall && !(_player->HasUnitState(UNIT_STATE_ROAMING|UNIT_STATE_ROAMING_MOVE|UNIT_STATE_CHARGING) || _justCharged))
    {
        SendGMWarning(HACK_TYPE_HOVER);
        return false;
    }

    if(hasFeatherFall)
        _lastFeatherFallTime = _currServerTime;

    return true;
}

bool PlayerMovementChecker::isWaterWalkOk()
{
    if(!(sWorld->getIntConfig(CONFIG_MOVEMENT_CHECKS_MASK) & HACK_TYPE_WATERWALK))
        return true;

    const bool hasWaterwalkAura = (_mover->HasAuraType(SPELL_AURA_WATER_WALK) || _mover->HasAuraType(SPELL_AURA_GHOST));

    uint32 waterWalkTimeDelta = _currServerTime - _lastWaterwalkTime;

    const bool justHadWaterwalkAura = waterWalkTimeDelta < _latency * 2 + _timediff * 2 + 1000;

    if(!hasWaterwalkAura && !justHadWaterwalkAura && !(_player->HasUnitState(UNIT_STATE_ROAMING|UNIT_STATE_ROAMING_MOVE|UNIT_STATE_CHARGING) || _justCharged) && _player->HasUnitMovementFlag(MOVEMENTFLAG_WATERWALKING))
    {
        SendGMWarning(HACK_TYPE_WATERWALK);
        return false;
    }

    if(hasWaterwalkAura)
        _lastWaterwalkTime = _currServerTime;

    return true;
}

bool PlayerMovementChecker::isSpeedOk()
{
    if(!(sWorld->getIntConfig(CONFIG_MOVEMENT_CHECKS_MASK) & HACK_TYPE_SPEED) || _movementInfo->flags & MOVEMENTFLAG_ONTRANSPORT || _justTurn)
    {
        _lastClientTime = _movementInfo->time;
        _lastServerTime = _currServerTime;
        return true;
    }

    UnitMoveType moveType = GetMovementType();

    // TODO: prevent speed hacking with vehicles
    if(_player->GetVehicle())
        return true;
        // _currSpeed = _vehicle->GetSpeed(moveType);

    uint32 clientTimeDelta = _movementInfo->time - _lastClientTime + (_latency * 2);
    const uint32 serverTimeDelta = _currServerTime - _lastServerTime + _latency + _timediff;

    if(clientTimeDelta > 1000)
        clientTimeDelta += _timediff;

    _lastClientTime = _movementInfo->time;
    _lastServerTime = _currServerTime;

    //const float totalTimeDelta = (clientTimeDelta < serverTimeDelta) ? (float)clientTimeDelta * 0.001f : (float)serverTimeDelta * 0.001f;
    const float totalTimeDelta = (float)clientTimeDelta * 0.001f;

    float speedToCheck = (_currSpeed < _lastSpeed) ? _lastSpeed : _currSpeed;

    _justWasInAir = (_justWasInAir) ? isInAir() : false;

    bool charging = _mover->HasUnitState(UNIT_STATE_CHARGING) || _justCharged;

    if(_opcode == MSG_MOVE_FALL_LAND || _movementInfo->fallTime != _lastFallTime || _justWasInAir || charging)
        speedToCheck = SPEED_CHARGE;

    _justCharged = (!_mover->HasUnitState(UNIT_STATE_CHARGING) && _justCharged) ? false : _mover->HasUnitState(UNIT_STATE_CHARGING);

    _lastFallTime = _movementInfo->fallTime;

    float allowedDistance = (speedToCheck * totalTimeDelta) + _moverSize;

    if(_distance2D > allowedDistance)
    {
        SendGMWarning(HACK_TYPE_SPEED);
        return false;
    }

    return true;
}

bool PlayerMovementChecker::isZCoordOk()
{
    if(!(sWorld->getIntConfig(CONFIG_MOVEMENT_CHECKS_MASK) & HACK_TYPE_CLIMB))
        return true;

    /* NEEDS REWRITE? */
    
    if(_hasFlyingFlags || _justTurn || _player->IsInFlight() || (_movementInfo->flags & (MOVEMENTFLAG_SWIMMING|MOVEMENTFLAG_ONTRANSPORT))
     || !isInAir())
        return true;

    float deltaZ = _movementInfo->pos.m_positionZ - _lastZCoord;
    float deltaX = _movementInfo->pos.m_positionX - _lastXCoord;
    float deltaY = _movementInfo->pos.m_positionY - _lastYCoord;

    if (_player->IsMounted())
    {
        if((deltaZ > _moverSize + 3.0f) && deltaX < 4.5f && deltaX > -4.5f && deltaY < 4.5f && deltaY > -4.5f)
        {
            SendGMWarning(HACK_TYPE_CLIMB);
            return false;
        }
    }
    else
    if((deltaZ > _moverSize + 3.0f) && deltaX < 4.0f && deltaX > -4.0f && deltaY < 4.0f && deltaY > -4.0f)
    {
        SendGMWarning(HACK_TYPE_CLIMB);
        return false;
    }
    
    return true;
}

bool PlayerMovementChecker::isInAir()
{
    if(_hasFlyingFlags || _player->IsInFlight() || (_movementInfo->flags & (MOVEMENTFLAG_SWIMMING)))
        return false;

    _isAboveFloor = (_movementInfo->pos.m_positionZ > (_floorZ + _moverSize * 3));

    _moverHeight = _movementInfo->pos.m_positionZ - _floorZ + 0.01f;

    return _isAboveFloor;
}

UnitMoveType PlayerMovementChecker::GetMovementType()
{
    UnitMoveType moveType;

    switch(_movementInfo->flags & (MOVEMENTFLAG_FLYING | MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_WALKING))
    {
        case MOVEMENTFLAG_FLYING:
            moveType = _movementInfo->flags & MOVEMENTFLAG_BACKWARD ? MOVE_FLIGHT_BACK : MOVE_FLIGHT;
            break;
        case MOVEMENTFLAG_SWIMMING:
            moveType = _movementInfo->flags & MOVEMENTFLAG_BACKWARD ? MOVE_SWIM_BACK : MOVE_SWIM;
            break;
        case MOVEMENTFLAG_WALKING:
            moveType = MOVE_WALK;
            break;
        default:
            moveType = _movementInfo->flags & MOVEMENTFLAG_BACKWARD ? MOVE_RUN_BACK : MOVE_RUN;
            break;
    }

    return moveType;
}

void PlayerMovementChecker::doMovementCorrection()
{
    WorldPacket data;

    _mover->SetUnitMovementFlags(MOVEMENTFLAG_NONE);

    Player *plMover = _mover->ToPlayer();

    if(plMover && plMover != _player)
        plMover->SendTeleportAckPacket();

    _player->SendTeleportAckPacket();

    _mover->BuildHeartBeatMsg(&data);
    _mover->SendMessageToSet(&data, true);
}

void PlayerMovementChecker::SendGMWarning(uint32 type)
{
    std::string _type;
    std::string playername = _player->GetName();

    switch (type)
    {
        case HACK_TYPE_FLYING:
            _type = "Fly";
            break;
        case HACK_TYPE_TELEPORT:
            _type = "Teleport";
            break;
        case HACK_TYPE_JUMP:
            _type = "Jump";
            break;
        case HACK_TYPE_HOVER:
            _type = "Hover";
            break;
        case HACK_TYPE_WATERWALK:
            _type = "Water-Walk";
            break;
        case HACK_TYPE_SPEED:
            _type = "Speed";
            break;
        case HACK_TYPE_CLIMB:
            _type = "Climb";
            break;
        case HACK_TYPE_COLLISION:
        case HACK_TYPE_FACTION:
        default:
            _type = "<error>";
            break;
    }

    sLog->outString("ACTIVEANTICHEAT:  [Player: %s (Guid: %u, Account: %u)] caught %s hacking", playername.c_str(), _player->GetGUIDLow(), _player->GetSession()->GetAccountId(), _type.c_str());

    std::string msg = "";
    msg = "|cFFFFFC00[SHIELD]|cFFff0000[|cFF60FF00" + playername + "|cFFff0000] Caught "+_type+" hacking!";
    WorldPacket data(SMSG_NOTIFICATION, (msg.size()+1));
    data << msg;
    sWorld->SendGlobalGMMessage(&data);
}
