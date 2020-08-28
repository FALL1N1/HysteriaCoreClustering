#include "DatabaseEnv.h"
#include "Log.h"
#include "Config.h"
#include "MapRelations.h"
#include "World.h"


void MapRelations::LoadNodeMap()
{
    QueryResult result = LogonDatabase.Query( "SELECT * FROM map_table" );

    int count = 0;
    if ( !result )
    {
        sLog->outString();
        sLog->outString( ">> Loaded %u Node assigns", count );
        return;
    }

    do
    {
        NodeMap m_nodemap;

        Field *fields = result->Fetch();
        uint32 Mapid     = fields[0].GetUInt32();
        m_nodemap.NodeID = fields[1].GetUInt32();
        m_nodemap.BackupNodeID = fields[2].GetUInt32();
        m_nodemap.SubNodeID = fields[3].GetUInt32();

        m_maplist[Mapid] = m_nodemap;

        ++count;

    }while (result->NextRow());

    sLog->outString();
    sLog->outString( ">> Loaded %u Node assigns", count );
}

uint32 MapRelations::GetNodeForMap(uint32 Map_ID)
{
    NodeMapList::const_iterator iter = m_maplist.find(Map_ID);
    if (iter == m_maplist.end())
        return 0;
    else
        return iter->second.NodeID;
}

uint32 MapRelations::GetBackupNodeForMap(uint32 Map_ID)
{
    NodeMapList::const_iterator iter = m_maplist.find(Map_ID);
    if (iter == m_maplist.end())
        return 0;
    else
        return iter->second.SubNodeID;
}

uint32 MapRelations::GetNodeForID(uint32 Node_ID)
{
    NodeMapList::iterator iter = m_maplist.begin();
    for (; iter != m_maplist.end(); ++iter)
        if (iter->second.BackupNodeID == Node_ID)
            return iter->second.NodeID;

    return 0;
}

uint32 MapRelations::GetBackupNodeForID(uint32 Node_ID)
{
    NodeMapList::iterator iter = m_maplist.begin();
    for (; iter != m_maplist.end(); ++iter)
        if (iter->second.NodeID == Node_ID)
            return iter->second.BackupNodeID;

    return 0;
}

uint32 MapRelations::GetSubNodeForMap(uint32 Map_ID)
{
    NodeMapList::const_iterator iter = m_maplist.find(Map_ID);
    if (iter == m_maplist.end())
        return 0;
    else
        return iter->second.SubNodeID;
}
