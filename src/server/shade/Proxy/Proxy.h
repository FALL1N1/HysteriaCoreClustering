#ifndef __LOGON_H
#define __LOGON_H

#include "Common.h"
#include "Timer.h"
#include "SharedDefines.h"
#include "QueryResult.h"
#include "Callback.h"
#include <atomic>
#include <map>
#include <set>
#include <list>
#include <unordered_map>

typedef std::set<std::string> AntiSpam;
class Player;
#define LOGON_LOGIN_DELAY 2 * IN_MILLISECONDS * IN_MILLISECONDS
#define LOGON_DOS_STRIKE_LIMIT 5

struct GlobalPlayerData
{
    uint32 guidLow;
    uint32 accountId;
    std::string name;
    uint8 race;
    uint8 playerClass;
    uint8 gender;
    uint8 level;
    uint16 mailCount;
    uint32 guildId;
    uint32 groupId;
    uint32 arenaTeamId[3];
};

enum GlobalPlayerUpdateMask
{
    PLAYER_UPDATE_DATA_LEVEL            = 0x01,
    PLAYER_UPDATE_DATA_RACE                = 0x02,
    PLAYER_UPDATE_DATA_CLASS            = 0x04,
    PLAYER_UPDATE_DATA_GENDER            = 0x08,
    PLAYER_UPDATE_DATA_NAME                = 0x10,
};

// ServerMessages.dbc
enum ServerMessageType
{
    SERVER_MSG_SHUTDOWN_TIME      = 1,
    SERVER_MSG_RESTART_TIME       = 2,
    SERVER_MSG_STRING             = 3,
    SERVER_MSG_SHUTDOWN_CANCELLED = 4,
    SERVER_MSG_RESTART_CANCELLED  = 5
};

typedef std::unordered_map<uint32, GlobalPlayerData> GlobalPlayerDataMap;
typedef std::map<std::string, uint32> GlobalPlayerNameMap;
typedef std::unordered_map<uint32, ClientSession*> SessionMap;


enum ShutdownMask
{
    SHUTDOWN_MASK_RESTART = 1,
    SHUTDOWN_MASK_IDLE    = 2,
    SHUTDOWN_MASK_HALT    = 3,
};

enum ShutdownExitCode
{
    SHUTDOWN_EXIT_CODE = 0,
    ERROR_EXIT_CODE    = 1,
    RESTART_EXIT_CODE  = 2,
    SERVICE_EXIT_CODE  = 3,
};

/// Type of server, this is values from second column of Cfg_Configs.dbc
enum RealmType
{
    REALM_TYPE_NORMAL = 0,
    REALM_TYPE_PVP = 1,
    REALM_TYPE_NORMAL2 = 4,
    REALM_TYPE_RP = 6,
    REALM_TYPE_RPPVP = 8,
    REALM_TYPE_FFA_PVP = 16                                 // custom, free for all pvp mode like arena PvP in all zones except rest activated places and sanctuaries
                                                            // replaced by REALM_PVP in realm list
};

