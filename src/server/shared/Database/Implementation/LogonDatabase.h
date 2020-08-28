/*
 * Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _LOGONDATABASE_H
#define _LOGONDATABASE_H

#include "DatabaseWorkerPool.h"
#include "MySQLConnection.h"

class LogonDatabaseConnection : public MySQLConnection
{
    public:
        //- Constructors for sync and async connections
        LogonDatabaseConnection(MySQLConnectionInfo& connInfo) : MySQLConnection(connInfo) {}
        LogonDatabaseConnection(ACE_Activation_Queue* q, MySQLConnectionInfo& connInfo) : MySQLConnection(q, connInfo) {}

        //- Loads database type specific prepared statements
        void DoPrepareStatements();
};

typedef DatabaseWorkerPool<LogonDatabaseConnection> LogonDatabaseWorkerPool;

enum LogonDatabaseStatements
{
    /*  Naming standard for defines:
        {DB}_{SET/DEL/ADD/REP}_{Summary of data changed}
        When updating more than one field, consider looking at the calling function
        name for a suiting suffix.
    */

    LOGON_DEL_REALM_CHARACTERS,
    LOGON_INS_REALM_CHARACTERS,
    LOGON_UPD_LOGON_STATE,
    LOGON_INS_LOGON_STATE,
    LOGON_SEL_COMMANDS,
    LOGON_SEL_NODE_SECURITY_LEVEL,
    MAX_LOGONDATABASE_STATEMENTS,
};

#endif
