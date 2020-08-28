#include "PoolSession.h"
#include "PoolSessionMgr.h"
#include "ClusterDefines.h"

PoolSession* PoolSessionMgr::GetSession(uint32 id)
{
    PoolSessionMap::const_iterator itr;
    if (id == uint32(0))
    {
        itr = m_sessions.begin();
        return itr->second;
    }
    return NULL;
}

void PoolSessionMgr::SendGUIDSync(uint32 type, uint32 guid)
{
    PoolSessionMap::const_iterator itr;
    itr = m_sessions.begin();
    if (itr == m_sessions.end())
        return;

    WorldPacket packet(NODE_SYNC_DATA);
    packet << uint32 (CL_DEF_GUID_SYNC);
    packet << uint32 (type);
    packet << guid;
    itr->second->SendPacket(&packet);

    return;
}

void PoolSessionMgr::SendPacketToAll(const WorldPacket* packet)
{
    for (PoolSessionMap::iterator itr = m_sessions.begin(), next; itr != m_sessions.end(); ++itr)
        itr->second->SendPacket(packet);
}

void PoolSessionMgr::Initialize()
{
    m_PlayerCount = 0;
    m_MaxPlayerCount = 0;
}

void PoolSessionMgr::ServerClose()
{
    ///- Empty the kicked session set
    while (!m_sessions.empty())
    {
        // not remove from queue, prevent loading new sessions
        delete m_sessions.begin()->second;
        m_sessions.erase(m_sessions.begin());
    }
}

void PoolSessionMgr::AddSession(PoolSession* s)
{
    addSessQueue.add(s);
}

void PoolSessionMgr::AddQueuedPlayer(PoolSession* sess)
{
    m_QueuedPlayer.push_back(sess);
}

bool PoolSessionMgr::RemoveQueuedPlayer(PoolSession* sess)
{
    // sessions count including queued to remove (if removed_session set)
    uint32 sessions = GetActiveSessionCount();

    uint32 position = 1;
    Queue::iterator iter = m_QueuedPlayer.begin();

    // search to remove and count skipped positions
    bool found = false;

    for (; iter != m_QueuedPlayer.end(); ++iter, ++position)
    {
        if (*iter == sess)
        {
            iter = m_QueuedPlayer.erase(iter);
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
    if ((!m_playerLimit || sessions < m_playerLimit) && !m_QueuedPlayer.empty())
    {
        m_QueuedPlayer.pop_front();

        // update iter to point first queued socket or end() if queue is empty now
        iter = m_QueuedPlayer.begin();
        position = 1;
    }

    return found;
}

void PoolSessionMgr::AddSession_(PoolSession* s)
{
    ASSERT (s);

    //NOTE - Still there is race condition in PoolSession* being used in the Sockets

    ///- kick already loaded player with same account (if any) and remove session
    ///- if player is in loading and want to load again, return
    if (!RemoveSession (s->GetServerId()))
    {
        //s->KickPlayer();
        delete s;                                           // session not added yet in session list, so not listed in queue
        return;
    }

    // decrease session counts only at not reconnection case
    bool decrease_session = true;

    // if session already exist, prepare to it deleting at next world update
    // NOTE - KickPlayer() should be called on "old" in RemoveSession()
    {
        PoolSessionMap::const_iterator old = m_sessions.find(s->GetServerId());

        if (old != m_sessions.end())
        {
            // prevent decrease sessions count if session queued
            if (RemoveQueuedPlayer(old->second))
                decrease_session = false;
            // not remove replaced session form queue if listed
            delete old->second;
        }
    }

    m_sessions[s->GetServerId()] = s;

    uint32 Sessions = GetActiveAndQueuedSessionCount();
    uint32 pLimit = GetPlayerAmountLimit();
    uint32 QueueSize = GetQueuedSessionCount();             //number of players in the queue

    //so we don't count the user trying to
    //login as a session and queue the socket that we are using
    if (decrease_session)
        --Sessions; 

    //Its okay
    WorldPacket pack(SMSG_AUTH_RESPONSE);
    pack << uint16(AUTH_OK);
    s->SendPacket(&pack);

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

/// Remove a given session
bool PoolSessionMgr::RemoveSession(uint32 id)
{
    ///- Find the session, kick the user, but we can't delete session at this moment to prevent iterator invalidation
    PoolSessionMap::const_iterator itr = m_sessions.find(id);

    if (itr != m_sessions.end() && itr->second)
    {
        //itr->second->KickPlayer();
    }

    return true;
} 

int32 PoolSessionMgr::GetQueuePos(PoolSession* sess)
{
    uint32 position = 1;

    for (Queue::const_iterator iter = m_QueuedPlayer.begin(); iter != m_QueuedPlayer.end(); ++iter, ++position)
        if ((*iter) == sess)
            return position;

    return 0;
}

void PoolSessionMgr::UpdateMaxSessionCounters()
{
    m_maxActiveSessionCount = std::max(m_maxActiveSessionCount,uint32(m_sessions.size()-m_QueuedPlayer.size()));
    m_maxQueuedSessionCount = std::max(m_maxQueuedSessionCount,uint32(m_QueuedPlayer.size()));
}

void PoolSessionMgr::Update(uint32 diff)
{
    ///- Add new sessions
    PoolSession* sess;
    while (addSessQueue.next(sess))
        AddSession_ (sess);

    ///- Then send an update signal to remaining ones
    for (PoolSessionMap::iterator itr = m_sessions.begin(), next; itr != m_sessions.end(); itr = next)
    {
        next = itr;
        ++next;

        ///- and remove not active sessions from the list
        PoolSession * pSession = itr->second;

        if (!pSession->Update(diff))    // As interval = 0
        {
            //if (!RemoveQueuedPlayer(itr->second) && itr->second)// && sWorld->getIntConfig(CONFIG_INTERVAL_DISCONNECT_TOLERANCE))
                //m_disconnects[itr->second->GetServerId()] = time(NULL);
            RemoveQueuedPlayer(pSession);
            m_sessions.erase(itr);
            delete pSession;

        }
    }
}
