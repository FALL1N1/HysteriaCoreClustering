#ifndef __CONTROLSOCKET_H
#define __CONTROLSOCKET_H

#include <ace/Svc_Handler.h>
#include <ace/Unbounded_Queue.h>
#include <ace/SOCK_Connector.h>
#include <ace/Connector.h>

#include "Socket.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "Common.h"
#include "AuthCrypt.h"
#include "BigNumber.h"
#include "SocketConnector.h"

class ACE_Message_Block;
class WorldPacket;

class ControlSocket : public Socket
{
public:
    ControlSocket();
    ~ControlSocket();

private:
    virtual int _sendPacket(const WorldPacket &pct);
    virtual int _handleInputHeader();
    virtual int _processIncoming(WorldPacket *new_pct);

    AuthCrypt _crypt;

    uint32 _seed;
};

class ControlSocketConnector : public SocketConnector<ControlSocket>
{
public:
    ControlSocket * OpenNodeConnection(uint32 nodeID)
    {
        NodeList nodeInfo = sRoutingHelper->GetNodeConnectionData(nodeID);

        return SocketConnector<ControlSocket>::OpenConnection(nodeInfo.controlport, nodeInfo.IP);
    }
};

#define sControlSocketConnector ACE_Singleton<ControlSocketConnector, ACE_Thread_Mutex>::instance()

#endif
