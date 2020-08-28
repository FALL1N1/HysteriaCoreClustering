#include "Chat.h"
#include "Proxy.h"
#include "ClientSession.h"
#include "ClientSessionMgr.h"
#include "SystemConfig.h"
#include "revision.h"
#include "Language.h"
#include "RoutingHelper.h"
#include "AccountMgr.h"


bool ChatHandler::HandleServerInfoCommand(const char *args)
{
    uint32 playersNum = sClientSessionMgr->GetPlayerCount();
    uint32 maxPlayersNum = sClientSessionMgr->GetMaxPlayerCount();
    uint32 activeClientsNum = sClientSessionMgr->GetActiveSessionCount();
    uint32 queuedClientsNum = sClientSessionMgr->GetQueuedSessionCount();
    std::string uptime = secsToTimeString(sLogon->GetUptime());
    uint32 updateTime = sLogon->GetUpdateTime();
    uint32 nodecount = sRoutingHelper->GetNodeCount();

    PSendSysMessage("____________________________________");
    PSendSysMessage("|        Available Nodes: %u        |", nodecount);
    PSendSysMessage("|      -= Project BalnaZZar =-     |");
    PSendSysMessage("| diff: %u, Online Players: %u      |", updateTime, playersNum);
    PSendSysMessage("|__________________________________|");
    PSendSysMessage("|                                  |");
    for (uint32 i = 1; i != nodecount + 1; i++)
    {
        std::stringstream ss;
        bool online = sRoutingHelper->CheckNodeID(i);
        ss << "| NODE: " << i;
        if (i == 1) ss << " (Master)    "; else ss << " (Slave Node)";
        if (online)
            ss << " STATE: LIVE |";
        else
            ss << " STATE: DEAD |";
        PSendSysMessage("%s", ss.str().c_str());
    }
    PSendSysMessage("|__________________________________|");

    return true;
}

bool ChatHandler::HandleHaltServerCommand(const char *args)
{
    PSendSysMessage("Command");

    return true;
}

bool ChatHandler::HandleAnnounceCommand(const char *args)
{
    if (!*args)
        return false;

    std::stringstream ss;
    ss << args;
    sClientSessionMgr->SendServerMessage(SERVER_MSG_STRING, ss.str().c_str());
    return true;
}

bool ChatHandler::HandleServerShutDownCommand(const char *args)
{
    if (!*args)
        return false;

    char* time_str = strtok ((char*) args, " ");
    char* exitcode_str = strtok (NULL, "");

    int32 time = atoi (time_str);

    ///- Prevent interpret wrong arg value as 0 secs shutdown time
    if ((time == 0 && (time_str[0] != '0' || time_str[1] != '\0')) || time < 0)
        return false;

    if (exitcode_str)
    {
        int32 exitcode = atoi (exitcode_str);

        // Handle atoi() errors
        if (exitcode == 0 && (exitcode_str[0] != '0' || exitcode_str[1] != '\0'))
            return false;

        // Exit code should be in range of 0-125, 126-255 is used
        // in many shells for their own return codes and code > 255
        // is not supported in many others
        if (exitcode < 0 || exitcode > 125)
            return false;

        sLogon->ShutdownServ(time, 0, exitcode);
    }
    else
        sLogon->ShutdownServ(time, 0, SHUTDOWN_EXIT_CODE);
    return true;
}

bool ChatHandler::HandleServerShutDownCancelCommand(const char *args)
{
    sLogon->ShutdownCancel();
    return true;
}

bool ChatHandler::HandleBanAccountCommand(const char *args)
{
    return HandleBanHelper(BAN_ACCOUNT, args);
}

bool ChatHandler::HandleBanAccountByCharCommand(const char *args)
{
    return HandleBanHelper(BAN_CHARACTER, args);
}