enum RealmZone
{
    REALM_ZONE_UNKNOWN       = 0,                           // any language
    REALM_ZONE_DEVELOPMENT   = 1,                           // any language
    REALM_ZONE_UNITED_STATES = 2,                           // extended-Latin
    REALM_ZONE_OCEANIC       = 3,                           // extended-Latin
    REALM_ZONE_LATIN_AMERICA = 4,                           // extended-Latin
    REALM_ZONE_TOURNAMENT_5  = 5,                           // basic-Latin at create, any at login
    REALM_ZONE_KOREA         = 6,                           // East-Asian
    REALM_ZONE_TOURNAMENT_7  = 7,                           // basic-Latin at create, any at login
    REALM_ZONE_ENGLISH       = 8,                           // extended-Latin
    REALM_ZONE_GERMAN        = 9,                           // extended-Latin
    REALM_ZONE_FRENCH        = 10,                          // extended-Latin
    REALM_ZONE_SPANISH       = 11,                          // extended-Latin
    REALM_ZONE_RUSSIAN       = 12,                          // Cyrillic
    REALM_ZONE_TOURNAMENT_13 = 13,                          // basic-Latin at create, any at login
    REALM_ZONE_TAIWAN        = 14,                          // East-Asian
    REALM_ZONE_TOURNAMENT_15 = 15,                          // basic-Latin at create, any at login
    REALM_ZONE_CHINA         = 16,                          // East-Asian
    REALM_ZONE_CN1           = 17,                          // basic-Latin at create, any at login
    REALM_ZONE_CN2           = 18,                          // basic-Latin at create, any at login
    REALM_ZONE_CN3           = 19,                          // basic-Latin at create, any at login
    REALM_ZONE_CN4           = 20,                          // basic-Latin at create, any at login
    REALM_ZONE_CN5           = 21,                          // basic-Latin at create, any at login
    REALM_ZONE_CN6           = 22,                          // basic-Latin at create, any at login
    REALM_ZONE_CN7           = 23,                          // basic-Latin at create, any at login
    REALM_ZONE_CN8           = 24,                          // basic-Latin at create, any at login
    REALM_ZONE_TOURNAMENT_25 = 25,                          // basic-Latin at create, any at login
    REALM_ZONE_TEST_SERVER   = 26,                          // any language
    REALM_ZONE_TOURNAMENT_27 = 27,                          // basic-Latin at create, any at login
    REALM_ZONE_QA_SERVER     = 28,                          // any language
    REALM_ZONE_CN9           = 29,                          // basic-Latin at create, any at login
    REALM_ZONE_TEST_SERVER_2 = 30,                          // any language
    REALM_ZONE_CN10          = 31,                          // basic-Latin at create, any at login
    REALM_ZONE_CTC           = 32,
    REALM_ZONE_CNC           = 33,
    REALM_ZONE_CN1_4         = 34,                          // basic-Latin at create, any at login
    REALM_ZONE_CN2_6_9       = 35,                          // basic-Latin at create, any at login
    REALM_ZONE_CN3_7         = 36,                          // basic-Latin at create, any at login
    REALM_ZONE_CN5_8         = 37                           // basic-Latin at create, any at login
};

enum LogonTimer
{
    LUPDATE_UPTIME,
    LUPDATE_RECONNECT,
    LUPDATE_PINGDB,
    LUPDATE_REBASE,
    LUPDATE_BAN,
    LUPDATE_COUNT
};

/// Server rates
enum LogonRates
{
    RATE_HEALTH = 0,
    RATE_POWER_MANA,
    RATE_POWER_RAGE_INCOME,
    RATE_POWER_RAGE_LOSS,
    RATE_POWER_RUNICPOWER_INCOME,
    RATE_POWER_RUNICPOWER_LOSS,
    RATE_POWER_FOCUS,
    RATE_POWER_ENERGY,
    RATE_SKILL_DISCOVERY,
    RATE_DROP_ITEM_POOR,
    RATE_DROP_ITEM_NORMAL,
    RATE_DROP_ITEM_UNCOMMON,
    RATE_DROP_ITEM_RARE,
    RATE_DROP_ITEM_EPIC,
    RATE_DROP_ITEM_LEGENDARY,
    RATE_DROP_ITEM_ARTIFACT,
    RATE_DROP_ITEM_REFERENCED,
    RATE_DROP_ITEM_REFERENCED_AMOUNT,
    RATE_DROP_MONEY,
    RATE_XP_KILL,
    RATE_XP_QUEST,
    RATE_XP_EXPLORE,
    RATE_REPAIRCOST,
    RATE_REST_INGAME,
    RATE_REST_OFFLINE_IN_TAVERN_OR_CITY,
    RATE_REST_OFFLINE_IN_WILDERNESS,
    RATE_DAMAGE_FALL,
    RATE_AUCTION_TIME,
    RATE_AUCTION_DEPOSIT,
    RATE_AUCTION_CUT,
    RATE_HONOR,
    RATE_MINING_AMOUNT,
    RATE_MINING_NEXT,
    RATE_TALENT,
    RATE_REPUTATION_GAIN,
    RATE_REPUTATION_LOWLEVEL_KILL,
    RATE_REPUTATION_LOWLEVEL_QUEST,
    RATE_REPUTATION_RECRUIT_A_FRIEND_BONUS,

    MAX_RATES
};

