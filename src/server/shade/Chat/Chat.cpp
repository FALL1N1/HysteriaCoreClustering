#include "Common.h"
#include "ObjectMgr.h"
#include "Proxy.h"
#include "WorldPacket.h"
#include "ClientSession.h"
#include "DatabaseEnv.h"

#include "AccountMgr.h"
//#include "CellImpl.h"
#include "Chat.h"
//#include "GridNotifiersImpl.h"
#include "Language.h"
#include "Log.h"
#include "Opcodes.h"
#include "Player.h"
#include "ClientSessionMgr.h"

bool ChatHandler::load_command_table = true;

// wrapper for old-style handlers
template<bool (ChatHandler::*F)(const char*)>
bool OldHandler(ChatHandler* chatHandler, const char* args)
{
    return (chatHandler->*F)(args);
}

// get number of commands in table
static size_t getCommandTableSize(const ChatCommand* commands)
{
    if (!commands)
        return 0;
    size_t count = 0;
    while (commands[count].Name != NULL)
        count++;
    return count;
}

// append source command table to target, return number of appended commands
static size_t appendCommandTable(ChatCommand* target, const ChatCommand* source)
{
    const size_t count = getCommandTableSize(source);
    if (count)
        memcpy(target, source, count * sizeof(ChatCommand));
    return count;
}


