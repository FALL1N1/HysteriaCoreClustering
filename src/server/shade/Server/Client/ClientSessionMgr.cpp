#include "ClientSessionMgr.h"
#include "SessionUpdater.h"
#include "ClientSession.h"
#include "AccountMgr.h"

void ClientSessionMgr::Initialize()
{
    _PlayerCount = 0;
    _AllyCount = 0;
    _HordeCount = 0;
    _MaxPlayerCount = 0;

    _maxActiveSessionCount = 0;
    _maxQueuedSessionCount = 0;

    _dyn_max = sLogon->getIntConfig(CONFIG_SESSION_DYN_MAX);
    _dyn_timer = sLogon->getIntConfig(CONFIG_SESSION_DYN_TIMER);
    _dyn_enabled = sLogon->getBoolConfig(CONFIG_SESSION_DYN_ENABLED);

    Start();
}

void ClientSessionMgr::Start()
{
    // @emo
    int num_threads = sLogon->getIntConfig(CONFIG_SESSION_MT_THREADS);
    if (num_threads <= 0)
        num_threads = 1;

    _SessionThreadsCount = static_cast<size_t> (num_threads);
    _sessionupdater = new SessionUpdater[_SessionThreadsCount];

    sLog->outBasic ("Max allowed Sessions %d", ACE::max_handles());

    for (size_t i = 0; i < _SessionThreadsCount; ++i)
        _sessionupdater[i].Start();
}

void ClientSessionMgr::ServerClose()
{
    ///- Empty the kicked session set
    while (!_sessions.empty())
    {
        // not remove from queue, prevent loading new sessions
        _sessions.begin()->second->remove_reference();
        _sessions.erase(_sessions.begin());
    }

    for (size_t i = 0; i < _SessionThreadsCount; ++i)
        _sessionupdater[i].Stop();
}


/*********************************************************/
/***                 MISC HANDLING                     ***/
/*********************************************************/
bool ClientSessionMgr::HasRecentlyDisconnected(ClientSession* session)
{
    return false;
    /*
    if (!session)
        return false;

    if (uint32 tolerance = sLogon->getIntConfig(CONFIG_INTERVAL_DISCONNECT_TOLERANCE))
    {
        for (DisconnectMap::iterator i = _disconnects.begin(); i != _disconnects.end();)
        {
            if (difftime(i->second, time(NULL)) < tolerance)
            {
                if (i->first == session->GetAccountId())
                    return true;
                ++i;
            }
            else
                _disconnects.erase(i);
        }
    }
    return false;*/
 }

void ClientSessionMgr::UpdateMaxSessionCounters()
{
    _maxActiveSessionCount = std::max(_maxActiveSessionCount,uint32(_sessions.size()-_QueuedPlayer.size()));
    _maxQueuedSessionCount = std::max(_maxQueuedSessionCount,uint32(_QueuedPlayer.size()));
}

/*********************************************************/
/***                QUEUE HANDLING                     ***/
/*********************************************************/
void ClientSessionMgr::AddQueuedPlayer(ClientSession* sess)
{
    sess->SetInQueue(true);
    _QueuedPlayer.push_back(sess);

    // The 1st SMSG_AUTH_RESPONSE needs to contain other info too.
    sess->SendAuthResponse(AUTH_WAIT_QUEUE, false, GetQueuePos(sess));
}

