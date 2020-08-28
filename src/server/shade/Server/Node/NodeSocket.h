#ifndef __NODESOCKET_H
#define __NODESOCKET_H

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
#include "RoutingHelper.h"
#include "SocketConnector.h"

class ACE_Message_Block;
class WorldPacket;

class NodeSocket : public Socket
{
public:
    NodeSocket();

private:
    virtual int _sendPacket(const WorldPacket &pct);
    virtual int _handleInputHeader();
    virtual int _processIncoming(WorldPacket *new_pct);

    AuthCrypt _crypt;
    uint32 _seed;
};

class NodeSocketConnector : public SocketConnector<NodeSocket>
{
public:
    NodeSocket* OpenNodeConnection(uint32 nodeID)
    {
        //sLog->outString("OpenNodeConnection(%u)", nodeID);
        if (sRoutingHelper->CheckNodeID(nodeID))
        {
            //sLog->outString("CheckNodeID(%u)", nodeID);
            NodeList nodeInfo = sRoutingHelper->GetNodeConnectionData(nodeID);
            //sLog->outString("CheckNodeID(%u), nodeInfo(%u), nodeInfo(%s)", nodeID, nodeInfo.port, nodeInfo.IP);
                return SocketConnector<NodeSocket>::OpenConnection(nodeInfo.port, nodeInfo.IP);
        }
        
        return 0;
    }
};

#define sNodeSocketConnector ACE_Singleton<NodeSocketConnector, ACE_Thread_Mutex>::instance()

#endif
