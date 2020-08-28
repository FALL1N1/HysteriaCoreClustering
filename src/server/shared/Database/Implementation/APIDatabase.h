#ifndef _APIDATABASE_H
#define _APIDATABASE_H

#include "DatabaseWorkerPool.h"
#include "MySQLConnection.h"

class APIDatabaseConnection : public MySQLConnection
{
    public: 
        APIDatabaseConnection(MySQLConnectionInfo& connInfo) : MySQLConnection(connInfo) { }
        APIDatabaseConnection(ACE_Activation_Queue* q, MySQLConnectionInfo& connInfo) : MySQLConnection(q, connInfo) { }
        void DoPrepareStatements();
};

typedef DatabaseWorkerPool<APIDatabaseConnection> APIDatabaseWorkerPool;

enum APIDatabaseStatements
{ 
    API_DEL_CHAR_STATS,
    API_INS_CHAR_STATS,
    API_INS_ARENA_GAME,
    API_INS_ARENA_MEMBERSTATS,
    API_INS_PVE_KILL,
    API_INS_PLAYER_TALENT,
    API_DEL_PLAYER_TALENT_BY_SPELL,
    API_UPD_PLAYER_ITEMS,

    // required 5
    API_UPD_PLAYER_LOOTED_ITEMS_1,
    API_UPD_PLAYER_LOOTED_ITEMS_2,
    API_UPD_PLAYER_LOOTED_ITEMS_3,
    API_UPD_PLAYER_LOOTED_ITEMS_4,
    API_UPD_PLAYER_LOOTED_ITEMS_5,
    MAX_APIDATABASE_STATEMENTS,
};

#endif