bool ClientSessionMgr::RemoveQueuedPlayer(ClientSession* sess)
{
    // sessions count including queued to remove (if removed_session set)
    uint32 sessions = GetActiveSessionCount();

    uint32 position = 1;
    Queue::iterator iter = _QueuedPlayer.begin();

    // search to remove and count skipped positions
    bool found = false;

    for (; iter != _QueuedPlayer.end(); ++iter, ++position)
    {
        if (*iter == sess)
        {
            sess->SetInQueue(false);
            iter = _QueuedPlayer.erase(iter);
            found = true;                                   // removing queued session
            break;
        }
    }

    // iter point to next socked after removed or end()
    // position store position of removed socket and then new position next socket after removed

    // if session not queued then we need decrease sessions count
    if (!found && sessions)
        --sessions;

    // accept first in queue
    if ((!_playerLimit || sessions < _playerLimit) && !_QueuedPlayer.empty())
    {
        ClientSession* pop_sess = _QueuedPlayer.front();
        pop_sess->SetInQueue(false);
        pop_sess->SendAuthWaitQue(0);
        pop_sess->SendAddonsInfo();
        pop_sess->SendClientCacheVersion(sLogon->getIntConfig(CONFIG_CLIENTCACHE_VERSION));

        _QueuedPlayer.pop_front();

        // update iter to point first queued socket or end() if queue is empty now
        iter = _QueuedPlayer.begin();
        position = 1;
    }

    // update position from iter to end()
    // iter point to first not updated socket, position store new position
    for (; iter != _QueuedPlayer.end(); ++iter, ++position)
        (*iter)->SendAuthWaitQue(position);

    return found;
}

int32 ClientSessionMgr::GetQueuePos(ClientSession* sess)
{
    uint32 position = 1;

    for (Queue::const_iterator iter = _QueuedPlayer.begin(); iter != _QueuedPlayer.end(); ++iter, ++position)
        if ((*iter) == sess)
            return position;

    return 0;
}

/*********************************************************/
/***        EXTERNAL SESSION HANDLING SYSTEM           ***/
/*********************************************************/
void ClientSessionMgr::AddSession(ClientSession* s)
{
    s->add_reference();
    _addSessQueue.add(s);
}

void ClientSessionMgr::Update(uint32 diff)
{
    ///- Add new sessions
    ClientSession* sess;
    while (_addSessQueue.next(sess))
    {
        //Shouldn't happen, but if...
        if (sess)
            AddSession_ (sess);
    }

    //Update the remaining ones
    for (SessionMap::iterator itr = _sessions.begin(); itr != _sessions.end();)
    {
        ClientSession* pSession = itr->second;

        if (pSession->UpdateSingle(diff))
            ++itr;
        else
        {
            pSession->KickPlayer();
            pSession->remove_reference();
            _sessions.erase(itr++);
        }
    }

    //Update the Dynamics
    if (_dyn_enabled)
        UpdateDynamics(diff);
}

bool ClientSessionMgr::RemoveSession(uint32 id)
{
    SessionMap::const_iterator itr = _sessions.find(id);

    if (itr != _sessions.end() && itr->second)
        itr->second->KickPlayer();

    return true;
}

ClientSession* ClientSessionMgr::FindSession(uint32 id) const
{
    SessionMap::const_iterator itr = _sessions.find(id);

    if (itr != _sessions.end())
        return itr->second;                                 // also can return NULL for kicked session
    else
        return NULL;
}

