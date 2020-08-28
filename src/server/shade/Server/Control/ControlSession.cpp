#include "ControlSocket.h"
#include "RoutingHelper.h"

#include "ObjectMgr.h"
#include "Player.h"

#include "ControlSession.h"
#include "Opcodes.h"
#include "WorldPacket.h"
#include "ByteBuffer.h"
#include "Common.h"
#include "Log.h"
#include "zlib.h"
#include "ClusterDefines.h"

ControlSession::ControlSession(uint32 NodeID, uint8 NodeType) :_nodeId(NodeID), _nodeType(NodeType)
{
    m_Socket = NULL;
    m_Logout = false;

    if (ControlSocket *socket = sControlSocketConnector->OpenNodeConnection(NodeID))
    {
        socket->add_reference();

        m_Socket = socket;
        SetFlag(CTL_FLAG_NODE_RECONNECT, false);
    }
    else
    {
        _reconnect_timer = 2000;
        SetFlag(CTL_FLAG_NODE_RECONNECT, true);
    }

}

ControlSession::~ControlSession()
{
    SetFlag(CTL_FLAG_NODE_RECONNECT, false);
    /// - If have unclosed socket, close it

    if (m_Socket)
    {
        m_Socket->close();
        m_Socket->remove_reference();

        m_Socket = NULL;
    }
}

void ControlSession::CloseSocket()
{
    if (m_Socket)
        m_Socket->close();
}

bool ControlSession::Update(uint32 diff)
{
    if (HasFlag(CTL_FLAG_NODE_RECONNECT))
    {
        if (_reconnect_timer <= diff)
        {
            _reconnect_timer = 2000;
            if (sRoutingHelper->CheckNodeID(_nodeId))
                if (ControlSocket* socket = sControlSocketConnector->OpenNodeConnection(_nodeId))
                {
                    socket->add_reference();
                    m_Socket = socket;

                    SetFlag(CTL_FLAG_NODE_RECONNECT, false);
                }
        }
        else
            _reconnect_timer -= diff;
    }

    UpdateControlIO(diff);

    if (m_Socket && m_Socket->IsClosing())
    {
        sRoutingHelper->SetNodeOffline(GetNodeId()); //Set the Node offline

        m_Socket->remove_reference();
        m_Socket = NULL;

        _reconnect_timer = 2000;
        SetFlag(CTL_FLAG_NODE_RECONNECT, true);
    }

    return true;
}

void ControlSession::HandlePacket(WorldPacket *packet)
{
    sLog->outDetail("SESSION_CTRL: Received opcode 0x%.4X (%s)", packet->GetOpcode(), LookupOpcodeName(packet->GetOpcode()));

    //sLog->outString("SESSION_NODE: Received opcode 0x%.4X (%s)", packet->GetOpcode(), LookupOpcodeName(packet->GetOpcode()));
    if (packet->GetOpcode() >= NUM_MSG_TYPES)
    {
        sLog->outError("SESSION_NODE: received non-existed opcode %s (0x%.4X)",
            LookupOpcodeName(packet->GetOpcode()),
            packet->GetOpcode());
    }
    else
    {
        try
        {
            switch (packet->GetOpcode())
            {
                case SMSG_AUTH_RESPONSE:
                    Handle_SMSG_AUTH_RESPONSE(*packet);
                    break;
                case NODE_INIT_ACK:
                    Handle_NODE_INIT_ACK(*packet);
                    break;
                case NODE_SYNC_DATA:
                    Handle_NODE_SYNC_DATA(*packet);
                    break;
                case SMSG_AUTH_CHALLENGE:
                    Handle_SMSG_AUTH_CHALLENGE(*packet);
                    break;
                default:
                    break;
            }
        }
        catch (ByteBufferException &)
        {
            //sLog->outError("WorldSession::Update ByteBufferException occured while parsing a packet (opcode: %u) from client %s, accountid=%i. Skipped packet.",
            //       packet->GetOpcode(), GetRemoteAddress().c_str(), GetAccountId());
            if (sLog->IsOutDebug())
            {
                sLog->outString("Dumping error causing packet:");
                packet->hexlike();
            }
        }
    }

    delete packet;
}

bool ControlSession::UpdateControlIO(uint32 diff)
{
    WorldPacket* packet;

    while (m_Socket && m_Socket->GetNextSinglePacket(packet))
        HandlePacket(packet);

    while (m_Socket && m_Socket->GetNextMultiPacket(packet))
        HandlePacket(packet);

    return 0;
}

//Sendhandlings
/// Send a packet to the client
void ControlSession::SendPacket(WorldPacket const* packet)
{
    if (!m_Socket)
        return;

    if (m_Socket->SendPacket(*packet) == -1)
        CloseSocket();
}

//Handle
void ControlSession::Handle_NULL(WorldPacket &recv_data)
{
}

void ControlSession::Handle_ServerSide(WorldPacket &recv_data)
{
}

void ControlSession::Handle_SMSG_AUTH_RESPONSE(WorldPacket &recv_data)
{
    uint8 in;
    recv_data >> in;
    if (in == AUTH_OK)
    {
        SetFlag(CTL_FLAG_NODE_AUTHED, true);
        SendInitAuthedNode();
    }
    else
        CloseSocket();
}

void ControlSession::Handle_NODE_INIT_ACK(WorldPacket &recvPacket)
{
    uint8 ack_code;

    recvPacket >> ack_code;
    if (ack_code == NODE_INIT_ACK_OK)
        sRoutingHelper->SetNodeOnline(_nodeId);
    else
        CloseSocket();

    if(_nodeId == 1)
        sLog->outString("[MasterNode]: ID: %u recieved status 'ONLINE' with code: 0x0%u_BLNZR", _nodeId, ack_code);
    else
        sLog->outString("[SlaveNode]: ID: %u recieved status 'ONLINE' with code: 0x0%u_BLNZR", _nodeId, ack_code);
}

void ControlSession::Handle_NODE_SYNC_DATA(WorldPacket &recvPacket)
{
    uint32 cmd = recvPacket.read<uint32>();

    switch (cmd)
    {
        case CL_DEF_GUID_SYNC:
            uint32 Type;
            uint32 GUID;
            recvPacket >> Type;
            recvPacket >> GUID;
            sRoutingHelper->SetLastGUID(_nodeId, Type, GUID);
            break;
    }
}

void ControlSession::Handle_SMSG_AUTH_CHALLENGE(WorldPacket& recvPacket)
{
    QueryResult result = LoginDatabase.PQuery("SELECT name FROM realmlist WHERE id = '%u'", realmID);

    // Stop if the account is not found
    if (!result)
    {
        sLog->outString("ControlSocket::HandleAuth: LogonServerName not found??? wtf...");
        CloseSocket();
    }

    Field* fields = result->Fetch();

    //Auth the Session
    WorldPacket packet(LOGON_AUTH_MASTER);
    packet << fields[0].GetString();

    SendPacket(&packet);
}

/********************\
|       TEMP         |
\********************/
void ControlSession::Handle_STEST(WorldPacket &recv_data)
{
    uint32 id;
    recv_data >> id;
    sLog->outString("BLA %u", id);
}
