#ifndef _MAPRELATIONS_H
#define _MAPRELATIONS_H

//MapHolder
struct NodeMap
{
    uint32 NodeID;
    bool NodeOnline;
    uint32 BackupNodeID;
    bool BackupNodeOnline;
    uint32 Master;

    //Used for expand Maps
    uint32 SubNodeID;
    bool SubNodeOnline;
};
typedef std::unordered_map<uint32, NodeMap> NodeMapList;

class MapRelations
{
    public:

        void LoadNodeMap();

        uint32 GetNodeForMap(uint32 Map_ID);
        uint32 GetNodeForID(uint32 Node_ID);
        uint32 GetBackupNodeForMap(uint32 Map_ID);
        uint32 GetBackupNodeForID(uint32 Node_ID);
        uint32 GetSubNodeForMap(uint32 Map_ID);
       
    private:
        NodeMapList m_maplist;
};

#define sMapRelations ACE_Singleton<MapRelations, ACE_Thread_Mutex>::instance()

#endif