bool ChatHandler::HandleBanCharacterCommand(const char *args)
{
    if (!*args)
        return false;

    char* cname = strtok((char*)args, " ");
    if (!cname)
        return false;

    std::string name = cname;

    char* duration = strtok(NULL, " ");
    if (!duration || !atoi(duration))
        return false;

   char* reason = strtok(NULL, "");
    if (!reason)
        return false;

    if (!normalizePlayerName(name))
    {
        SendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    switch (sLogon->BanCharacter(name, duration, reason, m_session ? m_session->GetPlayer()->GetPlayerName() : ""))
    {
        case BAN_SUCCESS:
        {
            if (atoi(duration) > 0)
                PSendSysMessage(LANG_BAN_YOUBANNED, name.c_str(), secsToTimeString(TimeStringToSecs(duration), true).c_str(), reason);
            else
                PSendSysMessage(LANG_BAN_YOUPERMBANNED, name.c_str(), reason);
            break;
        }
        case BAN_NOTFOUND:
        {
            PSendSysMessage(LANG_BAN_NOTFOUND, "character", name.c_str());
            SetSentErrorMessage(true);
            return false;
        }
        default:
            break;
    }

    return true;
}

bool ChatHandler::HandleBanIPCommand(const char *args)
{
    return HandleBanHelper(BAN_IP, args);
}

bool ChatHandler::HandleBanHelper(BanMode mode, const char *args)
{
    if (!*args)
        return false;

    char* cnameOrIP = strtok ((char*)args, " ");
    if (!cnameOrIP)
        return false;

    std::string nameOrIP = cnameOrIP;

    char* duration = strtok (NULL, " ");
    if (!duration || !atoi(duration))
        return false;

    char* reason = strtok (NULL, "");
    if (!reason)
        return false;

    switch (mode)
    {
        case BAN_ACCOUNT:
            if (!AccountMgr::normalizeString(nameOrIP))
            {
                PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, nameOrIP.c_str());
                SetSentErrorMessage(true);
                return false;
            }
            break;
        case BAN_CHARACTER:
            if (!normalizePlayerName(nameOrIP))
            {
                SendSysMessage(LANG_PLAYER_NOT_FOUND);
                SetSentErrorMessage(true);
                return false;
            }
            break;
        case BAN_IP:
            if (!IsIPAddress(nameOrIP.c_str()))
                return false;
            break;
    }

    switch (sLogon->BanAccount(mode, nameOrIP, duration, reason, m_session ? m_session->GetPlayer()->GetPlayerName() : ""))
    {
        case BAN_SUCCESS:
            if (atoi(duration)>0)
                PSendSysMessage(LANG_BAN_YOUBANNED, nameOrIP.c_str(), secsToTimeString(TimeStringToSecs(duration), true).c_str(), reason);
            else
                PSendSysMessage(LANG_BAN_YOUPERMBANNED, nameOrIP.c_str(), reason);
            break;
        case BAN_SYNTAX_ERROR:
            return false;
        case BAN_NOTFOUND:
            switch (mode)
            {
                default:
                    PSendSysMessage(LANG_BAN_NOTFOUND, "account", nameOrIP.c_str());
                    break;
                case BAN_CHARACTER:
                    PSendSysMessage(LANG_BAN_NOTFOUND, "character", nameOrIP.c_str());
                    break;
                case BAN_IP:
                    PSendSysMessage(LANG_BAN_NOTFOUND, "ip", nameOrIP.c_str());
                    break;
            }
            SetSentErrorMessage(true);
            return false;
    }

    return true;
}

bool ChatHandler::HandleUnBanAccountCommand(const char *args)
{
    return HandleUnBanHelper(BAN_ACCOUNT, args);
}

bool ChatHandler::HandleUnBanAccountByCharCommand(const char *args)
{
    return HandleUnBanHelper(BAN_CHARACTER, args);
}