ChatCommand* ChatHandler::getCommandTable()
{
    static ChatCommand accountCommandTable[] =
    {
        { "addon",         SEC_CONSOLE,        true,  OldHandler<&ChatHandler::HandleAccountAddonCommand>,         "", NULL },
        { "create",        SEC_CONSOLE,        true,  OldHandler<&ChatHandler::HandleAccountCreateCommand>,        "", NULL },
        { "delete",        SEC_CONSOLE,        true,  OldHandler<&ChatHandler::HandleAccountDeleteCommand>,        "", NULL },
        { "lock",          SEC_CONSOLE,        true,  OldHandler<&ChatHandler::HandleAccountLockCommand>,          "", NULL },
        { "onlinelist",    SEC_CONSOLE,        true,  OldHandler<&ChatHandler::HandleAccountOnlineListCommand>,    "", NULL },
        { "password",      SEC_CONSOLE,        true,  OldHandler<&ChatHandler::HandleAccountPasswordCommand>,      "", NULL },
        { "",              SEC_PLAYER,         false, OldHandler<&ChatHandler::HandleAccountCommand>,              "", NULL },
        { NULL,            0,                  false, NULL,                                                        "", NULL }
    };

    static ChatCommand antispamCommandTable[] =
    {
        { "insert",        SEC_ADMINISTRATOR,  true,  OldHandler<&ChatHandler::HandleAntiSpamInsertCommand>,       "", NULL },
        { "reload",        SEC_ADMINISTRATOR,  true,  OldHandler<&ChatHandler::HandleAntiSpamReload>,              "", NULL },
        { NULL,            0,                  false, NULL,                                                        "", NULL }
    };

    static ChatCommand banCommandTable[] =
    {
        { "account",        SEC_ADMINISTRATOR,  true,  OldHandler<&ChatHandler::HandleBanAccountCommand>,          "", NULL },
        { "character",      SEC_ADMINISTRATOR,  true,  OldHandler<&ChatHandler::HandleBanCharacterCommand>,        "", NULL },
        { "playeraccount",  SEC_ADMINISTRATOR,  true,  OldHandler<&ChatHandler::HandleBanAccountByCharCommand>,    "", NULL },
        { "ip",             SEC_ADMINISTRATOR,  true,  OldHandler<&ChatHandler::HandleBanIPCommand>,               "", NULL },
        { NULL,             0,                  false, NULL,                                           "", NULL }
    };

    static ChatCommand baninfoCommandTable[] =
    {
        { "account",        SEC_ADMINISTRATOR,  true,  OldHandler<&ChatHandler::HandleBanInfoAccountCommand>,      "", NULL },
        { "character",      SEC_ADMINISTRATOR,  true,  OldHandler<&ChatHandler::HandleBanInfoCharacterCommand>,    "", NULL },
        { "ip",             SEC_ADMINISTRATOR,  true,  OldHandler<&ChatHandler::HandleBanInfoIPCommand>,           "", NULL },
        { NULL,             0,                  false, NULL,                                           "", NULL }
    };

    static ChatCommand banlistCommandTable[] =
    {
        { "account",        SEC_ADMINISTRATOR,  true,  OldHandler<&ChatHandler::HandleBanListAccountCommand>,      "", NULL },
        { "character",      SEC_ADMINISTRATOR,  true,  OldHandler<&ChatHandler::HandleBanListCharacterCommand>,    "", NULL },
        { "ip",             SEC_ADMINISTRATOR,  true,  OldHandler<&ChatHandler::HandleBanListIPCommand>,           "", NULL },
        { NULL,             0,                  false, NULL,                                           "", NULL }
    };

    static ChatCommand channelSetCommandTable[] =
    {
        { "ownership",      SEC_ADMINISTRATOR,  false,  OldHandler<&ChatHandler::HandleChannelSetOwnership>,        "", NULL },
        { NULL,             0,                  false,  NULL,                                                       "", NULL }
    };

    static ChatCommand channelCommandTable[] =
    {
        { "set",            SEC_ADMINISTRATOR,  true,      NULL,                                                    "", channelSetCommandTable             },
        { NULL,             0,                  false,     NULL,                                                    "", NULL                               }
    };

    static ChatCommand gmCommandTable[] =
    {
        { "chat",           SEC_MODERATOR,      true,      OldHandler<&ChatHandler::HandleGMChatCommand>,           "", NULL                               },
        { "visible",        SEC_MODERATOR,      true,      OldHandler<&ChatHandler::HandleToggleGmVisible>,         "", NULL                               },
        { "",               SEC_MODERATOR,      false,     OldHandler<&ChatHandler::HandleGMCommand>,               "", NULL                               },
        { NULL,             0,                  false,     NULL,                                                    "", NULL                               }
    };

    static ChatCommand serverShutdownCommandTable[] =
    {
        { "cancel",         SEC_ADMINISTRATOR,  true,  OldHandler<&ChatHandler::HandleServerShutDownCancelCommand>, "", NULL },
        { ""   ,            SEC_ADMINISTRATOR,  true,  OldHandler<&ChatHandler::HandleServerShutDownCommand>,       "", NULL },
        { NULL,             0,                  false, NULL,                                                        "", NULL }
    };

    static ChatCommand serverCommandTable[] =
    {
        { "info",           SEC_PLAYER,         true,  OldHandler<&ChatHandler::HandleServerInfoCommand>,           "", NULL },
        { "shutdown",       SEC_ADMINISTRATOR,  true,  NULL,                                                        "", serverShutdownCommandTable  },
        { NULL,             0,                  false, NULL,                                                        "", NULL }
    };

    static ChatCommand unbanCommandTable[] =
    {
        { "account",        SEC_ADMINISTRATOR,  true,  OldHandler<&ChatHandler::HandleUnBanAccountCommand>,         "", NULL },
        { "character",      SEC_ADMINISTRATOR,  true,  OldHandler<&ChatHandler::HandleUnBanCharacterCommand>,       "", NULL },
        { "playeraccount",  SEC_ADMINISTRATOR,  true,  OldHandler<&ChatHandler::HandleUnBanAccountByCharCommand>,   "", NULL },
        { "ip",             SEC_ADMINISTRATOR,  true,  OldHandler<&ChatHandler::HandleUnBanIPCommand>,              "", NULL },
        { NULL,             0,                  false, NULL,                                                        "", NULL }
    };

    static ChatCommand ticketResponseCommandTable[] =
    {
        { "append",         SEC_MODERATOR,      true,  OldHandler<&ChatHandler::HandleGMTicketResponseAppendCommand>,   "", NULL },
        { "appendln",       SEC_MODERATOR,      true,  OldHandler<&ChatHandler::HandleGMTicketResponseAppendLnCommand>, "", NULL },
        { NULL,             0,                  false, NULL,                                                            "", NULL }
    };

    static ChatCommand ticketCommandTable[] =
    {
        { "assign",         SEC_GAMEMASTER,     true,  OldHandler<&ChatHandler::HandleGMTicketAssignToCommand>,         "", NULL },
        { "close",          SEC_MODERATOR,      true,  OldHandler<&ChatHandler::HandleGMTicketCloseByIdCommand>,        "", NULL },
        { "closedlist",     SEC_MODERATOR,      true,  OldHandler<&ChatHandler::HandleGMTicketListClosedCommand>,       "", NULL },
        { "comment",        SEC_MODERATOR,      true,  OldHandler<&ChatHandler::HandleGMTicketCommentCommand>,          "", NULL },
        { "complete",       SEC_MODERATOR,      true,  OldHandler<&ChatHandler::HandleGMTicketCompleteCommand>,         "", NULL },
        { "delete",         SEC_ADMINISTRATOR,  true,  OldHandler<&ChatHandler::HandleGMTicketDeleteByIdCommand>,       "", NULL },
        { "escalate",       SEC_MODERATOR,      true,  OldHandler<&ChatHandler::HandleGMTicketEscalateCommand>,         "", NULL },
        { "escalatedlist",  SEC_GAMEMASTER,     true,  OldHandler<&ChatHandler::HandleGMTicketListEscalatedCommand>,    "", NULL },
        { "list",           SEC_MODERATOR,      true,  OldHandler<&ChatHandler::HandleGMTicketListCommand>,             "", NULL },
        { "onlinelist",     SEC_MODERATOR,      true,  OldHandler<&ChatHandler::HandleGMTicketListOnlineCommand>,       "", NULL },
        { "reset",          SEC_ADMINISTRATOR,  true,  OldHandler<&ChatHandler::HandleGMTicketResetCommand>,            "", NULL },
        { "togglesystem",   SEC_ADMINISTRATOR,  true,  OldHandler<&ChatHandler::HandleToggleGMTicketSystem>,            "", NULL },
        { "unassign",       SEC_GAMEMASTER,     true,  OldHandler<&ChatHandler::HandleGMTicketUnAssignCommand>,         "", NULL },
        { "viewid",         SEC_MODERATOR,      true,  OldHandler<&ChatHandler::HandleGMTicketGetByIdCommand>,          "", NULL },
        { "viewname",       SEC_MODERATOR,      true,  OldHandler<&ChatHandler::HandleGMTicketGetByNameCommand>,        "", NULL },
        { "response",       SEC_MODERATOR,      true,  NULL,                                                            "", ticketResponseCommandTable },
        { NULL,             0,                  false, NULL,                                                            "", NULL }
    };

    static ChatCommand commandTable[] =
    {
        { "account",        SEC_MODERATOR,      true,  NULL,                                                        "", accountCommandTable  },
        { "antispam",       SEC_MODERATOR,      true,  NULL,                                                        "", antispamCommandTable },
        { "ban",            SEC_ADMINISTRATOR,  true,  NULL,                                                        "", banCommandTable      },
        { "unban",          SEC_ADMINISTRATOR,  true,  NULL,                                                        "", unbanCommandTable    },
        { "baninfo",        SEC_ADMINISTRATOR,  false, NULL,                                                        "", baninfoCommandTable  },
        { "banlist",        SEC_ADMINISTRATOR,  true,  NULL,                                                        "", banlistCommandTable  },
        { "channel",        SEC_ADMINISTRATOR,  true,  NULL,                                                        "", channelCommandTable  },
        { "gm",             SEC_MODERATOR,      false, NULL,                                                        "", gmCommandTable       },
        { "server",         SEC_ADMINISTRATOR,  true,  NULL,                                                        "", serverCommandTable   },
        { "ticket",         SEC_MODERATOR,      false, NULL,                                                        "", ticketCommandTable   },
        { "announce",       SEC_ADMINISTRATOR,  false, OldHandler<&ChatHandler::HandleAnnounceCommand>,             "", NULL },
        { "halt",           SEC_ADMINISTRATOR,  false, OldHandler<&ChatHandler::HandleHaltServerCommand>,           "", NULL },
        { "login",          SEC_ADMINISTRATOR,  false, OldHandler<&ChatHandler::HandleLoginCommand>,                "", NULL },
        { "kick",           SEC_MODERATOR,      true,  OldHandler<&ChatHandler::HandleKickCommand>,                 "", NULL },
        { "mute",           SEC_MODERATOR,      true,  OldHandler<&ChatHandler::HandleMuteCommand>,                 "", NULL },
        { "unmute",         SEC_MODERATOR,      true,  OldHandler<&ChatHandler::HandleUnmuteCommand>,               "", NULL },
        { "whispers",       SEC_MODERATOR,      false, OldHandler<&ChatHandler::HandleWhispersCommand>,             "", NULL },
        { NULL,             0,                  false, NULL,                                                        "", NULL }
    };

    static ChatCommand* commandTableCache = 0;

    if (LoadCommandTable())
    {
        SetLoadCommandTable(false);

        {
            // count total number of top-level commands
            size_t total = getCommandTableSize(commandTable);
            std::vector<ChatCommand*> const dynamic;
            for (std::vector<ChatCommand*>::const_iterator it = dynamic.begin(); it != dynamic.end(); ++it)
                total += getCommandTableSize(*it);
            total += 1; // ending zero

            // cache top-level commands
            commandTableCache = (ChatCommand*)malloc(sizeof(ChatCommand) * total);
            memset(commandTableCache, 0, sizeof(ChatCommand) * total);
            ACE_ASSERT(commandTableCache);
            size_t added = appendCommandTable(commandTableCache, commandTable);
            for (std::vector<ChatCommand*>::const_iterator it = dynamic.begin(); it != dynamic.end(); ++it)
                added += appendCommandTable(commandTableCache + added, *it);
        }

        PreparedStatement* stmt = LogonDatabase.GetPreparedStatement(LOGON_SEL_COMMANDS);

        PreparedQueryResult result = LogonDatabase.Query(stmt);

        if (result)
        {
            do
            {
                Field* fields = result->Fetch();
                std::string name = fields[0].GetString();

                SetDataForCommandInTable(commandTableCache, name.c_str(), fields[1].GetUInt8(), fields[2].GetString(), name);

            } while (result->NextRow());
        }
    }

    return commandTableCache;
}

