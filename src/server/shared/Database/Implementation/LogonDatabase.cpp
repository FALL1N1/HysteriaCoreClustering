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

#include "LogonDatabase.h"

void LogonDatabaseConnection::DoPrepareStatements()
{
    if (!m_reconnecting)
        m_stmts.resize(MAX_LOGONDATABASE_STATEMENTS);

    PrepareStatement(LOGON_DEL_REALM_CHARACTERS, "DELETE FROM nodecharacters WHERE acctid = ? AND realmid = ?", CONNECTION_ASYNC);
    PrepareStatement(LOGON_INS_REALM_CHARACTERS, "INSERT INTO nodecharacters (numchars, acctid, realmid) VALUES (?, ?, ?)", CONNECTION_ASYNC);
    PrepareStatement(LOGON_UPD_LOGON_STATE, "UPDATE logonstates SET value = ? WHERE entry = ? AND realmid = ?", CONNECTION_ASYNC);
    PrepareStatement(LOGON_INS_LOGON_STATE, "INSERT INTO logonstates (realmid, entry, value) VALUES (?, ?, ?)", CONNECTION_ASYNC);
    PrepareStatement(LOGON_SEL_COMMANDS, "SELECT name, security, help FROM command", CONNECTION_SYNCH);
    PrepareStatement(LOGON_SEL_NODE_SECURITY_LEVEL, "SELECT allowedSecurityLevel from nodelist WHERE NodeID = ?", CONNECTION_SYNCH);

    /* ############## END OF LOADING PREPARED STATEMENTS ############## */
}
