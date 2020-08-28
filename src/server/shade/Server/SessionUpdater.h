#ifndef _SESSION_UPDATER
#define _SESSION_UPDATER

#include <ace/Thread_Mutex.h>
#include <ace/Condition_Thread_Mutex.h>

#include "DelayExecutor.h"

class ClientSession;

class SessionUpdater : protected ACE_Task_Base
{
    public:
        SessionUpdater();
        virtual ~SessionUpdater();
        void Stop();
        int Start();
        void Sleep();

        void Wait() { ACE_Task_Base::wait(); }

        long Counter() { return static_cast<long> (m_Counter.value()); }
        int AddSession (ClientSession* sess);

    protected:
        void AddNewSessions();
        void SetSleep();
        virtual int svc();

    private:
        typedef ACE_Atomic_Op<ACE_SYNCH_MUTEX, long> AtomicInt;
        typedef std::set<ClientSession*> SessionSet;
        typedef ACE_Based::LockedQueue<ClientSession*, ACE_RW_Mutex> SessionQueue;

        AtomicInt m_Counter;
        int m_ThreadId;

        SessionQueue _newSessions;
        SessionSet m_Sessions;

        bool _sleep;
        bool _stop;
};

#endif 