/*********************************************************/
/***        INTERNAL SESSION HANDLING SYSTEM           ***/
/*********************************************************/
void ClientSessionMgr::AddSession_(ClientSession* s)
{
    ASSERT (s);

    //NOTE - Still there is race condition in ClientSession* being used in the Sockets

    ///- kick already loaded player with same account (if any) and remove session
    ///- if player is in loading and want to load again, return
    if (!RemoveSession (s->GetAccountId()))
        return;

    // decrease session counts only at not reconnection case
    bool decrease_session = true;

    // if session already exist, prepare to it deleting at next world update
    // NOTE - KickPlayer() should be called on "old" in RemoveSession()
    {
        SessionMap::const_iterator old = _sessions.find(s->GetAccountId());

        if (old != _sessions.end())
        {
            // prevent decrease sessions count if session queued
            if (RemoveQueuedPlayer(old->second))
                decrease_session = false;
        }
    }

    _sessions[s->GetAccountId()] = s;

    //Give the SessionUpdate to the Updater
    size_t min = 0;
    ACE_ASSERT (_SessionThreadsCount >= 1);
    if (_dyn_enabled)
    {
        if(_sessionupdater[0].Counter() >= _dyn_max)
        {
            for (size_t i = 0; i < _SessionThreadsCount; ++i)
            {
                if (_sessionupdater[i].Counter() < _sessionupdater[min].Counter())
                    min = i;
            }
        }
    }
    else
    {
        for (size_t i = 0; i < _SessionThreadsCount; ++i)
        {
            if (_sessionupdater[i].Counter() < _sessionupdater[min].Counter())
                min = i;
        }
    }
    _sessionupdater[min].AddSession(s);

    uint32 Sessions = GetActiveAndQueuedSessionCount();
    uint32 pLimit = GetPlayerAmountLimit();
    uint32 QueueSize = GetQueuedSessionCount();             //number of players in the queue

    //so we don't count the user trying to
    //login as a session and queue the socket that we are using
    if (decrease_session)
        --Sessions;

    if (pLimit > 0 && Sessions >= pLimit && s->GetSecurity() == SEC_PLAYER && !HasRecentlyDisconnected(s))
    {
        AddQueuedPlayer (s);
        UpdateMaxSessionCounters();
        sLog->outDetail ("PlayerQueue: Account id %u is in Queue Position (%u).", s->GetAccountId(), ++QueueSize);
        return;
    }

    s->SendAuthResponse(AUTH_OK, true);

    s->SendAddonsInfo();

    //s->SendClientCacheVersion(sLogon->getIntConfig(CONFIG_CLIENTCACHE_VERSION));
    //s->SendTutorialsData();

    UpdateMaxSessionCounters();

    // Updates the population
    if (pLimit > 0)
    {
        float popu = (float)GetActiveSessionCount();              // updated number of users on the server
        popu /= pLimit;
        popu *= 2;
        sLog->outDetail ("Server Population (%f).", popu);
    }
}

/*********************************************************/
/***                 BROADCAST SYSTEM                  ***/
/*********************************************************/
void ClientSessionMgr::ShutdownMsg(uint32 m_ShutdownTimer, uint32 m_ShutdownMask, bool show, Player* player)
{
    // not show messages for idle shutdown mode
    if (m_ShutdownMask & SHUTDOWN_MASK_IDLE)
        return;

    ///- Display a message every 12 hours, hours, 5 minutes, minute, 5 seconds and finally seconds
    if (show ||
        (m_ShutdownTimer < 5* MINUTE && (m_ShutdownTimer % 15) == 0) || // < 5 min; every 15 sec
        (m_ShutdownTimer < 15 * MINUTE && (m_ShutdownTimer % MINUTE) == 0) || // < 15 min ; every 1 min
        (m_ShutdownTimer < 30 * MINUTE && (m_ShutdownTimer % (5 * MINUTE)) == 0) || // < 30 min ; every 5 min
        (m_ShutdownTimer < 12 * HOUR && (m_ShutdownTimer % HOUR) == 0) || // < 12 h ; every 1 h
        (m_ShutdownTimer > 12 * HOUR && (m_ShutdownTimer % (12 * HOUR)) == 0)) // > 12 h ; every 12 h
    {

        std::string out = "ProxyServer is ";
        out.append(m_ShutdownMask & SHUTDOWN_MASK_RESTART ? "restarting" : "shutting down");
        out.append(" in: ");
        std::string str = secsToTimeString(m_ShutdownTimer);
        out.append(str);

        SendServerMessage(SERVER_MSG_STRING, out.c_str(), player);

        sLog->outStaticDebug("Server is %s in %s", (m_ShutdownMask & SHUTDOWN_MASK_RESTART ? "restart" : "shuttingdown"), str.c_str());
    }
}