bool ChatHandler::HandleUnBanCharacterCommand(const char *args)
{
    if (!*args)
        return false;

    char* cname = strtok((char*)args, " ");
    if (!cname)
        return false;

    std::string name = cname;

    if (!normalizePlayerName(name))
    {
        SendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    if (!sLogon->RemoveBanCharacter(name))
    {
        SendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    return true;
}

bool ChatHandler::HandleUnBanIPCommand(const char *args)
{
    return HandleUnBanHelper(BAN_IP, args);
}

bool ChatHandler::HandleUnBanHelper(BanMode mode, const char *args)
{
    if (!*args)
        return false;

    char* cnameOrIP = strtok ((char*)args, " ");
    if (!cnameOrIP)
        return false;

    std::string nameOrIP = cnameOrIP;

    switch (mode)
    {
        case BAN_ACCOUNT:
            if (!AccountMgr::normalizeString(nameOrIP))
            {
                PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, nameOrIP.c_str());
                SetSentErrorMessage(true);
                return false;
            }
            break;
        case BAN_CHARACTER:
            if (!normalizePlayerName(nameOrIP))
            {
                SendSysMessage(LANG_PLAYER_NOT_FOUND);
                SetSentErrorMessage(true);
                return false;
            }
            break;
        case BAN_IP:
            if (!IsIPAddress(nameOrIP.c_str()))
                return false;
            break;
    }

    if (sLogon->RemoveBanAccount(mode, nameOrIP))
        PSendSysMessage(LANG_UNBAN_UNBANNED, nameOrIP.c_str());
    else
        PSendSysMessage(LANG_UNBAN_ERROR, nameOrIP.c_str());

    return true;
}

bool ChatHandler::HandleBanInfoAccountCommand(const char *args)
{
    if (!*args)
        return false;

    char* cname = strtok((char*)args, "");
    if (!cname)
        return false;

    std::string account_name = cname;
    if (!AccountMgr::normalizeString(account_name))
    {
        PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, account_name.c_str());
        SetSentErrorMessage(true);
        return false;
    }

    uint32 accountid = AccountMgr::GetId(account_name);
    if (!accountid)
    {
        PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, account_name.c_str());
        return true;
    }

    return HandleBanInfoHelper(accountid, account_name.c_str());
}

bool ChatHandler::HandleBanInfoCharacterCommand(const char *args)
{
    if (!*args)
        return false;

    Player* target = sObjectMgr->FindPlayerByName(args);
    uint32 target_guid = 0;
    std::string name(args);

    if (!target)
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GUID_BY_NAME);
        stmt->setString(0, name);
        PreparedQueryResult resultCharacter = CharacterDatabase.Query(stmt);

        if (!resultCharacter)
        {
            PSendSysMessage(LANG_BANINFO_NOCHARACTER);
            return false;
        }

        target_guid = (*resultCharacter)[0].GetUInt32();
    }
    else
        target_guid = target->GetGUIDLow();

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_BANINFO);
    stmt->setUInt32(0, target_guid);
    PreparedQueryResult result = CharacterDatabase.Query(stmt);
    if (!result)
    {
        PSendSysMessage(LANG_CHAR_NOT_BANNED, name.c_str());
        return true;
    }

    PSendSysMessage(LANG_BANINFO_BANHISTORY, name.c_str());
    do
    {
        Field* fields = result->Fetch();
        time_t unbandate = time_t(fields[3].GetUInt32());
        bool active = false;
        if (fields[2].GetUInt8() && (!fields[1].GetUInt32() || unbandate >= time(NULL)))
            active = true;
        bool permanent = (fields[1].GetUInt32() == uint32(0));
        std::string bantime = permanent ? GetTrinityString(LANG_BANINFO_INFINITE) : secsToTimeString(fields[1].GetUInt32(), true);
        PSendSysMessage(LANG_BANINFO_HISTORYENTRY,
            fields[0].GetCString(), bantime.c_str(), active ? GetTrinityString(LANG_BANINFO_YES) : GetTrinityString(LANG_BANINFO_NO), fields[4].GetCString(), fields[5].GetCString());
    }
    while (result->NextRow());

    return true;
}

