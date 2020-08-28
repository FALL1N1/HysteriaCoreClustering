#include "DatabaseEnv.h"
#include "Log.h"
#include "Config.h"
#include "RoutingHelper.h"
#include "ClientSession.h"
#include "Proxy.h"
#include "ClusterDefines.h"
#include "ControlSession.h"
#include "ControlSessionMgr.h"

void RoutingHelper::LoadNodeMap()
{
    QueryResult result = LogonDatabase.Query("SELECT * FROM map_table");

    int count = 0;
    if (!result)
    {
        sLog->outString();
        sLog->outString(">> Loaded %u Node assigns", count);
        return;
    }

    do
    {
        NodeMap m_nodemap;

        Field *fields = result->Fetch();
        uint32 Mapid = fields[0].GetUInt32();
        m_nodemap.NodeID = fields[1].GetUInt32();
        m_nodemap.BackupNodeID = fields[2].GetUInt32();
        m_nodemap.SubNodeID = fields[3].GetUInt32();
        m_nodemap.Name = fields[4].GetString();
        m_nodemap.Color = fields[5].GetString(); 

        m_maplist[Mapid] = m_nodemap;

        ++count;

    }
    while (result->NextRow());

    sLog->outString();
    sLog->outString(">> Loaded %u Node assigns", count);

    MasterNode = 1;// GetFirstNode();
    m_OnMaster = true;
}

int32 RoutingHelper::GetNodeForMap(uint32 Map_ID)
{
    NodeMapList::const_iterator iter = m_maplist.find(Map_ID);
    if (iter == m_maplist.end())
        return -1;
    else
        return iter->second.NodeID;
}

std::string RoutingHelper::GetNameForMapNode(uint32 m)
{
    NodeMapList::iterator iter = m_maplist.begin();
    for (; iter != m_maplist.end(); ++iter)
        if (iter->second.NodeID == m)
            return iter->second.Name;

    return "|cffFF0000 error |r";
}

std::string RoutingHelper::GetColorForMapNode(uint32 m)
{
    NodeMapList::iterator iter = m_maplist.begin();
    for (; iter != m_maplist.end(); ++iter)
        if (iter->second.NodeID == m)
            return iter->second.Color;

    return "|cffFF0000 error |r";
}

uint32 RoutingHelper::GetBackupNodeForMap(uint32 Map_ID)
{
    NodeMapList::const_iterator iter = m_maplist.find(Map_ID);
    if (iter == m_maplist.end())
        return 0;
    else
        return iter->second.SubNodeID;
}

uint32 RoutingHelper::GetNodeForID(uint32 Node_ID)
{
    NodeMapList::iterator iter = m_maplist.begin();
    for (; iter != m_maplist.end(); ++iter)
        if (iter->second.BackupNodeID == Node_ID)
            return iter->second.NodeID;

    return 0;
}

uint32 RoutingHelper::GetBackupNodeForID(uint32 Node_ID)
{
    NodeMapList::iterator iter = m_maplist.begin();
    for (; iter != m_maplist.end(); ++iter)
        if (iter->second.NodeID == Node_ID)
            return iter->second.BackupNodeID;

    return 0;
}

uint32 RoutingHelper::GetSubNodeForMap(uint32 Map_ID)
{
    NodeMapList::const_iterator iter = m_maplist.find(Map_ID);
    if (iter == m_maplist.end())
        return 0;
    else
        return iter->second.SubNodeID;
}
/*
bool RoutingHelper::GoToMapNode(ClientSession *m_Session, uint32 MapID)
{
    NodeMapList::const_iterator iter = m_maplist.find(MapID);
    if (iter == m_maplist.end())
        return false;
    else
    {
        if (CheckNodeID(iter->second.NodeID))
            m_Session->SendToNode(iter->second.NodeID, MapID);
        else if (CheckNodeID(iter->second.BackupNodeOnline))
            m_Session->SendToNode(iter->second.BackupNodeID, MapID);
        return true;
    }

    return false;
}
*/

//Master ist immer Node1 bis der Logon alles selbst kann
//Wenn Node1 tot ist gehts zum Backup
uint32 RoutingHelper::ConnectToMaster()
{
    MasterNode = 1;
    if (CheckNodeID(MasterNode)) {
        //sLog->outString("[1] CheckNodeID(%u)", MasterNode);
        return MasterNode;
    }
    else
    {
        if (m_OnMaster)
        {
            uint32 NewMasterNode = GetBackupNodeForID(MasterNode);
            if ((NewMasterNode > 0) && CheckNodeID(NewMasterNode))
            {
                //sLog->outString("[2] ConnectToMasterNode(%u)", MasterNode);
                MasterNode = NewMasterNode;
                m_OnMaster = false;
                return MasterNode;
            }
        }
        else
        {
            uint32 NewMasterNode = GetNodeForID(MasterNode);
            if ((NewMasterNode > 0) && CheckNodeID(NewMasterNode))
            {
                //sLog->outString("[3] ConnectToMasterNode(%u)", MasterNode);
                MasterNode = NewMasterNode;
                m_OnMaster = true;
                return MasterNode;
            }
        }

    }
    return 0;
}