bool ChatHandler::SetDataForCommandInTable(ChatCommand* table, const char* text, uint32 security, std::string const& help, std::string const& fullcommand)
{
    std::string cmd = "";

    while (*text != ' ' && *text != '\0')
    {
        cmd += *text;
        ++text;
    }

    while (*text == ' ') ++text;

    for (uint32 i = 0; table[i].Name != NULL; i++)
    {
        // for data fill use full explicit command names
        if (table[i].Name != cmd)
            continue;

        // select subcommand from child commands list (including "")
        if (table[i].ChildCommands != NULL)
        {
            if (SetDataForCommandInTable(table[i].ChildCommands, text, security, help, fullcommand))
                return true;
            else if (*text)
                return false;

            // fail with "" subcommands, then use normal level up command instead
        }
        // expected subcommand by full name DB content
        else if (*text)
        {
            sLog->outErrorDb("Table `command` have unexpected subcommand '%s' in command '%s', skip.", text, fullcommand.c_str());
            return false;
        }

        if (table[i].SecurityLevel != security)
            sLog->outDetail("Table `command` overwrite for command '%s' default security (%u) by %u", fullcommand.c_str(), table[i].SecurityLevel, security);

        table[i].SecurityLevel = security;
        table[i].Help          = help;
        return true;
    }

    // in case "" command let process by caller
    if (!cmd.empty())
    {
        if (table == getCommandTable())
            sLog->outErrorDb("Table `command` have not existed command '%s', skip.", cmd.c_str());
        else
            sLog->outErrorDb("Table `command` have not existed subcommand '%s' in command '%s', skip.", cmd.c_str(), fullcommand.c_str());
    }

    return false;
}

