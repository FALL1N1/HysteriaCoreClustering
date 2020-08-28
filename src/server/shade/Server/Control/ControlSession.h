#ifndef __CONTROLSESSION_H
#define __CONTROLSESSION_H

#include "Proxy.h"
#include "AddonMgr.h"
#include "DatabaseEnv.h"
#include "ByteBuffer.h"
#include "Proxy.h"

class WorldPacket;
class ByteBuffer;
class ControlSocket;
class Player;
class LoginQueryHolder;

enum CTL_SessionFlags
{
    CTL_FLAG_NODE_RECONNECT      = 0,
    CTL_FLAG_NODE_AUTHED,
    CTL_MAX_NODE_FLAGS
};

class ControlSession
{

public:
    ControlSession(uint32 NodeID, uint8 NodeType);
    ~ControlSession();

    void CloseSocket();

    //Flags
    void SetFlag(CTL_SessionFlags flag, bool state) { m_sessionflags[flag] = state; }
    bool HasFlag(CTL_SessionFlags flag) { return m_sessionflags[flag]; }

    bool Update(uint32 diff);
    bool UpdateControlIO(uint32 diff);

    //NodeInformations
    uint32 GetNodeId() const { return _nodeId; }
    uint8  GetNodeType() const { return _nodeType; }

    //SendHandlings
public:
    void SendPacket(WorldPacket const* packet);

public:
    void SendInitAuthedNode();

    //The OpcodeHandling
public:
    void Handle_NULL(WorldPacket& recvPacket);
    void Handle_ServerSide(WorldPacket& recvPacket);
    void Handle_SMSG_AUTH_RESPONSE(WorldPacket &recv_data);  //Ja wir nutzen einen AuthResponse
    void Handle_NODE_INIT_ACK(WorldPacket &recvPacket);      //InitACK == 254 wenn alles Okay
    void Handle_NODE_SYNC_DATA(WorldPacket &recvPacket);     //SyncDatas für GUIDs
    void Handle_STEST(WorldPacket &recv_data);
    void Handle_SMSG_AUTH_CHALLENGE(WorldPacket& recvPacket);

private:
    void HandlePacket(WorldPacket *packet);

    ControlSocket *m_Socket;
    uint32 _reconnect_timer;
    bool m_sessionflags[CTL_MAX_NODE_FLAGS];

    uint32 _nodeId;
    uint8  _nodeType;

    bool   m_Logout;
    bool   m_inQueue;
    std::string m_Address;
};

#endif