enum LogonIntConfigs
{
    CONFIG_SOCKET_TIMEOUTTIME = 0,
    CONFIG_GROUP_VISIBILITY,
    CONFIG_BATTLEGROUND_INVITATION_TYPE,
    CONFIG_PORT_LOGON,
    CONFIG_MAX_OVERSPEED_PINGS,
    CONFIG_SESSION_ADD_DELAY,
    CONFIG_SESSION_MT_THREADS,
    CONFIG_SESSION_DYN_TIMER,
    CONFIG_SESSION_DYN_MAX,
    CONFIG_EXPANSION,
    CONFIG_GAME_TYPE,
    CONFIG_REALM_ZONE,
    CONFIG_CLIENTCACHE_VERSION,
    CONFIG_DB_PING_INTERVAL,
    CONFIG_UPTIME_UPDATE,
    CONFIG_REBASE_INTERVAL,

    // CHAT SETTINGS
    CONFIG_CHAT_WHISPER_LEVEL_REQ,

    // GM Settings
    CONFIG_GM_LOGIN_STATE,
    CONFIG_GM_VISIBLE_STATE,
    CONFIG_GM_CHAT,
    CONFIG_GM_WHISPERING_TO,
    CONFIG_GM_LEVEL_IN_GM_LIST,
    CONFIG_GM_LEVEL_IN_WHO_LIST,
    CONFIG_GM_LEVEL_ALLOW_ACHIEVEMENTS,

    // Warden
    CONFIG_WARDEN_NUM_MEM_CHECKS,
    CONFIG_WARDEN_NUM_OTHER_CHECKS,
    CONFIG_WARDEN_CLIENT_BAN_DURATION,
    CONFIG_WARDEN_CLIENT_CHECK_HOLDOFF,
    CONFIG_WARDEN_CLIENT_RESPONSE_DELAY,

    // Channels
    CONFIG_PRESERVE_CUSTOM_CHANNEL_DURATION,

    // Tickets
    CONFIG_GMTICKET_LEVEL_REQ,

    // Rates
    CONFIG_SKILL_GAIN_CRAFTING,
    CONFIG_SKILL_GAIN_DEFENSE,
    CONFIG_SKILL_GAIN_GATHERING,
    CONFIG_SKILL_GAIN_WEAPON,
    CONFIG_SKILL_CHANCE_ORANGE,
    CONFIG_SKILL_CHANCE_YELLOW,
    CONFIG_SKILL_CHANCE_GREEN,
    CONFIG_SKILL_CHANCE_GREY,
    CONFIG_SKILL_CHANCE_MINING_STEPS,
    CONFIG_SKILL_CHANCE_SKINNING_STEPS,

    INT_CONFIG_VALUE_COUNT
};

enum LogonBoolConfigs
{
    CONFIG_SESSION_DYN_ENABLED,
    CONFIG_DECLINED_NAMES_USED,
    CONFIG_INTERNAL_CACHE,

    // Logging
    CONFIG_CHATLOG_CHANNEL,
    CONFIG_CHATLOG_WHISPER,
    CONFIG_CHATLOG_SYSCHAN,
    CONFIG_CHATLOG_PARTY,
    CONFIG_CHATLOG_RAID,
    CONFIG_CHATLOG_GUILD,
    CONFIG_CHATLOG_PUBLIC,
    CONFIG_CHATLOG_ADDON,
    CONFIG_CHATLOG_BGROUND,
    CONFIG_CHATLOG_GM_ACTIVITY,

    // PLAYER INTERACTION
    CONFIG_ALLOW_TWO_SIDE_INTERACTION_CHAT,
    CONFIG_ALLOW_TWO_SIDE_INTERACTION_CHANNEL,
    CONFIG_ALLOW_TWO_SIDE_WHO_LIST,
    CONFIG_ALLOW_TWO_SIDE_ADD_FRIEND,

    // CHAT SETTINGS

    // GAME MASTER SETTINGS
    CONFIG_ALLOW_GM_FRIEND,
    CONFIG_ALLOW_GM_GROUP,
    CONFIG_GM_LOWER_SECURITY,

    // Groups
    CONFIG_ALLOW_TWO_SIDE_INTERACTION_GROUP,

    // Channels
    CONFIG_PRESERVE_CUSTOM_CHANNELS,
    CONFIG_RESTRICTED_LFG_CHANNEL,
    CONFIG_SILENTLY_GM_JOIN_TO_CHANNEL,

    // Tickets
    CONFIG_GMTICKET_ALLOW_TICKETS,

    // Misc
    CONFIG_DIE_COMMAND_MODE,

    BOOL_CONFIG_VALUE_COUNT
};

enum LogonFloatConfigs
{
    CONFIG_GMTICKET_CHANCE_OF_GM_SURVEY,
    FLOAT_CONFIG_VALUE_COUNT
};

