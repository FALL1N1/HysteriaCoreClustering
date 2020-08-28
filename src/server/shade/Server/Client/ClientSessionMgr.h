#ifndef __CLIENTSESSIONMGR_H
#define __CLIENTSESSIONMGR_H

#include "Common.h"
#include "Timer.h"
#include <ace/Singleton.h>
#include <ace/Atomic_Op.h>
#include "QueryResult.h"
#include "Callback.h"
#include "SessionUpdater.h"

#include "Proxy.h"
#include "Player.h"
#include "WorldPacket.h"
#include "Opcodes.h"

#include <map>
#include <set>
#include <list>


typedef std::unordered_map<uint32, ClientSession*> SessionMap;

class ClientSessionMgr
{
    public:

        void Initialize();
        void ServerClose();
        void Start();

        /*********************************************************/
        /***                 MISC HANDLING                     ***/
        /*********************************************************/
        bool HasRecentlyDisconnected(ClientSession* session);
        void UpdateMaxSessionCounters();
        uint32 GetActiveAndQueuedSessionCount() const { return _sessions.size(); }
        uint32 GetActiveSessionCount() const { return _sessions.size() - _QueuedPlayer.size(); }
        uint32 GetQueuedSessionCount() const { return _QueuedPlayer.size(); }

        void SetPlayerAmountLimit(uint32 limit) { _playerLimit = limit; }
        uint32 GetPlayerAmountLimit() const { return _playerLimit; }
        inline uint32 GetMaxPlayerCount() const { return _MaxPlayerCount; }
        inline uint32 GetPlayerCount() const { return _PlayerCount; }

        inline uint32 GetAllyCount() const { return _AllyCount; }
        inline uint32 GetHordeCount() const { return _HordeCount; }

        inline void IncreasePlayerCount(uint32 team)
        {
            _PlayerCount++;
            _MaxPlayerCount = std::max(_MaxPlayerCount, _PlayerCount);
            if (team == HORDE)
                _HordeCount++;
            else if (team == ALLIANCE)
                _AllyCount++;
        }
        inline void DecreasePlayerCount(uint32 team)
        {
            _PlayerCount--;
            if (team == HORDE)
                _HordeCount--;
            else if (team == ALLIANCE)
                _AllyCount--;
        }

        /*********************************************************/
        /***                QUEUE HANDLING                     ***/
        /*********************************************************/
        void AddQueuedPlayer(ClientSession*);
        bool RemoveQueuedPlayer(ClientSession* session);
        int32 GetQueuePos(ClientSession*);
        typedef std::list<ClientSession*> Queue;    

        /*********************************************************/
        /***        EXTERNAL SESSION HANDLING SYSTEM           ***/
        /*********************************************************/
        void AddSession(ClientSession *s);
        void Update(uint32 diff);
        bool RemoveSession(uint32 id);
        ClientSession* FindSession(uint32 id) const;
        
        /*********************************************************/
        /***                 BROADCAST SYSTEM                  ***/
        /*********************************************************/
        void ShutdownMsg(uint32 m_ShutdownTimer, uint32 m_ShutdownMask, bool show = false, Player* player = NULL);
        void HaltMsg(uint32 m_HaltTimer, uint32 m_ShutdownMask, bool show = false, Player* player = NULL);

        void SendServerMessage(ServerMessageType type, const char *text = "", Player* player = NULL);
        void SendGlobalMessage(WorldPacket* packet, ClientSession* self = 0, uint32 team = 0);
        void SendGameMasterMessage(const char *text, ...);

    private:
        /*********************************************************/
        /***        INTERNAL SESSION HANDLING SYSTEM           ***/
        /*********************************************************/
        void AddSession_(ClientSession* s);

    private:
        /*********************************************************/
        /***              DYNAMIC CONTROLER                    ***/
        /*********************************************************/
        void UpdateDynamics(uint32 diff);
        uint8   _dyn_max;
        uint32  _dyn_timer;
        bool    _dyn_enabled;
        
    private:
        ACE_Based::LockedQueue<ClientSession*, ACE_Thread_Mutex> _addSessQueue;     //Add Session to active List
        SessionMap                                               _sessions;
        SessionUpdater*                                          _sessionupdater;
        size_t                                                   _SessionThreadsCount;
        Queue                                                    _QueuedPlayer;

        typedef std::unordered_map<uint32, time_t> DisconnectMap;
        DisconnectMap _disconnects;

        uint32 _maxActiveSessionCount;
        uint32 _maxQueuedSessionCount;
        uint32 _PlayerCount;
        uint32 _MaxPlayerCount;
        uint32 _AllyCount;
        uint32 _HordeCount;
        uint32 _playerLimit;
};
#define sClientSessionMgr ACE_Singleton<ClientSessionMgr, ACE_Null_Mutex>::instance()
#endif
