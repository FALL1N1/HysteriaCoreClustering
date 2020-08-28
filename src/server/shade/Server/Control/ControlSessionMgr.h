#ifndef __CONTROLSESSIONMGR_H
#define __CONTROLSESSIONMGR_H

#include "Common.h"
#include "Timer.h"
#include <ace/Singleton.h>
#include <ace/Atomic_Op.h>
#include "QueryResult.h"
#include "Callback.h"
#include "SessionUpdater.h"
#include "ControlSession.h"

#include <map>
#include <set>
#include <list>


typedef std::unordered_map<uint32, ControlSession*> CtrlSessionMap;
typedef std::list<uint32> DiscoSessionList;

class ControlSessionMgr
{
    public:
        void ServerClose();

        void Update(uint32 diff);
        void AddSession(ControlSession *s) { addSessQueue.add(s); }

        ControlSession* findNodebyID(uint32 id);

        void SendGlobal(WorldPacket* packet);
        void SendToNodeType(uint8 Type, WorldPacket* packet);

    private:
        void AddSession_(ControlSession* s);

        ACE_Based::LockedQueue<ControlSession*, ACE_Thread_Mutex> addSessQueue;
        CtrlSessionMap m_sessions;

};
#define sControlSessionMgr ACE_Singleton<ControlSessionMgr, ACE_Null_Mutex>::instance()
#endif