struct CliCommandHolder
{
    typedef void Print(void*, const char*);
    typedef void CommandFinished(void*, bool success);

    void* m_callbackArg;
    char *m_command;
    Print* m_print;

    CommandFinished* m_commandFinished;

    CliCommandHolder(void* callbackArg, const char *command, Print* zprint, CommandFinished* commandFinished)
        : m_callbackArg(callbackArg), m_print(zprint), m_commandFinished(commandFinished)
    {
        size_t len = strlen(command)+1;
        m_command = new char[len];
        memcpy(m_command, command, len);
    }

    ~CliCommandHolder() { delete[] m_command; }
};

class Logon
{
    public:
        static Logon* instance()
        {
            static Logon instance;
            return &instance;
        }

        static std::atomic<uint32> m_logonLoopCounter;

        void Update(uint32 diff);

        void LoadConfigSettings(bool reload = false);
        void LoadDBVersion();
        char const* GetDBVersion() const { return m_DBVersion.c_str(); }

        void DetectDBCLang();
        LocaleConstant GetAvailableDbcLocale(LocaleConstant locale) const { if (m_availableDbcLocaleMask & (1 << locale)) return locale; else return m_defaultDbcLocale; }
        LocaleConstant GetDefaultDbcLocale() const { return m_defaultDbcLocale; }

        void SetInitialLogonSettings();
        
        // Global Player Data Storage system
        void LoadGlobalPlayerDataStore();
        uint32 GetGlobalPlayerGUID(std::string const& name) const;
        GlobalPlayerData const* GetGlobalPlayerData(uint32 guid) const;
        void AddGlobalPlayerData(uint32 guid, uint32 accountId, std::string const& name, uint8 gender, uint8 race, uint8 playerClass, uint8 level, uint16 mailCount, uint32 guildId);
        void UpdateGlobalPlayerData(uint32 guid, uint8 mask, std::string const& name, uint8 level = 0, uint8 gender = 0, uint8 race = 0, uint8 playerClass = 0);
        void UpdateGlobalPlayerMails(uint32 guid, int16 count, bool add = true);
        void UpdateGlobalPlayerGuild(uint32 guid, uint32 guildId);
        void UpdateGlobalPlayerGroup(uint32 guid, uint32 groupId);
        void UpdateGlobalPlayerArenaTeam(uint32 guid, uint8 slot, uint32 arenaTeamId);
        void UpdateGlobalNameData(uint32 guidLow, std::string const& oldName, std::string const& newName);
        void DeleteGlobalPlayerData(uint32 guid, std::string const& name);
        
        void SendGlobalMessage(WorldPacket* packet, ClientSession* self = 0, TeamId teamId = TEAM_NEUTRAL);
        void SendGlobalGMMessage(WorldPacket* packet, ClientSession* self = 0, TeamId teamId = TEAM_NEUTRAL);

        //ServerInfo
        bool IsClosed() const { return m_isClosed; }
        AccountTypes GetPlayerSecurityLimit() const { return m_allowedSecurityLevel; }
        //void SetPlayerSecurityLimit(AccountTypes sec);

        //Config-elements
        void setRate(LogonRates rate, float value) { rate_values[rate]=value; }
        float getRate(LogonRates rate) const { return rate_values[rate]; }
        uint32 getIntConfig(LogonIntConfigs index) const
        {
            return index < INT_CONFIG_VALUE_COUNT ? m_int_configs[index] : 0;

        }
        bool getBoolConfig(LogonBoolConfigs index) const
        {
            return index < BOOL_CONFIG_VALUE_COUNT ? m_bool_configs[index] : 0;
        }
        float getFloatConfig(LogonFloatConfigs index) const
        {
            return index < FLOAT_CONFIG_VALUE_COUNT ? m_float_configs[index] : 0;
        }

        //Shutdown
        bool IsShutdowning() const { return m_ShutdownTimer > 0; }
        void ShutdownServ(uint32 time, uint32 options, uint8 exitcode);
        void ShutdownCancel();
        void HaltServ(uint32 time, uint32 options);
        static uint8 GetExitCode() { return m_ExitCode; }
        static void StopNow(uint8 exitcode) { m_stopEvent = true; m_ExitCode = exitcode; }
        static bool IsStopped() { return m_stopEvent; }

        bool IsHalting() const { return m_HaltTimer > 0; }
        static bool IsHalted() { return m_haltEvent; }
        static void HaltNow() { m_haltEvent = true;}

