#ifndef __POOLSESSION_H
#define __POOLSESSION_H

#include "DatabaseEnv.h"
#include "ByteBuffer.h"

class WorldPacket;
class ByteBuffer;
class PoolSocket;

class PoolSession
{
    
    public:
        PoolSession (uint32 id,  PoolSocket *sock);
        ~PoolSession();

        void CloseSockets();

        //Packethandling
        void QueueServerPacket(WorldPacket* new_packet);
        
        bool Update(uint32 diff);
        bool UpdateServerIO(uint32 diff);

        //AccountInformations
        uint32 GetServerId() const { return _serverId; }

    //SendHandlings
    public:
        void SendPacket(WorldPacket const* packet);
        void SendInitACK(uint8 ack);

    //The OpcodeHandling
    public:
        void Handle_NULL (WorldPacket& recvPacket);
        void Handle_ServerSide(WorldPacket& recvPacket);
        void Handle_LOGON_INIT_NODE(WorldPacket &recvPacket);
        void Handle_LOGON_SYNC_DATA(WorldPacket &recvPacket);

    private:
        PoolSocket *m_PoolSocket;
        uint32 _serverId;

        //Queues for I/O
        ACE_Based::LockedQueue<WorldPacket*, ACE_Thread_Mutex> _recvPoolQueue;
};

#endif