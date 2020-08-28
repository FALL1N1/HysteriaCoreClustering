#include "SessionUpdater.h"
#include "ControlSession.h"
#include "ControlSessionMgr.h"

void ControlSessionMgr::ServerClose()
{
    while (!m_sessions.empty())
    {
        // not remove from queue, prevent loading new sessions
        delete m_sessions.begin()->second;
        m_sessions.erase(m_sessions.begin());
    }
}

void ControlSessionMgr::AddSession_(ControlSession* s)
{
    ASSERT (s);

    m_sessions[s->GetNodeId()] = s;
}

void ControlSessionMgr::Update(uint32 diff)
{
    ///- Add new sessions
    ControlSession* sess;
    while (addSessQueue.next(sess))
        AddSession_(sess);

    ///- Then send an update signal to remaining ones
    for (CtrlSessionMap::iterator itr = m_sessions.begin(), next; itr != m_sessions.end(); itr = next)
    {
        next = itr;
        ++next;

        ///- and remove not active sessions from the list
        ControlSession * pSession = itr->second;

        if (!pSession->Update(diff))    // As interval = 0
        {
            m_sessions.erase(itr);
            delete pSession;
        }
    }
}

ControlSession* ControlSessionMgr::findNodebyID(uint32 id)
{
    CtrlSessionMap::const_iterator itr = m_sessions.find(id);
    if (itr != m_sessions.end())
        return itr->second;
    else
        return NULL;
}

//Send to all Nodes
void ControlSessionMgr::SendGlobal(WorldPacket* packet)
{
    for (CtrlSessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); itr++)
        itr->second->SendPacket(packet);
}

//Send to specified Nodes
void ControlSessionMgr::SendToNodeType (uint8 Type, WorldPacket *packet)
{
    for (CtrlSessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); itr++)
    {
        if (itr->second->GetNodeType() == Type)
            itr->second->SendPacket(packet);
    }
}