bool ChatHandler::ExecuteCommandInTable(ChatCommand* table, const char* text, std::string& fullcmd)
{
    char const* oldtext = text;
    std::string cmd = "";

    while (*text != ' ' && *text != '\0')
    {
        if (*text)
        {
            cmd += *text;
            ++text;
        }
    }

    while (*text == ' ')
        ++text;

    for (uint32 i = 0; table[i].Name != NULL; ++i)
    {
        if (!hasStringAbbr(table[i].Name, cmd.c_str()))
            continue;

        bool match = false;
        if (strlen(table[i].Name) > cmd.length())
        {
            for (uint32 j = 0; table[j].Name != NULL; ++j)
            {
                if (!hasStringAbbr(table[j].Name, cmd.c_str()))
                    continue;

                if (strcmp(table[j].Name, cmd.c_str()) != 0)
                    continue;
                else
                {
                    match = true;
                    break;
                }
            }
        }
        if (match)
            continue;

        // select subcommand from child commands list
        if (table[i].ChildCommands != NULL)
        {
            if (!ExecuteCommandInTable(table[i].ChildCommands, text, fullcmd))
            {
                if (text && text[0] != '\0')
                    SendSysMessage(LANG_NO_SUBCMD);
                else
                    SendSysMessage(LANG_CMD_SYNTAX);

                ShowHelpForCommand(table[i].ChildCommands, text);
            }

            return true;
        }

        // must be available and have handler
        if (!table[i].Handler || !isAvailable(table[i]))
            continue;

        SetSentErrorMessage(false);
        // table[i].Name == "" is special case: send original command to handler
        if ((table[i].Handler)(this, table[i].Name[0] != '\0' ? text : oldtext))
        {
            if (!AccountMgr::IsVIPorPlayer(table[i].SecurityLevel))
            {
                // chat case
                if (m_session)
                {
                    Player* p = m_session->GetPlayer();
                    sLog->outCommand(m_session->GetAccountId(), "Command: %s [Player: %s (Account: %u)",
                        fullcmd.c_str(), p->GetPlayerName().c_str(), m_session->GetAccountId());
                }
            }
        }
        // some commands have custom error messages. Don't send the default one in these cases.
        else if (!HasSentErrorMessage())
        {
            if (!table[i].Help.empty())
                SendSysMessage(table[i].Help.c_str());
            else
                SendSysMessage(LANG_CMD_SYNTAX);
        }

        return true;
    }

    return false;
}

bool ChatHandler::ShowHelpForCommand(ChatCommand* table, const char* cmd)
{
    if (*cmd)
    {
        for (uint32 i = 0; table[i].Name != NULL; ++i)
        {
            // must be available (ignore handler existence for show command with possible available subcommands)
            if (!isAvailable(table[i]))
                continue;

            if (!hasStringAbbr(table[i].Name, cmd))
                continue;

            // have subcommand
            char const* subcmd = (*cmd) ? strtok(NULL, " ") : "";

            if (table[i].ChildCommands && subcmd && *subcmd)
            {
                if (ShowHelpForCommand(table[i].ChildCommands, subcmd))
                    return true;
            }

            if (!table[i].Help.empty())
                SendSysMessage(table[i].Help.c_str());

            if (table[i].ChildCommands)
                if (ShowHelpForSubCommands(table[i].ChildCommands, table[i].Name, subcmd ? subcmd : ""))
                    return true;

            return !table[i].Help.empty();
        }
    }
    else
    {
        for (uint32 i = 0; table[i].Name != NULL; ++i)
        {
            // must be available (ignore handler existence for show command with possible available subcommands)
            if (!isAvailable(table[i]))
                continue;

            if (strlen(table[i].Name))
                continue;

            if (!table[i].Help.empty())
                SendSysMessage(table[i].Help.c_str());

            if (table[i].ChildCommands)
                if (ShowHelpForSubCommands(table[i].ChildCommands, "", ""))
                    return true;

            return !table[i].Help.empty();
        }
    }

    return ShowHelpForSubCommands(table, "", cmd);
}

