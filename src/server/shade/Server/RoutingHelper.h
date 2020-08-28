#ifndef ROUTING_HELPER_H
#define ROUTING_HELPER_H

#include "Define.h"
#include <ace/Singleton.h>
#include <ace/Thread_Mutex.h>
#include "Common.h"
#include <unordered_map>

class ClientSession;

//MapHolder
struct NodeMap
{
    uint32 NodeID;
    bool NodeOnline;
    uint32 BackupNodeID;
    bool BackupNodeOnline;
    uint32 Master;
    std::string Name;
    std::string Color;

    //Used for expand Maps
    uint32 SubNodeID;
    bool SubNodeOnline;
};
typedef std::unordered_map<uint32, NodeMap> NodeMapList;

struct NodeList
{
    std::string name;
    std::string IP;
    uint16 port;
    uint16 controlport;
    bool online;
    bool can_enter;
    uint8 type;

    //GuID-Holder
    uint32 LastItemGUID;   // The last used ItemGUID
    uint32 LastMailGUID;   // The last used MailGUID
    uint32 LastGroupGUID;  // The last used GroupGUID
    uint32 LastCorpseID;   // The last used CorpseID
};

typedef std::unordered_map<uint32, NodeList> NodeListList;

class RoutingHelper
{
public:
    void SetupControler();

    void LoadNodeMap();

    int32 GetNodeForMap(uint32 Map_ID);
    std::string GetNameForMapNode(uint32 m);
    std::string GetColorForMapNode(uint32 m);
    uint32 GetNodeForID(uint32 Node_ID);
    uint32 GetBackupNodeForMap(uint32 Map_ID);
    uint32 GetBackupNodeForID(uint32 Node_ID);
    uint32 GetSubNodeForMap(uint32 Map_ID);

    bool GoToMapNode(ClientSession *m_Session, uint32 MapID);
    uint32 ConnectToMaster();

    /// Load NodeList
    void LoadNodeList();
    /// FindNodeID
    bool CheckNodeID(uint32 NodeID);
    bool CanEnterNodeID(uint32 NodeID);
    uint32 GetFirstNode();
    NodeList GetNodeConnectionData(uint32 NodeID);

    uint32 GetNodeCount();

    void SetNodeOnline(uint32 Node);
    void SetNodeOffline(uint32 Node);
    void SetLastGUID(uint32 Node, uint32 type, uint32 GUID);
    void ResetLastGUID();

    void RefreshOnlineStat();


private:
    NodeMapList m_maplist;
    NodeListList m_nodelist;

    bool m_OnMaster;
    uint32 MasterNode;

    ACE_Thread_Mutex    i_Lock;

};

#define sRoutingHelper ACE_Singleton<RoutingHelper, ACE_Thread_Mutex>::instance()
#endif