        static void ContinueNow() { m_haltEvent = false;}

        BanReturn BanAccount(BanMode mode, std::string nameOrIP, std::string duration, std::string reason, std::string author);
        BanReturn BanAccountbyId(uint32 accountId, int32 duration, std::string reason, std::string author);
        bool RemoveBanAccount(BanMode mode, std::string nameOrIP);
        BanReturn BanCharacter(std::string name, std::string duration, std::string reason, std::string author);
        bool RemoveBanCharacter(std::string name);

        /// Are we on a "Player versus Player" server?
        bool IsPvPRealm() const { return (getIntConfig(CONFIG_GAME_TYPE) == REALM_TYPE_PVP || getIntConfig(CONFIG_GAME_TYPE) == REALM_TYPE_RPPVP || getIntConfig(CONFIG_GAME_TYPE) == REALM_TYPE_FFA_PVP); }
        bool IsFFAPvPRealm() const { return getIntConfig(CONFIG_GAME_TYPE) == REALM_TYPE_FFA_PVP; }

        //FakeMSG für Keazy
        void InitAntiSpam();
        char GetReplaceChar(uint16 c) const;
        std::string NormalizeForAntiSpam(std::string str) const;
        void InsertSpamMsg(std::string str);
        bool CheckForSpam(std::string data) const;
        void SetFakeMsg(const std::string& motd);
        const char* GetFakeMsg() const;

        //Whisper
        void Whisper(const std::string& text, uint32 language, Player* player);

        //States
        void setLogonState(uint32 index, uint64 value);
        uint64 getLogonState(uint32 index) const;
        void LoadLogonStates();

        //CLI
        void ProcessCliCommands();
        void QueueCliCommand(CliCommandHolder* commandHolder) { cliCmdQueue.add(commandHolder); }

        void UpdateRealmCharCount(uint32 accid);

        //Kleinkram
        uint32 debugOpcode;
        /// Update time
        uint32 GetUpdateTime() const { return m_updateTime; }
        uint32 GetUptime() const { return uint32(m_gameTime - m_startTime); } 

    protected:
        void _UpdateGameTime();
        void _UpdateRealmCharCount(PreparedQueryResult resultCharCount);

    private:
        Logon();
        ~Logon();
        ACE_Thread_Mutex _Mutex_;
        AntiSpam _antiSpam;
        std::unordered_map<int, char> _antiSpamReplace;
        std::string _fakemsg;
        std::string m_dataPath;
        static std::atomic<bool> m_stopEvent;
        static std::atomic<bool> m_haltEvent;
        static uint8 m_ExitCode;
        uint32 m_ShutdownTimer;
        uint32 m_ShutdownMask;
        uint32 m_HaltTimer;

        time_t m_startTime;
        time_t m_gameTime;
        uint32 m_updateTime, m_updateTimeSum;
        uint32 m_updateTimeCount;
        uint32 m_currentTime;
        IntervalTimer m_timers[LUPDATE_COUNT];

        std::string m_DBVersion;
        std::string m_CreatureEventAIVersion;

        //LogonStates
        typedef std::map<uint32, uint64> LogonStatesMap;
        LogonStatesMap m_logonstates;

        //Infos
        AccountTypes m_allowedSecurityLevel;
        bool m_isClosed;

        // CLI command holder to be thread safe
        ACE_Based::LockedQueue<CliCommandHolder*,ACE_Thread_Mutex> cliCmdQueue;

        //Rates
        float rate_values[MAX_RATES];
        uint32 m_int_configs[INT_CONFIG_VALUE_COUNT];
        bool m_bool_configs[BOOL_CONFIG_VALUE_COUNT];
        float m_float_configs[FLOAT_CONFIG_VALUE_COUNT];

        //Locales
        LocaleConstant m_defaultDbcLocale;                     // from config for one from loaded DBC locales
        uint32 m_availableDbcLocaleMask;                       // by loaded DBC

        void ProcessQueryCallbacks();
        ACE_Future_Set<PreparedQueryResult> m_realmCharCallbacks;
         
        GlobalPlayerDataMap _globalPlayerDataStore;
        GlobalPlayerNameMap _globalPlayerNameStore;

        std::string _realmName;
        
        SessionMap m_sessions;
        SessionMap m_offlineSessions;
};
extern uint32 realmID;
#define sLogon Logon::instance()
#endif