bool ChatHandler::ShowHelpForSubCommands(ChatCommand* table, char const* cmd, char const* subcmd)
{
    std::string list;
    for (uint32 i = 0; table[i].Name != NULL; ++i)
    {
        // must be available (ignore handler existence for show command with possible available subcommands)
        if (!isAvailable(table[i]))
            continue;

        // for empty subcmd show all available
        if (*subcmd && !hasStringAbbr(table[i].Name, subcmd))
            continue;

        if (m_session)
            list += "\n    ";
        else
            list += "\n\r    ";

        list += table[i].Name;

        if (table[i].ChildCommands)
            list += " ...";
    }

    if (list.empty())
        return false;

    if (table == getCommandTable())
    {
        SendSysMessage(LANG_AVIABLE_CMD);
        PSendSysMessage("%s", list.c_str());
    }
    else
        PSendSysMessage(LANG_SUBCMDS_LIST, cmd, list.c_str());

    return true;
}

bool ChatHandler::ParseCommands(char const* text)
{
    ASSERT(text);
    //ASSERT(*text);

    std::string fullcmd = text;

    if (m_session && AccountMgr::IsPlayerAccount(m_session->GetSecurity()))
       return false;

    /// chat case (.command or !command format)
    if (m_session)
    {
        if (text[0] != '!' && text[0] != '.')
            return false;
    }

    /// ignore single . and ! in line
    if (strlen(text) < 2)
        return false;
    // original `text` can't be used. It content destroyed in command code processing.

    /// ignore messages staring from many dots.
    if ((text[0] == '.' && text[1] == '.') || (text[0] == '!' && text[1] == '!'))
        return false;

    /// skip first . or ! (in console allowed use command with . and ! and without its)
    if (text[0] == '!' || text[0] == '.')
        ++text;

    if (!ExecuteCommandInTable(getCommandTable(), text, fullcmd))
    {
        if (m_session && AccountMgr::IsPlayerAccount(m_session->GetSecurity()))
            return false;

        SendSysMessage(LANG_NO_CMD);
    }
    return true;
}

bool ChatHandler::isAvailable(ChatCommand const& cmd) const
{
    // check security level only for simple  command (without child commands)
    return m_session->GetSecurity() >= AccountTypes(cmd.SecurityLevel);
}

void ChatHandler::SendSysMessage(const char *str)
{
    WorldPacket data;

    // need copy to prevent corruption by strtok call in LineFromMessage original string
    char* buf = strdup(str);
    char* pos = buf;

    while (char* line = LineFromMessage(pos))
    {
        FillSystemMessageData(&data, line);
        m_session->SendPacket(&data);
    }

    free(buf);
}

void ChatHandler::SendSysMessage(int32 entry)
{
    SendSysMessage(GetTrinityString(entry));
}

const char *ChatHandler::GetTrinityString(int32 entry) const
{
    return m_session->GetTrinityString(entry);
}

void ChatHandler::PSendSysMessage(int32 entry, ...)
{
    const char *format = GetTrinityString(entry);
    va_list ap;
    char str [2048];
    va_start(ap, entry);
    vsnprintf(str, 2048, format, ap);
    va_end(ap);
    SendSysMessage(str);
}

void ChatHandler::PSendSysMessage(const char *format, ...)
{
    va_list ap;
    char str [2048];
    va_start(ap, format);
    vsnprintf(str, 2048, format, ap);
    va_end(ap);
    SendSysMessage(str);
}

std::string ChatHandler::PGetParseString(int32 entry, ...) const
{
    const char *format = GetTrinityString(entry);
    char str[1024];
    va_list ap;
    va_start(ap, entry);
    vsnprintf(str, 1024, format, ap);
    va_end(ap);
    return std::string(str);
}

