#ifndef __POOLSESSIONMGR_H
#define __POOLSESSIONMGR_H

#include "Common.h"
#include "Timer.h"
#include <ace/Singleton.h>
#include <ace/Atomic_Op.h>
#include "QueryResult.h"
#include "Callback.h"

#include <map>
#include <set>
#include <list>


class PoolSession;

typedef std::unordered_map<uint32, PoolSession*> PoolSessionMap;

class PoolSessionMgr
{
    public:
        typedef std::list<PoolSession*> Queue;
        void Initialize();
        void ServerClose();
        
        void Update(uint32 diff);

        void AddSession(PoolSession *s);
        bool RemoveSession(uint32 id); 

        void AddQueuedPlayer(PoolSession*);
        bool RemoveQueuedPlayer(PoolSession* session);
        int32 GetQueuePos(PoolSession*);

        uint32 GetActiveAndQueuedSessionCount() const { return m_sessions.size(); }
        uint32 GetActiveSessionCount() const { return m_sessions.size() - m_QueuedPlayer.size(); }
        uint32 GetQueuedSessionCount() const { return m_QueuedPlayer.size(); }
        void UpdateMaxSessionCounters();

        void SetPlayerAmountLimit(uint32 limit) { m_playerLimit = limit; }
        uint32 GetPlayerAmountLimit() const { return m_playerLimit; }
        inline uint32 GetMaxPlayerCount() const { return m_MaxPlayerCount; }
        inline uint32 GetPlayerCount() const { return m_PlayerCount; }

        inline void IncreasePlayerCount()
        {
            m_PlayerCount++;
            m_MaxPlayerCount = std::max(m_MaxPlayerCount, m_PlayerCount);
        }
        inline void DecreasePlayerCount() { m_PlayerCount--; }

        PoolSession* GetSession(uint32 id);
        void SendGUIDSync(uint32 type, uint32 guid);
        void SendPacketToAll(WorldPacket const* packet);

    private:
        void AddSession_(PoolSession* s);

        Queue m_QueuedPlayer;
        ACE_Based::LockedQueue<PoolSession*, ACE_Thread_Mutex> addSessQueue;
        PoolSessionMap m_sessions; 

        uint32 m_maxActiveSessionCount;
        uint32 m_maxQueuedSessionCount;
        uint32 m_PlayerCount;
        uint32 m_MaxPlayerCount;
        uint32 m_playerLimit;

};
#define sPoolSessionMgr ACE_Singleton<PoolSessionMgr, ACE_Null_Mutex>::instance()
#endif