void RoutingHelper::LoadNodeList()
{
    QueryResult result = LogonDatabase.Query("SELECT * FROM nodelist");

    int count = 0;
    if (!result)
    {
        sLog->outString();
        sLog->outString(">> Loaded %u Nodes ", count);
        return;
    }

    do
    {
        NodeList m_nodes;

        Field *fields = result->Fetch();
        uint32 NodeID = fields[0].GetUInt32();
        m_nodes.name = fields[1].GetString();
        m_nodes.IP = fields[2].GetString();
        m_nodes.port = fields[3].GetInt32();
        m_nodes.controlport = fields[4].GetInt32();
        m_nodes.online = fields[5].GetBool();
        m_nodes.type = fields[6].GetUInt8();
        m_nodes.LastItemGUID = 0;
        m_nodes.LastMailGUID = 0;
        m_nodes.LastGroupGUID = 0;
        m_nodelist[NodeID] = m_nodes;

        ++count;

    }
    while (result->NextRow());

    sLog->outString();
    sLog->outString(">> Loaded %u Nodes", count);
}

bool RoutingHelper::CheckNodeID(uint32 NodeID)
{
    NodeListList::const_iterator iter = m_nodelist.find(NodeID);

    if (iter == m_nodelist.end())
        return false;

    if (iter->second.online)
        return true;

    return false;
}

bool RoutingHelper::CanEnterNodeID(uint32 NodeID)
{
    NodeListList::const_iterator iter = m_nodelist.find(NodeID);

    if (iter == m_nodelist.end())
        return false;

    if (iter->second.can_enter)
        return true;

    return false;
}

uint32 RoutingHelper::GetFirstNode()
{
    return 1;
}

NodeList RoutingHelper::GetNodeConnectionData(uint32 NodeID)
{
    NodeList m_return;
    NodeListList::const_iterator iter = m_nodelist.find(NodeID);

    if (iter == m_nodelist.end())
        return m_return;
    else
        return iter->second;
}

uint32 RoutingHelper::GetNodeCount()
{
    uint32 count = 0;
    for (NodeListList::iterator iter = m_nodelist.begin(); iter != m_nodelist.end(); ++iter)
        count = std::max(count, iter->first);

    return count;
}

void RoutingHelper::SetNodeOnline(uint32 Node)
{
    TRINITY_GUARD(ACE_Thread_Mutex, i_Lock);
    {
        NodeListList::iterator iter = m_nodelist.find(Node);

        if (iter == m_nodelist.end())
            return;

        m_nodelist[iter->first].can_enter = true;
    }
    return;
}

void RoutingHelper::SetNodeOffline(uint32 Node)
{
    TRINITY_GUARD(ACE_Thread_Mutex, i_Lock);
    {
        NodeListList::iterator iter = m_nodelist.find(Node);

        if (iter == m_nodelist.end())
            return;

        m_nodelist[iter->first].online = false;
        m_nodelist[iter->first].can_enter = false;
    }
    return;
}

void RoutingHelper::SetLastGUID(uint32 Node, uint32 type, uint32 GUID)
{
    NodeListList::iterator iter = m_nodelist.find(Node);

    if (iter == m_nodelist.end())
        return;

    switch (type)
    {
        case ENTITYID_ITEM:
            m_nodelist[iter->first].LastItemGUID = GUID;
            break;
        case ENTITYID_MAIL:
            m_nodelist[iter->first].LastMailGUID = GUID;
            break;
        case ENTITYID_GROUP:
            m_nodelist[iter->first].LastGroupGUID = GUID;
            break;
        case ENTITYID_CORPSE:
            m_nodelist[iter->first].LastCorpseID = GUID;
            break;
    }
}

void RoutingHelper::ResetLastGUID()
{
    for (NodeListList::iterator itr = m_nodelist.begin(); itr != m_nodelist.end(); itr++)
    {
        m_nodelist[itr->first].LastItemGUID = 0;
        m_nodelist[itr->first].LastMailGUID = 0;
        m_nodelist[itr->first].LastGroupGUID = 0;
    }
}

void RoutingHelper::RefreshOnlineStat()
{
    TRINITY_GUARD(ACE_Thread_Mutex, i_Lock);
    {
        NodeListList::iterator iter = m_nodelist.begin();
        for (; iter != m_nodelist.end(); ++iter)
        {
            if (!iter->second.online)
            {
                QueryResult result = LogonDatabase.PQuery("SELECT * FROM nodelist WHERE NodeID = %u;", iter->first);
                Field *fields = result->Fetch();

                m_nodelist[iter->first].online = fields[5].GetBool();
            }
        }
    }
}

void RoutingHelper::SetupControler()
{
    NodeListList::iterator iter = m_nodelist.begin();
    for (; iter != m_nodelist.end(); ++iter)
    {
        ControlSession *m_Sess = new ControlSession(iter->first, iter->second.type);
        sControlSessionMgr->AddSession(m_Sess);
    }
}