//Note: target_guid used only in CHAT_MSG_WHISPER_INFORM mode (in this case channelName ignored)
void ChatHandler::FillMessageData(WorldPacket *data, ClientSession* session, uint8 type, uint32 language, const char *channelName, uint64 target_guid, const char *message, Player *speaker)
{
    uint32 messageLength = (message ? strlen(message) : 0) + 1;

    data->Initialize(SMSG_MESSAGECHAT, 100);                // guess size
    *data << uint8(type);
    if ((type != CHAT_MSG_CHANNEL && type != CHAT_MSG_WHISPER) || language == LANG_ADDON)
        *data << uint32(language);
    else
        *data << uint32(LANG_UNIVERSAL);

    switch(type)
    {
        case CHAT_MSG_SAY:
        case CHAT_MSG_PARTY:
        case CHAT_MSG_PARTY_LEADER:
        case CHAT_MSG_RAID:
        case CHAT_MSG_GUILD:
        case CHAT_MSG_OFFICER:
        case CHAT_MSG_YELL:
        case CHAT_MSG_WHISPER:
        case CHAT_MSG_CHANNEL:
        case CHAT_MSG_RAID_LEADER:
        case CHAT_MSG_RAID_WARNING:
        case CHAT_MSG_BG_SYSTEM_NEUTRAL:
        case CHAT_MSG_BG_SYSTEM_ALLIANCE:
        case CHAT_MSG_BG_SYSTEM_HORDE:
        case CHAT_MSG_BATTLEGROUND:
        case CHAT_MSG_BATTLEGROUND_LEADER:
            target_guid = session ? session->GetPlayer()->GetGUID() : 0;
            break;
        case CHAT_MSG_MONSTER_SAY:
        case CHAT_MSG_MONSTER_PARTY:
        case CHAT_MSG_MONSTER_YELL:
        case CHAT_MSG_MONSTER_WHISPER:
        case CHAT_MSG_MONSTER_EMOTE:
        case CHAT_MSG_RAID_BOSS_WHISPER:
        case CHAT_MSG_RAID_BOSS_EMOTE:
        case CHAT_MSG_BATTLENET:
        /*{
            *data << uint64(speaker->GetGUID());
            *data << uint32(0);                             // 2.1.0
            *data << uint32(strlen(speaker->GetPlayerName()) + 1);
            *data << speaker->GetPlayerName();
            uint64 listener_guid = 0;
            *data << uint64(listener_guid);
            if (listener_guid && !IS_PLAYER_GUID(listener_guid))
            {
                *data << uint32(1);                         // string listener_name_length
                *data << uint8(0);                          // string listener_name
            }
            *data << uint32(messageLength);
            *data << message;
            *data << uint8(0);
            return;
        }*/
        default:
            if (type != CHAT_MSG_WHISPER_INFORM && type != CHAT_MSG_IGNORED && type != CHAT_MSG_DND && type != CHAT_MSG_AFK)
                target_guid = 0;                            // only for CHAT_MSG_WHISPER_INFORM used original value target_guid
            break;
    }

    *data << uint64(target_guid);                           // there 0 for BG messages
    *data << uint32(0);                                     // can be chat msg group or something

    if (type == CHAT_MSG_CHANNEL)
    {
        ASSERT(channelName);
        *data << channelName;
    }

    *data << uint64(target_guid);
    *data << uint32(messageLength);
    *data << message;
    if (session != 0 && type != CHAT_MSG_WHISPER_INFORM && type != CHAT_MSG_DND && type != CHAT_MSG_AFK)
        *data << uint8(session->GetPlayer()->GetChatTag());
    else
        *data << uint8(0);
}


//Helpers
bool ChatHandler::hasStringAbbr(const char* name, const char* part)
{
    // non "" command
    if (*name)
    {
        // "" part from non-"" command
        if (!*part)
            return false;

        for (;;)
        {
            if (!*part)
                return true;
            else if (!*name)
                return false;
            else if (tolower(*name) != tolower(*part))
                return false;
            ++name; ++part;
        }
    }
    // allow with any for ""

    return true;
}

const char *CliHandler::GetTrinityString(int32 entry) const
{
    return sObjectMgr->GetTrinityStringForDBCLocale(entry);
}

bool CliHandler::isAvailable(ChatCommand const& cmd) const
{
    // skip non-console commands in console case
    return cmd.AllowConsole;
}

bool ChatHandler::HasLowerSecurity(Player* target, uint64 guid, bool strong)
{
    ClientSession* target_session = NULL;
    uint32 target_account = 0;

    if (target)
        target_session = target->GetSession();
    else if (guid)
        target_account = sObjectMgr->GetPlayerAccountIdByGUID(guid);

    if (!target_session && !target_account)
    {
        SendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return true;
    }

    return HasLowerSecurityAccount(target_session, target_account, strong);
}

bool ChatHandler::HasLowerSecurityAccount(ClientSession* target, uint32 target_account, bool strong)
{
    uint32 target_sec;

    // allow everything from console and RA console
    if (!m_session)
        return false;

    // ignore only for non-players for non strong checks (when allow apply command at least to same sec level)
    if (!AccountMgr::IsVIPorPlayer(m_session->GetSecurity()) && !strong && !sLogon->getBoolConfig(CONFIG_GM_LOWER_SECURITY))
        return false;

    if (target)
        target_sec = target->GetSecurity();
    else if (target_account)
        target_sec = AccountMgr::GetSecurity(target_account, realmID);
    else
        return true;                                        // caller must report error for (target == NULL && target_account == 0)

    AccountTypes target_ac_sec = AccountTypes(target_sec);
    if (m_session->GetSecurity() < target_ac_sec || (strong && m_session->GetSecurity() <= target_ac_sec))
    {
        SendSysMessage(LANG_YOURS_SECURITY_IS_LOW);
        SetSentErrorMessage(true);
        return true;
    }

    return false;
}

void CliHandler::SendSysMessage(const char *str)
{
    m_print(m_callbackArg, str);
    m_print(m_callbackArg, "\r\n");
}

std::string CliHandler::GetNameLink() const
{
    return GetTrinityString(LANG_CONSOLE_COMMAND);
}

bool CliHandler::needReportToTarget(Player* /*chr*/) const
{
    return true;
}

LocaleConstant CliHandler::GetSessionDbcLocale() const
{
    return sLogon->GetDefaultDbcLocale();
}