bool ChatHandler::HandleBanInfoHelper(uint32 accountid, char const* accountname)
{
    QueryResult result = LoginDatabase.PQuery("SELECT FROM_UNIXTIME(bandate), unbandate-bandate, active, unbandate, banreason, bannedby FROM account_banned WHERE id = '%u' ORDER BY bandate ASC", accountid);
    if (!result)
    {
        PSendSysMessage(LANG_BANINFO_NOACCOUNTBAN, accountname);
        return true;
    }

    PSendSysMessage(LANG_BANINFO_BANHISTORY, accountname);
    do
    {
        Field* fields = result->Fetch();

        time_t unbandate = time_t(fields[3].GetUInt64());
        bool active = false;
        if (fields[2].GetBool() && (fields[1].GetUInt64() == (uint64)0 ||unbandate >= time(NULL)))
            active = true;
        bool permanent = (fields[1].GetUInt64() == (uint64)0);
        std::string bantime = permanent ? GetTrinityString(LANG_BANINFO_INFINITE) : secsToTimeString(fields[1].GetUInt64(), true);
        PSendSysMessage(LANG_BANINFO_HISTORYENTRY,
            fields[0].GetCString(), bantime.c_str(), active ? GetTrinityString(LANG_BANINFO_YES) : GetTrinityString(LANG_BANINFO_NO), fields[4].GetCString(), fields[5].GetCString());
    } while (result->NextRow());

    return true;
}

bool ChatHandler::HandleBanInfoIPCommand(const char *args)
{
    if (!*args)
        return false;

    char* cIP = strtok ((char*)args, "");
    if (!cIP)
        return false;

    if (!IsIPAddress(cIP))
        return false;

    std::string IP = cIP;

    LoginDatabase.EscapeString(IP);
    QueryResult result = LoginDatabase.PQuery("SELECT ip, FROM_UNIXTIME(bandate), FROM_UNIXTIME(unbandate), unbandate-UNIX_TIMESTAMP(), banreason, bannedby, unbandate-bandate FROM ip_banned WHERE ip = '%s'", IP.c_str());
    if (!result)
    {
        PSendSysMessage(LANG_BANINFO_NOIP);
        return true;
    }

    Field* fields = result->Fetch();
    bool permanent = !fields[6].GetUInt64();
    PSendSysMessage(LANG_BANINFO_IPENTRY,
        fields[0].GetCString(), fields[1].GetCString(), permanent ? GetTrinityString(LANG_BANINFO_NEVER) : fields[2].GetCString(),
        permanent ? GetTrinityString(LANG_BANINFO_INFINITE) : secsToTimeString(fields[3].GetUInt64(), true).c_str(), fields[4].GetCString(), fields[5].GetCString());

    return true;
}

bool ChatHandler::HandleBanListCharacterCommand(const char *args)
{
    if (!*args)
        return false;

    char* cFilter = strtok((char*)args, " ");
    if (!cFilter)
        return false;

    std::string filter(cFilter);
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GUID_BY_NAME_FILTER);
    stmt->setString(0, filter);
    PreparedQueryResult result = CharacterDatabase.Query(stmt);
    if (!result)
    {
        PSendSysMessage(LANG_BANLIST_NOCHARACTER);
        return true;
    }

    PSendSysMessage(LANG_BANLIST_MATCHINGCHARACTER);

    // Chat short output
    if (m_session)
    {
        do
        {
            Field* fields = result->Fetch();
            PreparedStatement* stmt2 = CharacterDatabase.GetPreparedStatement(CHAR_SEL_BANNED_NAME);
            stmt2->setUInt32(0, fields[0].GetUInt32());
            PreparedQueryResult banresult = CharacterDatabase.Query(stmt2);
            if (banresult)
                PSendSysMessage("%s", (*banresult)[0].GetCString());
        }
        while (result->NextRow());
    }
    // Console wide output
    else
    {
        SendSysMessage(LANG_BANLIST_CHARACTERS);
        SendSysMessage(" =============================================================================== ");
        SendSysMessage(LANG_BANLIST_CHARACTERS_HEADER);
        do
        {
            SendSysMessage("-------------------------------------------------------------------------------");

            Field* fields = result->Fetch();

            std::string char_name = fields[1].GetString();

            PreparedStatement* stmt2 = CharacterDatabase.GetPreparedStatement(CHAR_SEL_BANINFO_LIST);
            stmt2->setUInt32(0, fields[0].GetUInt32());
            PreparedQueryResult banInfo = CharacterDatabase.Query(stmt2);
            if (banInfo)
            {
                Field* banFields = banInfo->Fetch();
                do
                {
                    time_t t_ban = time_t(banFields[0].GetUInt32());
                    tm* aTm_ban = localtime(&t_ban);

                    if (banFields[0].GetUInt32() == banFields[1].GetUInt32())
                    {
                        PSendSysMessage("|%-15.15s|%02d-%02d-%02d %02d:%02d|   permanent  |%-15.15s|%-15.15s|",
                            char_name.c_str(), aTm_ban->tm_year%100, aTm_ban->tm_mon+1, aTm_ban->tm_mday, aTm_ban->tm_hour, aTm_ban->tm_min,
                            banFields[2].GetCString(), banFields[3].GetCString());
                    }
                    else
                    {
                        time_t t_unban = time_t(banFields[1].GetUInt32());
                        tm* aTm_unban = localtime(&t_unban);
                        PSendSysMessage("|%-15.15s|%02d-%02d-%02d %02d:%02d|%02d-%02d-%02d %02d:%02d|%-15.15s|%-15.15s|",
                            char_name.c_str(), aTm_ban->tm_year%100, aTm_ban->tm_mon+1, aTm_ban->tm_mday, aTm_ban->tm_hour, aTm_ban->tm_min,
                            aTm_unban->tm_year%100, aTm_unban->tm_mon+1, aTm_unban->tm_mday, aTm_unban->tm_hour, aTm_unban->tm_min,
                            banFields[2].GetCString(), banFields[3].GetCString());
                    }
                }
                while (banInfo->NextRow());
            }
        }
        while (result->NextRow());
        SendSysMessage(" =============================================================================== ");
    }

    return true;
}