void ClientSessionMgr::HaltMsg(uint32 m_HaltTimer, uint32 m_ShutdownMask, bool show, Player* player)
{
    return;
    ///- Display a message every 12 hours, hours, 5 minutes, minute, 5 seconds and finally seconds
    if (show ||
        (m_HaltTimer < 5* MINUTE && (m_HaltTimer % 15) == 0) || // < 5 min; every 15 sec
        (m_HaltTimer < 15 * MINUTE && (m_HaltTimer % MINUTE) == 0) || // < 15 min ; every 1 min
        (m_HaltTimer < 30 * MINUTE && (m_HaltTimer % (5 * MINUTE)) == 0) || // < 30 min ; every 5 min
        (m_HaltTimer < 12 * HOUR && (m_HaltTimer % HOUR) == 0) || // < 12 h ; every 1 h
        (m_HaltTimer > 12 * HOUR && (m_HaltTimer % (12 * HOUR)) == 0)) // > 12 h ; every 12 h
    {
        std::string out = "Server will be taken down for maintenance within ";
        std::string str = secsToTimeString(m_HaltTimer);
        out.append(str);
        out.append(" This may take 3 minutes till 3 hours.");

        SendServerMessage(SERVER_MSG_STRING, out.c_str(), player);
        sLog->outStaticDebug("Server is shutting down in %s", str.c_str());
    }
}

/// Send a server message to the user(s)
void ClientSessionMgr::SendServerMessage(ServerMessageType type, const char *text, Player* player)
{
    WorldPacket data(SMSG_SERVER_MESSAGE, 50);              // guess size
    data << uint32(type);
    if (type <= SERVER_MSG_STRING)
        data << text;

    if (player)
        player->GetSession()->SendPacket(&data);
    else
        SendGlobalMessage(&data);
}

/// Send a packet to all players (except self if mentioned)
void ClientSessionMgr::SendGlobalMessage(WorldPacket* packet, ClientSession* self, uint32 team)
{
    SessionMap::const_iterator itr;
    for (itr = _sessions.begin(); itr != _sessions.end(); ++itr)
    {
        if (itr->second &&
            itr->second->GetPlayer() &&
            itr->second != self &&
            (team == 0 || itr->second->GetPlayer()->GetTeam() == team))
        {
            itr->second->SendPacket(packet);
        }
    }
}

/// Send a server message to the GM(s)
void ClientSessionMgr::SendGameMasterMessage(const char* text, ...)
{
    va_list ap;
    char str[2048];

    va_start(ap, text);
    vsnprintf(str, 2048, text, ap);
    va_end(ap);

    WorldPacket data(SMSG_SERVER_MESSAGE, 50); // guess size
    data << uint32(SERVER_MSG_STRING);
    data << str;

    for (SessionMap::iterator itr = _sessions.begin(); itr != _sessions.end(); ++itr)
    {
        if (!itr->second || !itr->second->GetPlayer())
            continue;

        if (AccountMgr::IsVIPorPlayer(itr->second->GetSecurity()))
            continue;

        itr->second->SendPacket(&data);
    }
}

/*********************************************************/
/***              DYNAMIC CONTROLER                    ***/
/*********************************************************/

void ClientSessionMgr::UpdateDynamics(uint32 diff)
{
    ///- Check
    if (_SessionThreadsCount == 1)
        return;

    ///- Is timer expired?
    if (_dyn_timer >= diff)
    {
        _dyn_timer -= diff;
        return;
    }
    _dyn_timer = sLogon->getIntConfig(CONFIG_SESSION_DYN_TIMER);

    ///- Is dyn Value reached
    if (_sessions.size() >=  _dyn_max)
        return;

    ///- Lets check if any other thread than 0/1 have any sessions
    bool t_dyn = true;
    ACE_ASSERT (_SessionThreadsCount >= 1);

    for (size_t i = 1; i < _SessionThreadsCount; ++i)
    {
        if (_sessionupdater[i].Counter() == 0)
            t_dyn = false;
    }

    if (!t_dyn)
        return;

    for (size_t i = 0; i < _SessionThreadsCount; ++i)
        _sessionupdater[i].Sleep();

    //Update the remaining ones
    for (SessionMap::iterator itr = _sessions.begin(), next; itr != _sessions.end(); ++itr)
    {
        ClientSession* pSession = itr->second;
        _sessionupdater[0].AddSession(pSession);
    }
    return;
}