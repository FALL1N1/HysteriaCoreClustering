#include "Network.h"

#include <ace/TP_Reactor.h>
#include <ace/Dev_Poll_Reactor.h>
#include <ace/Basic_Types.h>
#include <ace/os_include/netinet/os_tcp.h>

#include "Socket.h"
#include "Config.h"
#include "Log.h"

ACE_THR_FUNC_RETURN event_loop(void *arg)
{
    ACE_Reactor *reactor = (ACE_Reactor *) arg;

    while (!reactor->reactor_event_loop_done())
    {
        ACE_Time_Value interval(0, 10000);

        if (reactor->handle_events(interval) < 0)
            return 0;
    }

   return 0;
}

int NetworkMgr::open()
{
    _compressionLevel = sConfigMgr->GetIntDefault("Compression", 1);

    _useNoDelay = sConfigMgr->GetBoolDefault("Network.TcpNodelay", true);

    // -1 means use default
    _sockOutKBuff = sConfigMgr->GetIntDefault ("Network.OutKBuff", -1);
    _sockOutUBuff = sConfigMgr->GetIntDefault ("Network.OutUBuff", 65536);
    if (_sockOutUBuff <= 0)
    {
        sLog->outError("Network.OutUBuff is wrong in your config file");
        return -1;
    }

    _threadsCount = sConfigMgr->GetIntDefault("Network.Threads", 1);
    if (_threadsCount <= 0)
    {
        sLog->outError("Network.Threads is wrong in your config file");
        return -1;
    }

    ACE_Reactor_Impl* imp = 0;

#if defined (ACE_HAS_EVENT_POLL) || defined (ACE_HAS_DEV_POLL)

    imp = new ACE_Dev_Poll_Reactor();

    imp->max_notify_iterations (128);
    imp->restart (1);

#else

    imp = new ACE_TP_Reactor();
    imp->max_notify_iterations(128);

#endif

    _reactor = new ACE_Reactor(imp, 1);

    _threadGroup = ACE_Thread_Manager::instance()->spawn_n(_threadsCount, event_loop, _reactor);

    sLog->outString("Max allowed connections %d", ACE::max_handles());
    return 0;
}

void NetworkMgr::close()
{
    _reactor->end_reactor_event_loop();

    wait();
}

void NetworkMgr::wait()
{
    ACE_Thread_Manager::instance()->wait_grp(_threadGroup);
}

int NetworkMgr::GetCompressionLevel() const
{
    return this->_compressionLevel;
}

int NetworkMgr::GetSockOutKBuff() const
{
    return _sockOutKBuff;
}

int NetworkMgr::GetSockOutUBuff() const
{
    return _sockOutUBuff;
}

bool NetworkMgr::UseNoDelay() const
{
    return _useNoDelay;
}

ACE_Reactor * NetworkMgr::reactor()
{
    return _reactor;
}

NetworkMgr::NetworkMgr() :
    _threadsCount(0),
    _threadGroup(-1),
    _reactor(NULL),
    _sockOutKBuff(-1),
    _sockOutUBuff(65536),
    _useNoDelay(true),
    _compressionLevel(0)
{
}

NetworkMgr::~NetworkMgr()
{
    delete _reactor;
}