bool ChatHandler::HandleBanListAccountCommand(const char *args)
{
    PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_EXPIRED_IP_BANS);
    LoginDatabase.Execute(stmt);

    char* cFilter = strtok((char*)args, " ");
    std::string filter = cFilter ? cFilter : "";

    PreparedQueryResult result;

    if (filter.empty())
    {
        PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_BANNED_ALL);

        result = LoginDatabase.Query(stmt);
    }
    else
    {
        PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_BANNED_BY_USERNAME);

        stmt->setString(0, filter);

        result = LoginDatabase.Query(stmt);
    }

    if (!result)
    {
        PSendSysMessage(LANG_BANLIST_NOACCOUNT);
        return true;
    }

    return HandleBanListHelper(result);
}

bool ChatHandler::HandleBanListHelper(PreparedQueryResult result)
{
    PSendSysMessage(LANG_BANLIST_MATCHINGACCOUNT);

    // Chat short output
    if (m_session)
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 accountid = fields[0].GetUInt32();

            QueryResult banresult = LoginDatabase.PQuery("SELECT account.username FROM account, account_banned WHERE account_banned.id='%u' AND account_banned.id=account.id", accountid);
            if (banresult)
            {
                Field* fields2 = banresult->Fetch();
                PSendSysMessage("%s", fields2[0].GetCString());
            }
        } while (result->NextRow());
    }
    // Console wide output
    else
    {
        SendSysMessage(LANG_BANLIST_ACCOUNTS);
        SendSysMessage(" ===============================================================================");
        SendSysMessage(LANG_BANLIST_ACCOUNTS_HEADER);
        do
        {
            SendSysMessage("-------------------------------------------------------------------------------");
            Field* fields = result->Fetch();
            uint32 account_id = fields[0].GetUInt32();

            std::string account_name;

            // "account" case, name can be get in same query
            if (result->GetFieldCount() > 1)
                account_name = fields[1].GetString();
            // "character" case, name need extract from another DB
            else
                AccountMgr::GetName (account_id, account_name);

            // No SQL injection. id is uint32.
            QueryResult banInfo = LoginDatabase.PQuery("SELECT bandate, unbandate, bannedby, banreason FROM account_banned WHERE id = %u ORDER BY unbandate", account_id);
            if (banInfo)
            {
                Field* fields2 = banInfo->Fetch();
                do
                {
                    time_t t_ban = fields2[0].GetUInt64();
                    tm* aTm_ban = localtime(&t_ban);

                    if (fields2[0].GetUInt64() == fields2[1].GetUInt64())
                    {
                        PSendSysMessage("|%-15.15s|%02d-%02d-%02d %02d:%02d|   permanent  |%-15.15s|%-15.15s|",
                            account_name.c_str(), aTm_ban->tm_year%100, aTm_ban->tm_mon+1, aTm_ban->tm_mday, aTm_ban->tm_hour, aTm_ban->tm_min,
                            fields2[2].GetCString(), fields2[3].GetCString());
                    }
                    else
                    {
                        time_t t_unban = fields2[1].GetUInt64();
                        tm* aTm_unban = localtime(&t_unban);
                        PSendSysMessage("|%-15.15s|%02d-%02d-%02d %02d:%02d|%02d-%02d-%02d %02d:%02d|%-15.15s|%-15.15s|",
                            account_name.c_str(), aTm_ban->tm_year%100, aTm_ban->tm_mon+1, aTm_ban->tm_mday, aTm_ban->tm_hour, aTm_ban->tm_min,
                            aTm_unban->tm_year%100, aTm_unban->tm_mon+1, aTm_unban->tm_mday, aTm_unban->tm_hour, aTm_unban->tm_min,
                            fields2[2].GetCString(), fields2[3].GetCString());
                    }
                } while (banInfo->NextRow());
            }
        }while (result->NextRow());
        SendSysMessage(" ===============================================================================");
    }
    return true;
}

