#ifndef TRINITYCORE_CHAT_H
#define TRINITYCORE_CHAT_H

#include "SharedDefines.h"
#include "Player.h"

#include <vector>

class ChatHandler;
class ClientSession;
class Player;

class ChatCommand
{
    public:
        const char *       Name;
        uint32             SecurityLevel;                   // function pointer required correct align (use uint32)
        bool               AllowConsole;
        bool (*Handler)(ChatHandler*, const char* args);
        std::string        Help;
        ChatCommand*       ChildCommands;
};

class ChatHandler
{
    public:
        static ChatCommand* getCommandTable();
        bool ParseCommands(char const* text);

        ClientSession * GetSession() { return m_session; }
        explicit ChatHandler(ClientSession* session) : m_session(session) {}
        explicit ChatHandler(Player* player) : m_session(player->GetSession()) {}
        virtual ~ChatHandler() {}
        
        // Builds chat packet and returns receiver guid position in the packet to substitute in whisper builders
        static size_t BuildChatPacket(WorldPacket& data, ChatMsg chatType, Language language, uint64 senderGUID, uint64 receiverGUID, std::string const& message, uint8 chatTag,
                                    std::string const& senderName = "", std::string const& receiverName = "",
                                    uint32 achievementId = 0, bool gmMessage = false, std::string const& channelName = "");

        // Builds chat packet and returns receiver guid position in the packet to substitute in whisper builders
        static size_t BuildChatPacket(WorldPacket& data, ChatMsg chatType, Language language, ClientSession const* sender, ClientSession const* receiver, std::string const& message, uint32 achievementId = 0, std::string const& channelName = "", LocaleConstant locale = DEFAULT_LOCALE);


        static void FillMessageData(WorldPacket *data, ClientSession* session, uint8 type, uint32 language, const char *channelName, uint64 target_guid, const char *message, Player *speaker);
        void FillMessageData(WorldPacket *data, uint8 type, uint32 language, uint64 target_guid, const char* message)
        {
            FillMessageData(data, m_session, type, language, NULL, target_guid, message, NULL);
        }

        void FillSystemMessageData(WorldPacket *data, const char* message)
        {
            FillMessageData(data, CHAT_MSG_SYSTEM, LANG_UNIVERSAL, 0, message);
        }

        char*       extractKeyFromLink(char* text, char const* linkType, char** something1 = NULL);
        char*       extractKeyFromLink(char* text, char const* const* linkTypes, int* found_idx, char** something1 = NULL);
        // if args have single value then it return in arg2 and arg1 == NULL
        void        extractOptFirstArg(char* args, char** arg1, char** arg2);
        char*       extractQuotedArg(char* args);
        std::string extractPlayerNameFromLink(char* text);
        // select by arg (name/link) or in-game selection online/offline player
        bool extractPlayerTarget(char* args, Player** player, uint64* player_guid = NULL, std::string* player_name = NULL);
        std::string playerLink(std::string const& name) const { return m_session ? "|cffffffff|Hplayer:"+name+"|h["+name+"]|h|r" : name; }
        std::string GetNameLink(Player* chr) const { return playerLink(chr->GetName()); }

        // function with different implementation for chat/console
        virtual bool isAvailable(ChatCommand const& cmd) const;
        virtual const char *GetTrinityString(int32 entry) const;

        static char* LineFromMessage(char*& pos) { char* start = strtok(pos, "\n"); pos = NULL; return start; }
        virtual void SendSysMessage(const char *str);

        void SendSysMessage(int32     entry);
        void PSendSysMessage(const char *format, ...) ATTR_PRINTF(2, 3);
        void PSendSysMessage(int32     entry, ...);
        std::string PGetParseString(int32 entry, ...) const;

        void SetSentErrorMessage(bool val){ sentErrorMessage = val;};
        bool HasSentErrorMessage() const { return sentErrorMessage;}
        static bool LoadCommandTable() { return load_command_table;}
        static void SetLoadCommandTable(bool val){ load_command_table = val;};

        bool HasLowerSecurity(Player* target, uint64 guid, bool strong = false);
        bool HasLowerSecurityAccount(ClientSession* target, uint32 account, bool strong = false);

        //Helper
        bool hasStringAbbr(const char* name, const char* part);

    protected:
        explicit ChatHandler() : m_session(NULL) {}      // for CLI subclass
        bool ExecuteCommandInTable(ChatCommand* table, const char* text, std::string& fullcmd);
        bool ShowHelpForCommand(ChatCommand* table, const char* cmd);
        bool ShowHelpForSubCommands(ChatCommand* table, char const* cmd, char const* subcmd);
        static bool SetDataForCommandInTable(ChatCommand* table, const char* text, uint32 security, std::string const& help, std::string const& fullcommand);