int CliHandler::GetSessionDbLocaleIndex() const
{
    return sObjectMgr->GetDBCLocaleIndex();
}

char* ChatHandler::extractKeyFromLink(char* text, char const* linkType, char** something1)
{
    // skip empty
    if (!text)
        return NULL;

    // skip spaces
    while (*text == ' '||*text == '\t'||*text == '\b')
        ++text;

    if (!*text)
        return NULL;

    // return non link case
    if (text[0] != '|')
        return strtok(text, " ");

    // [name] Shift-click form |color|linkType:key|h[name]|h|r
    // or
    // [name] Shift-click form |color|linkType:key:something1:...:somethingN|h[name]|h|r

    char* check = strtok(text, "|");                        // skip color
    if (!check)
        return NULL;                                        // end of data

    char* cLinkType = strtok(NULL, ":");                    // linktype
    if (!cLinkType)
        return NULL;                                        // end of data

    if (strcmp(cLinkType, linkType) != 0)
    {
        strtok(NULL, " ");                                  // skip link tail (to allow continue strtok(NULL, s) use after retturn from function
        SendSysMessage(LANG_WRONG_LINK_TYPE);
        return NULL;
    }

    char* cKeys = strtok(NULL, "|");                        // extract keys and values
    char* cKeysTail = strtok(NULL, "");

    char* cKey = strtok(cKeys, ":|");                       // extract key
    if (something1)
        *something1 = strtok(NULL, ":|");                   // extract something

    strtok(cKeysTail, "]");                                 // restart scan tail and skip name with possible spaces
    strtok(NULL, " ");                                      // skip link tail (to allow continue strtok(NULL, s) use after return from function
    return cKey;
}

char* ChatHandler::extractKeyFromLink(char* text, char const* const* linkTypes, int* found_idx, char** something1)
{
    // skip empty
    if (!text)
        return NULL;

    // skip spaces
    while (*text == ' '||*text == '\t'||*text == '\b')
        ++text;

    if (!*text)
        return NULL;

    // return non link case
    if (text[0] != '|')
        return strtok(text, " ");

    // [name] Shift-click form |color|linkType:key|h[name]|h|r
    // or
    // [name] Shift-click form |color|linkType:key:something1:...:somethingN|h[name]|h|r
    // or
    // [name] Shift-click form |linkType:key|h[name]|h|r

    char* tail;

    if (text[1] == 'c')
    {
        char* check = strtok(text, "|");                    // skip color
        if (!check)
            return NULL;                                    // end of data

        tail = strtok(NULL, "");                            // tail
    }
    else
        tail = text+1;                                      // skip first |

    char* cLinkType = strtok(tail, ":");                    // linktype
    if (!cLinkType)
        return NULL;                                        // end of data

    for (int i = 0; linkTypes[i]; ++i)
    {
        if (strcmp(cLinkType, linkTypes[i]) == 0)
        {
            char* cKeys = strtok(NULL, "|");                // extract keys and values
            char* cKeysTail = strtok(NULL, "");

            char* cKey = strtok(cKeys, ":|");               // extract key
            if (something1)
                *something1 = strtok(NULL, ":|");           // extract something

            strtok(cKeysTail, "]");                         // restart scan tail and skip name with possible spaces
            strtok(NULL, " ");                              // skip link tail (to allow continue strtok(NULL, s) use after return from function
            if (found_idx)
                *found_idx = i;
            return cKey;
        }
    }

    strtok(NULL, " ");                                      // skip link tail (to allow continue strtok(NULL, s) use after return from function
    SendSysMessage(LANG_WRONG_LINK_TYPE);
    return NULL;
}

void ChatHandler::extractOptFirstArg(char* args, char** arg1, char** arg2)
{
    char* p1 = strtok(args, " ");
    char* p2 = strtok(NULL, " ");

    if (!p2)
    {
        p2 = p1;
        p1 = NULL;
    }

    if (arg1)
        *arg1 = p1;

    if (arg2)
        *arg2 = p2;
}

char* ChatHandler::extractQuotedArg(char* args)
{
    if (!*args)
        return NULL;

    if (*args == '"')
        return strtok(args+1, "\"");
    else
    {
        char* space = strtok(args, "\"");
        if (!space)
            return NULL;
        return strtok(NULL, "\"");
    }
}

std::string ChatHandler::extractPlayerNameFromLink(char* text)
{
    // |color|Hplayer:name|h[name]|h|r
    char* name_str = extractKeyFromLink(text, "Hplayer");
    if (!name_str)
        return "";

    std::string name = name_str;
    if (!normalizePlayerName(name))
        return "";

    return name;
}