bool ChatHandler::HandleBanListIPCommand(const char *args)
{
    PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_EXPIRED_IP_BANS);
    LoginDatabase.Execute(stmt);

    char* cFilter = strtok((char*)args, " ");
    std::string filter = cFilter ? cFilter : "";
    LoginDatabase.EscapeString(filter);

    PreparedQueryResult result;

    if (filter.empty())
    {
        PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_IP_BANNED_ALL);

        result = LoginDatabase.Query(stmt);
    }
    else
    {
        PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_IP_BANNED_BY_IP);

        stmt->setString(0, filter);

        result = LoginDatabase.Query(stmt);
    }

    if (!result)
    {
        PSendSysMessage(LANG_BANLIST_NOIP);
        return true;
    }

    PSendSysMessage(LANG_BANLIST_MATCHINGIP);
    // Chat short output
    if (m_session)
    {
        do
        {
            Field* fields = result->Fetch();
            PSendSysMessage("%s", fields[0].GetCString());
        } while (result->NextRow());
    }
    // Console wide output
    else
    {
        SendSysMessage(LANG_BANLIST_IPS);
        SendSysMessage(" ===============================================================================");
        SendSysMessage(LANG_BANLIST_IPS_HEADER);
        do
        {
            SendSysMessage("-------------------------------------------------------------------------------");
            Field* fields = result->Fetch();
            time_t t_ban = fields[1].GetUInt64();
            tm* aTm_ban = localtime(&t_ban);
            if (fields[1].GetUInt64() == fields[2].GetUInt64())
            {
                PSendSysMessage("|%-15.15s|%02d-%02d-%02d %02d:%02d|   permanent  |%-15.15s|%-15.15s|",
                    fields[0].GetCString(), aTm_ban->tm_year%100, aTm_ban->tm_mon+1, aTm_ban->tm_mday, aTm_ban->tm_hour, aTm_ban->tm_min,
                    fields[3].GetCString(), fields[4].GetCString());
            }
            else
            {
                time_t t_unban = fields[2].GetUInt64();
                tm* aTm_unban = localtime(&t_unban);
                PSendSysMessage("|%-15.15s|%02d-%02d-%02d %02d:%02d|%02d-%02d-%02d %02d:%02d|%-15.15s|%-15.15s|",
                    fields[0].GetCString(), aTm_ban->tm_year%100, aTm_ban->tm_mon+1, aTm_ban->tm_mday, aTm_ban->tm_hour, aTm_ban->tm_min,
                    aTm_unban->tm_year%100, aTm_unban->tm_mon+1, aTm_unban->tm_mday, aTm_unban->tm_hour, aTm_unban->tm_min,
                    fields[3].GetCString(), fields[4].GetCString());
            }
        }while (result->NextRow());
        SendSysMessage(" ===============================================================================");
    }

    return true;
}