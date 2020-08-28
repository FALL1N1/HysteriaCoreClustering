#include "SessionUpdater.h"
#include "DelayExecutor.h"
#include "ClientSession.h"
#include "ClientSessionMgr.h"
#include "DatabaseEnv.h"

#include <ace/ACE.h>
#include <ace/Log_Msg.h>
#include <ace/Reactor.h>
#include <ace/Reactor_Impl.h>
#include <ace/TP_Reactor.h>
#include <ace/Dev_Poll_Reactor.h>
#include <ace/Guard_T.h>
#include <ace/Atomic_Op.h>
#include <ace/os_include/arpa/os_inet.h>
#include <ace/os_include/netinet/os_tcp.h>
#include <ace/os_include/sys/os_types.h>
#include <ace/os_include/sys/os_socket.h>

SessionUpdater::SessionUpdater() :
    m_Counter(0),
    m_ThreadId(-1)
{
    _sleep = false;
    _stop = false;
}

SessionUpdater::~SessionUpdater()
{
    Stop();
    Wait();
}

void SessionUpdater::Stop()
{
    _stop = true;
}

int SessionUpdater::Start()
{
    if (m_ThreadId != -1)
        return -1;

    return (m_ThreadId = activate());
}

void SessionUpdater::Sleep()
{
    _sleep = true;
}

int SessionUpdater::AddSession(ClientSession* session)
{
    ++m_Counter;
    session->add_reference();
    _newSessions.add(session);
    return 0;
}

void SessionUpdater::AddNewSessions()
{
    ClientSession *session = NULL;

    while (_newSessions.next(session))
        m_Sessions.insert(session);
}

void SessionUpdater::SetSleep()
{
    _sleep = false;
    if (!_newSessions.empty())
        return;

    m_Counter = 0;
    m_Sessions.clear();
}

#define LOGON_SESSION_SLEEP_CONST 5000 //micro-seconds

int SessionUpdater::svc()
{
    uint32 realCurrTime = 0;
    uint32 realPrevTime = getUSTime();

    uint32 prevSleepTime = 0; 

    while (!_stop)
    { 
        realCurrTime = getUSTime();
        uint32 diff = getUSTimeDiff(realPrevTime, realCurrTime);

        if (_sleep)
            SetSleep();

        AddNewSessions();

        if (m_Sessions.empty())
            ACE_OS::sleep(ACE_Time_Value(0, 500000));

        for (SessionSet::iterator itr = m_Sessions.begin(); itr != m_Sessions.end();)
        {
            if (!(*itr))
            {
                m_Sessions.erase(itr++);
                --m_Counter;
            }
            else if (!(*itr)->UpdateMulti(diff))
            {
                (*itr)->remove_reference();
                m_Sessions.erase(itr++);
                --m_Counter;
            }
            else
                ++itr;
        }

        realPrevTime = realCurrTime;
        if (diff <= LOGON_SESSION_SLEEP_CONST + prevSleepTime)
        {
            prevSleepTime = LOGON_SESSION_SLEEP_CONST + prevSleepTime - diff;
            ACE_OS::sleep(ACE_Time_Value(0, prevSleepTime));
        }
        else
            prevSleepTime = 1;

    }
    return 0;
}