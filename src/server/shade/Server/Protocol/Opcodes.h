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

/// \addtogroup u2w
/// @{
/// \file

#ifndef _OPCODES_H
#define _OPCODES_H

#include "Common.h"
#include "OpcodeDefines.hpp"

// Note: this include need for be sure have full definition of class WorldSession
//       if this class definition not complite then VS for x64 release use different size for
//       struct OpcodeHandler in this header and Opcode.cpp and get totally wrong data from
//       table opcodeTable in source when Opcode.h included but WorldSession.h not included
#include "ClientSession.h"

/// Player state
enum SessionStatus
{
    STATUS_AUTHED = 0,                                      // Player authenticated (_player == NULL, m_playerRecentlyLogout = false or will be reset before handler call, m_GUID have garbage)
    STATUS_LOGGEDIN,                                        // Player in game (_player != NULL, m_GUID == _player->GetGUID(), inWorld())
    STATUS_TRANSFER,                                        // Player transferring to another map (_player != NULL, m_GUID == _player->GetGUID(), !inWorld())
    STATUS_LOGGEDIN_OR_RECENTLY_LOGGOUT,                    // _player!= NULL or _player == NULL && m_playerRecentlyLogout, m_GUID store last _player guid)
    STATUS_NEVER,                                           // Opcode not accepted from client (deprecated or server side only)
    STATUS_UNHANDLED,                                       // Opcode not handled yet
};

enum PacketState
{
    PACKET_STATE_NULL,                                      // Route to NULL
    PACKET_STATE_LOGON,                                     // Handled by Logon only
    PACKET_STATE_NODE,                                      // Route to node
    PACKET_STATE_BOTH,                                      // Handled by both
    PACKET_STATE_PASS                                       // Packet will pass from Node to Client
};

class WorldPacket;

struct OpcodeHandler
{
    char const* name;
    bool multipass;
    SessionStatus status;
    PacketState to_node;
    PacketState from_node;
    void (ClientSession::*handler)(WorldPacket& recvPacket);
};

extern OpcodeHandler opcodeTable[NUM_MSG_TYPES];

/// Lookup opcode name for human understandable logging
inline const char* LookupOpcodeName(uint16 opcode)
{
    if (opcode >= NUM_MSG_TYPES)
        return "Received unknown opcode, it's more than max!";
    return opcodeTable[opcode].name;
}
#endif
/// @}