        //Account
        bool HandleAccountCommand(char const* /*args*/);
        bool HandleAccountAddonCommand(const char *args);
        bool HandleAccountCreateCommand(const char* args);
        bool HandleAccountDeleteCommand(char const* args);
        bool HandleAccountLockCommand(char const* args);
        bool HandleAccountOnlineListCommand(char const* /*args*/);
        bool HandleAccountPasswordCommand(char const* args);

        //Bans
        bool HandleBanAccountCommand(const char* args);
        bool HandleBanAccountByCharCommand(const char* args);
        bool HandleBanCharacterCommand(const char* args);
        bool HandleBanIPCommand(const char* args);
        bool HandleBanInfoAccountCommand(const char* args);
        bool HandleBanInfoCharacterCommand(const char* args);
        bool HandleBanInfoIPCommand(const char* args);
        bool HandleBanListAccountCommand(const char* args);
        bool HandleBanListCharacterCommand(const char* args);
        bool HandleBanListIPCommand(const char* args);

        bool HandleUnBanAccountCommand(const char* args);
        bool HandleUnBanAccountByCharCommand(const char* args);
        bool HandleUnBanCharacterCommand(const char* args);
        bool HandleUnBanIPCommand(const char* args);

        bool HandleBanListHelper(PreparedQueryResult result);
        bool HandleBanHelper(BanMode mode, char const* args);
        bool HandleBanInfoHelper(uint32 accountid, char const* accountname);
        bool HandleUnBanHelper(BanMode mode, char const* args);

        //ChannelCommandz
        bool HandleLoginCommand(const char *args);
        bool HandleChannelSetOwnership(const char *args);
        bool HandleMuteCommand(const char* args);
        bool HandleUnmuteCommand(const char* args);

        //MiscCommandz
        bool HandleAntiSpamInsertCommand(const char *args);
        bool HandleAntiSpamReload(const char *args);
        bool HandleSetFakeMSGCommand(const char *args);

        //GM-Commands
        bool HandleGMCommand(const char *args);
        bool HandleGMChatCommand(const char *args);
        bool HandleToggleGmVisible(const char *args);
        bool HandleWhispersCommand(const char *args);
        bool HandleKickCommand(const char* args);

        //Ticket-Commands
        bool HandleGMTicketResponseAppendCommand(const char* args);
        bool HandleGMTicketResponseAppendLnCommand(const char* args);
        bool HandleGMTicketAssignToCommand(const char* args);
        bool HandleGMTicketCloseByIdCommand(const char* args);
        bool HandleGMTicketListClosedCommand(const char* args);
        bool HandleGMTicketCommentCommand(const char* args);
        bool HandleGMTicketCompleteCommand(const char* args);
        bool HandleGMTicketDeleteByIdCommand(const char* args);
        bool HandleGMTicketEscalateCommand(const char* args);
        bool HandleGMTicketListEscalatedCommand(const char* args);
        bool HandleGMTicketListCommand(const char* args);
        bool HandleGMTicketListOnlineCommand(const char* args);
        bool HandleGMTicketResetCommand(const char* args);
        bool HandleToggleGMTicketSystem(const char* args);
        bool HandleGMTicketUnAssignCommand(const char* args);
        bool HandleGMTicketGetByIdCommand(const char* args);
        bool HandleGMTicketGetByNameCommand(const char* args);

        //Server Commands
        bool HandleHaltServerCommand(const char* args);

        bool HandleAnnounceCommand(const char* args);
        bool HandleServerInfoCommand(const char *args);
        bool HandleServerShutDownCommand(const char *args);
        bool HandleServerShutDownCancelCommand(const char *args);
    private:
        ClientSession * m_session;                           // != NULL for chat command call and NULL for CLI command
        // common global flag
        static bool load_command_table;
        bool sentErrorMessage;
};

class CliHandler : public ChatHandler
{
    public:
        typedef void Print(void*, char const*);
        explicit CliHandler(void* callbackArg, Print* zprint) : m_callbackArg(callbackArg), m_print(zprint) {}

        // overwrite functions
        const char *GetTrinityString(int32 entry) const;
        bool isAvailable(ChatCommand const& cmd) const;
        void SendSysMessage(const char *str);
        std::string GetNameLink() const;
        bool needReportToTarget(Player* chr) const;
        LocaleConstant GetSessionDbcLocale() const;
        int GetSessionDbLocaleIndex() const;

    private:
        void* m_callbackArg;
        Print* m_print;
};

#endif