bool ChatHandler::extractPlayerTarget(char* args, Player** player, uint64* player_guid /*=NULL*/, std::string* player_name /*= NULL*/)
{
    if (args && *args)
    {
        std::string name = extractPlayerNameFromLink(args);
        if (name.empty())
        {
            SendSysMessage(LANG_PLAYER_NOT_FOUND);
            SetSentErrorMessage(true);
            return false;
        }

        Player* pl = sObjectMgr->FindPlayerByName(name.c_str());

        // if allowed player pointer
        if (player)
            *player = pl;

        // if need guid value from DB (in name case for check player existence)
        uint64 guid = !pl && (player_guid || player_name) ? sObjectMgr->GetPlayerGUIDByName(name) : 0;

        // if allowed player guid (if no then only online players allowed)
        if (player_guid)
            *player_guid = pl ? pl->GetGUID() : guid;

        if (player_name)
            *player_name = pl || guid ? name : "";
    }

    // some from req. data must be provided (note: name is empty if player not exist)
    if ((!player || !*player) && (!player_guid || !*player_guid) && (!player_name || player_name->empty()))
    {
        SendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    return true;
}


size_t ChatHandler::BuildChatPacket(WorldPacket& data, ChatMsg chatType, Language language, uint64 senderGUID, uint64 receiverGUID, std::string const& message, uint8 chatTag,
                                  std::string const& senderName /*= ""*/, std::string const& receiverName /*= ""*/,
                                  uint32 achievementId /*= 0*/, bool gmMessage /*= false*/, std::string const& channelName /*= ""*/)
{
    size_t receiverGUIDPos = 0;
    data.Initialize(!gmMessage ? SMSG_MESSAGECHAT : SMSG_GM_MESSAGECHAT);
    data << uint8(chatType);
    data << int32(language);
    data << uint64(senderGUID);
    data << uint32(0);  // some flags
    switch (chatType)
    {
        case CHAT_MSG_MONSTER_SAY:
        case CHAT_MSG_MONSTER_PARTY:
        case CHAT_MSG_MONSTER_YELL:
        case CHAT_MSG_MONSTER_WHISPER:
        case CHAT_MSG_MONSTER_EMOTE:
        case CHAT_MSG_RAID_BOSS_EMOTE:
        case CHAT_MSG_RAID_BOSS_WHISPER:
        case CHAT_MSG_BATTLENET:
            data << uint32(senderName.length() + 1);
            data << senderName;
            receiverGUIDPos = data.wpos();
            data << uint64(receiverGUID);
            if (receiverGUID && !IS_PLAYER_GUID(receiverGUID) && !IS_PET_GUID(receiverGUID))
            {
                data << uint32(receiverName.length() + 1);
                data << receiverName;
            }
            break;
        case CHAT_MSG_WHISPER_FOREIGN:
            data << uint32(senderName.length() + 1);
            data << senderName;
            receiverGUIDPos = data.wpos();
            data << uint64(receiverGUID);
            break;
        case CHAT_MSG_BG_SYSTEM_NEUTRAL:
        case CHAT_MSG_BG_SYSTEM_ALLIANCE:
        case CHAT_MSG_BG_SYSTEM_HORDE:
            receiverGUIDPos = data.wpos();
            data << uint64(receiverGUID);
            if (receiverGUID && !IS_PLAYER_GUID(receiverGUID))
            {
                data << uint32(receiverName.length() + 1);
                data << receiverName;
            }
            break;
        case CHAT_MSG_ACHIEVEMENT:
        case CHAT_MSG_GUILD_ACHIEVEMENT:
            receiverGUIDPos = data.wpos();
            data << uint64(receiverGUID);
            break;
        default:
            if (gmMessage)
            {
                data << uint32(senderName.length() + 1);
                data << senderName;
            }

            if (chatType == CHAT_MSG_CHANNEL)
            {
                ASSERT(channelName.length() > 0);
                data << channelName;
            }

            receiverGUIDPos = data.wpos();
            data << uint64(receiverGUID);
            break;
    }

    data << uint32(message.length() + 1);
    data << message;
    data << uint8(chatTag);

    if (chatType == CHAT_MSG_ACHIEVEMENT || chatType == CHAT_MSG_GUILD_ACHIEVEMENT)
        data << uint32(achievementId);

    return receiverGUIDPos;
}

size_t ChatHandler::BuildChatPacket(WorldPacket& data, ChatMsg chatType, Language language, ClientSession const* sender, ClientSession const* receiver, std::string const& message,
                                  uint32 achievementId /*= 0*/, std::string const& channelName /*= ""*/, LocaleConstant locale /*= DEFAULT_LOCALE*/)
{
    uint64 senderGUID = 0;
    std::string senderName = "";
    uint8 chatTag = 0;
    bool gmMessage = false;
    uint64 receiverGUID = 0;
    std::string receiverName = "";

    if(sender)
        if (Player* plr = sender->GetPlayer())
        {
            senderGUID = plr->GetGUID();
            senderName = plr->GetName(); // plr->GetNameForLocaleIdx(locale);
            chatTag = plr->GetChatTag();
            gmMessage = plr->isGameMaster();
        }

    if(receiver)
        if (Player* plr = receiver->GetPlayer())
        {
            receiverGUID = plr->GetGUID();
            receiverName = plr->GetName(); // ->GetNameForLocaleIdx(locale);
        }

    return BuildChatPacket(data, chatType, language, senderGUID, receiverGUID, message, chatTag, senderName, receiverName, achievementId, gmMessage, channelName);
}
