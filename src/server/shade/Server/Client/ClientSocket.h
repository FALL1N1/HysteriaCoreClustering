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

/** \addtogroup u2w User to World Communication
 * @{
 * \file WorldSocket.h
 * \author Derex <derex101@gmail.com>
 */

#ifndef _CLIENTSOCKET_H
#define _CLIENTSOCKET_H

#include <ace/Svc_Handler.h>
#include <ace/Unbounded_Queue.h>
#include <ace/Message_Block.h>

#include "AcceptSocket.h"
#include "SocketAcceptor.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "Common.h"
#include "AuthCrypt.h"

class ACE_Message_Block;
class WorldPacket;

class ClientSocket : public AcceptSocket
{
public:
    ClientSocket();
    virtual ~ClientSocket();

    uint32 GetLatency() const;
    uint32 GetLastActionTime() const;

private:
    virtual int _sendPacket(const WorldPacket &pct);
    virtual int _handleInputHeader();
    virtual int _processIncoming(WorldPacket *new_pct);

    void _buildAuthChallengePacket(WorldPacket &packet);
    int _handleAuthSession(WorldPacket& recvPacket);
    int _handlePing(WorldPacket& recvPacket);

    ACE_Time_Value m_LastPingTime;
    uint32 m_OverSpeedPings;

    AuthCrypt m_Crypt;
    uint32 m_Seed;

    uint32 _latency;
    uint32 _lastActionTime;
};

#endif  /* _CLIENTSOCKET_H */

#define sClientSocketAcceptor ACE_Singleton<SocketAcceptor<ClientSocket>, ACE_Thread_Mutex>::instance()

/// @}

