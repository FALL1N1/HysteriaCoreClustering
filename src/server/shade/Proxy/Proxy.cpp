#include "Common.h"
#include "DatabaseEnv.h"
#include "Config.h"
#include "SystemConfig.h"
#include "Log.h"
#include "ClientSessionMgr.h"
#include "ClientSession.h"
#include "RoutingHelper.h"
#include "Chat.h"
#include "AchievementMgr.h"
#include "DBCStores.h"
#include "AccountMgr.h"

#include "Proxy.h"
#include "AddonMgr.h"
#include "ObjectMgr.h"
#include "LocalesMgr.h"
#include "MapHolder.h"

#include "WorldPacket.h"
#include "Opcodes.h"
#include "ControlSessionMgr.h"
#include "ControlSession.h"
#include "DatabaseRoutines.h"
#include "Warden.h"
#include "BattlegroundMgr.h" 
#include "GuildsMgr.h"

//Caching
#include "GameObjects.h"
#include "Creatures.h"
#include "Items.h"
#include "Quest.h"

//Tickets
#include "TicketMgr.h"
#include "GMQualityManager.h"

#include <algorithm>
#include <functional>

std::atomic<bool> Logon::m_stopEvent(false);
std::atomic<bool> Logon::m_haltEvent(false);
uint8 Logon::m_ExitCode = SHUTDOWN_EXIT_CODE;
std::atomic<uint32> Logon::m_logonLoopCounter(0);

Logon::Logon()
{
    m_ShutdownMask = 0;
    m_ShutdownTimer = 0;
    m_HaltTimer     = 0;

    m_defaultDbcLocale = LOCALE_enUS;

    m_gameTime = time(NULL);
    m_startTime = m_gameTime;
    m_isClosed = false;
    sClientSessionMgr->SetPlayerAmountLimit(0);
}

Logon::~Logon()
{
    sClientSessionMgr->ServerClose();
}

/// Initialize config values
void Logon::LoadConfigSettings(bool reload)
{
    if (reload)
        if (!sConfigMgr->Reload())
        {
            sLog->outError("ProxyServer settings reload failed, we can't read settings from %s.", sConfigMgr->GetFilename().c_str());
            return;
        }

    ///- Read the "Data" directory from the config file
    std::string dataPath = sConfigMgr->GetStringDefault("DataDir", "./");
    if (dataPath.at(dataPath.length()-1) != '/' && dataPath.at(dataPath.length()-1) != '\\')
        dataPath.append("/");

    if (reload)
    {
        if (dataPath != m_dataPath)
            sLog->outError("DataDir option can't be changed at worldserver.conf reload, using current value (%s).", m_dataPath.c_str());
    }
    else
    {
        m_dataPath = dataPath;
        sLog->outString("Using DataDir %s", m_dataPath.c_str());
    }

    SetFakeMsg("");

    sClientSessionMgr->SetPlayerAmountLimit(sConfigMgr->GetIntDefault("PlayerLimit", 100));

    if (reload)
    {
        uint32 val = sConfigMgr->GetIntDefault("LogonServerPort", 8085);
        if (val != m_int_configs[CONFIG_PORT_LOGON])
            sLog->outError("LogonServerPort option can't be changed at logonserver.conf reload, using current value (%u).",m_int_configs[CONFIG_PORT_LOGON]);
    }
    else
        m_int_configs[CONFIG_PORT_LOGON] = sConfigMgr->GetIntDefault("LogonServerPort", 8085);
    
    m_int_configs[CONFIG_BATTLEGROUND_INVITATION_TYPE] = sConfigMgr->GetIntDefault("DefaultBGInviteType", 1);
    m_int_configs[CONFIG_GROUP_VISIBILITY] = sConfigMgr->GetIntDefault("Visibility.GroupMode", 1);
    
    m_int_configs[CONFIG_SOCKET_TIMEOUTTIME] = sConfigMgr->GetIntDefault("SocketTimeOutTime", 900000);
    m_int_configs[CONFIG_MAX_OVERSPEED_PINGS] = sConfigMgr->GetIntDefault("MaxOverspeedPings",2);
    if (m_int_configs[CONFIG_MAX_OVERSPEED_PINGS] != 0 && m_int_configs[CONFIG_MAX_OVERSPEED_PINGS] < 2)
    {
        sLog->outError("MaxOverspeedPings (%i) must be in range 2..infinity (or 0 to disable check). Set to 2.",m_int_configs[CONFIG_MAX_OVERSPEED_PINGS]);
        m_int_configs[CONFIG_MAX_OVERSPEED_PINGS] = 2;
    }

    m_int_configs[CONFIG_SESSION_ADD_DELAY] = sConfigMgr->GetIntDefault("SessionAddDelay", 10000);

    if (reload)
    {
        uint32 val = sConfigMgr->GetIntDefault("GameType", 0);
        if (val != m_int_configs[CONFIG_GAME_TYPE])
            sLog->outError("GameType option can't be changed at logonserver.conf reload, using current value (%u).", m_int_configs[CONFIG_GAME_TYPE]);
    }
    else
        m_int_configs[CONFIG_GAME_TYPE] = sConfigMgr->GetIntDefault("GameType", 0);

    if (reload)
    {
        uint32 val = sConfigMgr->GetIntDefault("RealmZone", REALM_ZONE_DEVELOPMENT);
        if (val != m_int_configs[CONFIG_REALM_ZONE])
            sLog->outError("RealmZone option can't be changed at logonserver.conf reload, using current value (%u).", m_int_configs[CONFIG_REALM_ZONE]);
    }
    else
        m_int_configs[CONFIG_REALM_ZONE] = sConfigMgr->GetIntDefault("RealmZone", REALM_ZONE_DEVELOPMENT);

    if (reload)
    {
        uint32 val = sConfigMgr->GetIntDefault("Expansion",1);
        if (val != m_int_configs[CONFIG_EXPANSION])
            sLog->outError("Expansion option can't be changed at logonserver.conf reload, using current value (%u).",m_int_configs[CONFIG_EXPANSION]);
    }
    else
        m_int_configs[CONFIG_EXPANSION] = sConfigMgr->GetIntDefault("Expansion",3);

    m_int_configs[CONFIG_REBASE_INTERVAL] = sConfigMgr->GetIntDefault("OptimizeInterval", 7);

    if (int32 clientCacheId = sConfigMgr->GetIntDefault("ClientCacheVersion", 0))
    {
        // overwrite DB/old value
        if (clientCacheId > 0)
        {
            m_int_configs[CONFIG_CLIENTCACHE_VERSION] = clientCacheId;
            sLog->outString("Client cache version set to: %u", clientCacheId);
        }
        else
            sLog->outError("ClientCacheVersion can't be negative %d, ignored.", clientCacheId);
    }

    m_int_configs[CONFIG_UPTIME_UPDATE] = sConfigMgr->GetIntDefault("UpdateUptimeInterval", 10);
    if (int32(m_int_configs[CONFIG_UPTIME_UPDATE]) <= 0)
    {
        sLog->outError("UpdateUptimeInterval (%i) must be > 0, set to default 10.", m_int_configs[CONFIG_UPTIME_UPDATE]);
        m_int_configs[CONFIG_UPTIME_UPDATE] = 10;
    }
    if (reload)
    {
        m_timers[LUPDATE_UPTIME].SetInterval(m_int_configs[CONFIG_UPTIME_UPDATE]*MINUTE*IN_MILLISECONDS);
        m_timers[LUPDATE_UPTIME].Reset();
    }

    // MySQL ping time interval
    m_int_configs[CONFIG_DB_PING_INTERVAL]              = sConfigMgr->GetIntDefault("MaxPingTime", 30);
    if (!reload)
    {
        m_int_configs[CONFIG_SESSION_MT_THREADS]        = sConfigMgr->GetIntDefault("SessionThreadNumbers", 0);
        m_int_configs[CONFIG_SESSION_DYN_TIMER]         = sConfigMgr->GetIntDefault("DynamicThreading.Interval", 90000);
        m_int_configs[CONFIG_SESSION_DYN_MAX]           = sConfigMgr->GetIntDefault("DynamicThreading.Max", 30);
        m_bool_configs[CONFIG_SESSION_DYN_ENABLED]      = sConfigMgr->GetBoolDefault("DynamicThreading.Enabled", false);
        m_bool_configs[CONFIG_INTERNAL_CACHE]           = sConfigMgr->GetBoolDefault("InternalCache", true);
    }

    // Logging
    m_bool_configs[CONFIG_CHATLOG_CHANNEL]              = sConfigMgr->GetBoolDefault("ChatLogs.Channel", false);
    m_bool_configs[CONFIG_CHATLOG_WHISPER]              = sConfigMgr->GetBoolDefault("ChatLogs.Whisper", false);
    m_bool_configs[CONFIG_CHATLOG_SYSCHAN]              = sConfigMgr->GetBoolDefault("ChatLogs.SysChan", false);
    m_bool_configs[CONFIG_CHATLOG_PARTY]                = sConfigMgr->GetBoolDefault("ChatLogs.Party", false);
    m_bool_configs[CONFIG_CHATLOG_RAID]                 = sConfigMgr->GetBoolDefault("ChatLogs.Raid", false);
    m_bool_configs[CONFIG_CHATLOG_GUILD]                = sConfigMgr->GetBoolDefault("ChatLogs.Guild", false);
    m_bool_configs[CONFIG_CHATLOG_PUBLIC]               = sConfigMgr->GetBoolDefault("ChatLogs.Public", false);
    m_bool_configs[CONFIG_CHATLOG_ADDON]                = sConfigMgr->GetBoolDefault("ChatLogs.Addon", false);
    m_bool_configs[CONFIG_CHATLOG_BGROUND]              = sConfigMgr->GetBoolDefault("ChatLogs.Battleground", false);
    m_bool_configs[CONFIG_CHATLOG_GM_ACTIVITY]          = sConfigMgr->GetBoolDefault("ChatLogs.GM.Activity", false);

    // PLAYER INTERACTION
    m_bool_configs[CONFIG_ALLOW_TWO_SIDE_INTERACTION_CHAT]    = sConfigMgr->GetBoolDefault("AllowTwoSide.Interaction.Chat", false);
    m_bool_configs[CONFIG_ALLOW_TWO_SIDE_INTERACTION_CHANNEL] = sConfigMgr->GetBoolDefault("AllowTwoSide.Interaction.Channel", false);
    m_bool_configs[CONFIG_ALLOW_TWO_SIDE_WHO_LIST]      = sConfigMgr->GetBoolDefault("AllowTwoSide.WhoList", false);
    m_bool_configs[CONFIG_ALLOW_TWO_SIDE_ADD_FRIEND]    = sConfigMgr->GetBoolDefault("AllowTwoSide.AddFriend", false);

    // CHAT SETTINGS
    m_int_configs[CONFIG_CHAT_WHISPER_LEVEL_REQ]        = sConfigMgr->GetIntDefault("ChatLevelReq.Whisper", 1);

    // GAME MASTER SETTINGS
    m_int_configs[CONFIG_GM_LOGIN_STATE]                = sConfigMgr->GetIntDefault("GM.LoginState", 2);
    m_int_configs[CONFIG_GM_VISIBLE_STATE]              = sConfigMgr->GetIntDefault("GM.Visible", 2);
    m_int_configs[CONFIG_GM_CHAT]                       = sConfigMgr->GetIntDefault("GM.Chat", 2);
    m_int_configs[CONFIG_GM_WHISPERING_TO]              = sConfigMgr->GetIntDefault("GM.WhisperingTo", 2);
    m_int_configs[CONFIG_GM_LEVEL_IN_GM_LIST]           = sConfigMgr->GetIntDefault("GM.InGMList.Level", SEC_ADMINISTRATOR);
    m_int_configs[CONFIG_GM_LEVEL_IN_WHO_LIST]          = sConfigMgr->GetIntDefault("GM.InWhoList.Level", SEC_ADMINISTRATOR);
    m_bool_configs[CONFIG_ALLOW_GM_GROUP]               = sConfigMgr->GetBoolDefault("GM.AllowInvite", false);
    m_bool_configs[CONFIG_ALLOW_GM_FRIEND]              = sConfigMgr->GetBoolDefault("GM.AllowFriend", false);
    m_bool_configs[CONFIG_GM_LOWER_SECURITY]            = sConfigMgr->GetBoolDefault("GM.LowerSecurity", false);
    m_int_configs[CONFIG_GM_LEVEL_ALLOW_ACHIEVEMENTS]   = sConfigMgr->GetIntDefault("GM.AllowAchievementGain.Level", SEC_ADMINISTRATOR);

    // Chat & Channels


    // Warden
    m_int_configs[CONFIG_WARDEN_NUM_MEM_CHECKS]         = sConfigMgr->GetIntDefault("Warden.NumMemChecks", 3);
    m_int_configs[CONFIG_WARDEN_NUM_OTHER_CHECKS]       = sConfigMgr->GetIntDefault("Warden.NumOtherChecks", 7);
    m_int_configs[CONFIG_WARDEN_CLIENT_BAN_DURATION]    = sConfigMgr->GetIntDefault("Warden.BanDuration", 86400);
    m_int_configs[CONFIG_WARDEN_CLIENT_CHECK_HOLDOFF]   = sConfigMgr->GetIntDefault("Warden.ClientCheckHoldOff", 30);
    m_int_configs[CONFIG_WARDEN_CLIENT_RESPONSE_DELAY]  = sConfigMgr->GetIntDefault("Warden.ClientResponseDelay", 600);

    // Tickets
    m_bool_configs[CONFIG_GMTICKET_ALLOW_TICKETS] = sConfigMgr->GetBoolDefault("GMTicket.AllowTickets", true);
    m_int_configs[CONFIG_GMTICKET_LEVEL_REQ] = sConfigMgr->GetIntDefault("GMTicket.LevelReq", 1);
    m_float_configs[CONFIG_GMTICKET_CHANCE_OF_GM_SURVEY] = sConfigMgr->GetFloatDefault("GMTicket.ChanceOfGMSurvey", 50.0f);

    // Misc
    m_bool_configs[CONFIG_DIE_COMMAND_MODE] = sConfigMgr->GetBoolDefault("Die.Command.Mode", true);

    // Rates
    {
        rate_values[RATE_HEALTH]                        = sConfigMgr->GetFloatDefault("Rate.Health", 1);
        if (rate_values[RATE_HEALTH] < 0)
        {
            sLog->outError("Rate.Health (%f) must be > 0. Using 1 instead.", rate_values[RATE_HEALTH]);
            rate_values[RATE_HEALTH]                    = 1;
        }
        rate_values[RATE_POWER_MANA]                    = sConfigMgr->GetFloatDefault("Rate.Mana", 1);
        if (rate_values[RATE_POWER_MANA] < 0)
        {
            sLog->outError("Rate.Mana (%f) must be > 0. Using 1 instead.", rate_values[RATE_POWER_MANA]);
            rate_values[RATE_POWER_MANA]                = 1;
        }
        rate_values[RATE_POWER_RAGE_INCOME]             = sConfigMgr->GetFloatDefault("Rate.Rage.Income", 1);
        rate_values[RATE_POWER_RAGE_LOSS]               = sConfigMgr->GetFloatDefault("Rate.Rage.Loss", 1);
        if (rate_values[RATE_POWER_RAGE_LOSS] < 0)
        {
            sLog->outError("Rate.Rage.Loss (%f) must be > 0. Using 1 instead.", rate_values[RATE_POWER_RAGE_LOSS]);
            rate_values[RATE_POWER_RAGE_LOSS]           = 1;
        }
        rate_values[RATE_POWER_RUNICPOWER_INCOME]       = sConfigMgr->GetFloatDefault("Rate.RunicPower.Income", 1);
        rate_values[RATE_POWER_RUNICPOWER_LOSS]         = sConfigMgr->GetFloatDefault("Rate.RunicPower.Loss", 1);
        if (rate_values[RATE_POWER_RUNICPOWER_LOSS] < 0)
        {
            sLog->outError("Rate.RunicPower.Loss (%f) must be > 0. Using 1 instead.", rate_values[RATE_POWER_RUNICPOWER_LOSS]);
            rate_values[RATE_POWER_RUNICPOWER_LOSS]     = 1;
        }
        rate_values[RATE_POWER_FOCUS]                   = sConfigMgr->GetFloatDefault("Rate.Focus", 1.0f);
        rate_values[RATE_POWER_ENERGY]                  = sConfigMgr->GetFloatDefault("Rate.Energy", 1.0f);
        rate_values[RATE_SKILL_DISCOVERY]               = sConfigMgr->GetFloatDefault("Rate.Skill.Discovery", 1.0f);
        rate_values[RATE_DROP_ITEM_POOR]                = sConfigMgr->GetFloatDefault("Rate.Drop.Item.Poor", 1.0f);
        rate_values[RATE_DROP_ITEM_NORMAL]              = sConfigMgr->GetFloatDefault("Rate.Drop.Item.Normal", 1.0f);
        rate_values[RATE_DROP_ITEM_UNCOMMON]            = sConfigMgr->GetFloatDefault("Rate.Drop.Item.Uncommon", 1.0f);
        rate_values[RATE_DROP_ITEM_RARE]                = sConfigMgr->GetFloatDefault("Rate.Drop.Item.Rare", 1.0f);
        rate_values[RATE_DROP_ITEM_EPIC]                = sConfigMgr->GetFloatDefault("Rate.Drop.Item.Epic", 1.0f);
        rate_values[RATE_DROP_ITEM_LEGENDARY]           = sConfigMgr->GetFloatDefault("Rate.Drop.Item.Legendary", 1.0f);
        rate_values[RATE_DROP_ITEM_ARTIFACT]            = sConfigMgr->GetFloatDefault("Rate.Drop.Item.Artifact", 1.0f);
        rate_values[RATE_DROP_ITEM_REFERENCED]          = sConfigMgr->GetFloatDefault("Rate.Drop.Item.Referenced", 1.0f);
        rate_values[RATE_DROP_MONEY]                    = sConfigMgr->GetFloatDefault("Rate.Drop.Money", 1.0f);
        rate_values[RATE_DROP_ITEM_REFERENCED_AMOUNT]   = sConfigMgr->GetFloatDefault("Rate.Drop.Item.ReferencedAmount", 1.0f);
        rate_values[RATE_XP_KILL]                       = sConfigMgr->GetFloatDefault("Rate.XP.Kill", 1.0f);
        rate_values[RATE_XP_QUEST]                      = sConfigMgr->GetFloatDefault("Rate.XP.Quest", 1.0f);
        rate_values[RATE_XP_EXPLORE]                    = sConfigMgr->GetFloatDefault("Rate.XP.Explore", 1.0f);
        rate_values[RATE_REPAIRCOST]                    = sConfigMgr->GetFloatDefault("Rate.RepairCost", 1.0f);
        if (rate_values[RATE_REPAIRCOST] < 0.0f)
        {
            sLog->outError("Rate.RepairCost (%f) must be >=0. Using 0.0 instead.", rate_values[RATE_REPAIRCOST]);
            rate_values[RATE_REPAIRCOST] = 0.0f;
        }
        rate_values[RATE_REST_INGAME]                   = sConfigMgr->GetFloatDefault("Rate.Rest.InGame", 1.0f);
        rate_values[RATE_REST_OFFLINE_IN_TAVERN_OR_CITY] = sConfigMgr->GetFloatDefault("Rate.Rest.Offline.InTavernOrCity", 1.0f);
        rate_values[RATE_REST_OFFLINE_IN_WILDERNESS]    = sConfigMgr->GetFloatDefault("Rate.Rest.Offline.InWilderness", 1.0f);
        rate_values[RATE_DAMAGE_FALL]                   = sConfigMgr->GetFloatDefault("Rate.Damage.Fall", 1.0f);
        rate_values[RATE_AUCTION_TIME]                  = sConfigMgr->GetFloatDefault("Rate.Auction.Time", 1.0f);
        rate_values[RATE_AUCTION_DEPOSIT]               = sConfigMgr->GetFloatDefault("Rate.Auction.Deposit", 1.0f);
        rate_values[RATE_AUCTION_CUT]                   = sConfigMgr->GetFloatDefault("Rate.Auction.Cut", 1.0f);
        rate_values[RATE_HONOR]                         = sConfigMgr->GetFloatDefault("Rate.Honor", 1.0f);
        rate_values[RATE_MINING_AMOUNT]                 = sConfigMgr->GetFloatDefault("Rate.Mining.Amount", 1.0f);
        rate_values[RATE_MINING_NEXT]                   = sConfigMgr->GetFloatDefault("Rate.Mining.Next", 1.0f);
        rate_values[RATE_TALENT]                        = sConfigMgr->GetFloatDefault("Rate.Talent", 1.0f);
        if (rate_values[RATE_TALENT] < 0.0f)
        {
            sLog->outError("Rate.Talent (%f) must be > 0. Using 1 instead.", rate_values[RATE_TALENT]);
            rate_values[RATE_TALENT] = 1.0f;
        }
        rate_values[RATE_REPUTATION_GAIN]               = sConfigMgr->GetFloatDefault("Rate.Reputation.Gain", 1.0f);
        rate_values[RATE_REPUTATION_LOWLEVEL_KILL]      = sConfigMgr->GetFloatDefault("Rate.Reputation.LowLevel.Kill", 1.0f);
        rate_values[RATE_REPUTATION_LOWLEVEL_QUEST]     = sConfigMgr->GetFloatDefault("Rate.Reputation.LowLevel.Quest", 1.0f);
        rate_values[RATE_REPUTATION_RECRUIT_A_FRIEND_BONUS] = sConfigMgr->GetFloatDefault("Rate.Reputation.RecruitAFriendBonus", 0.1f);

        m_int_configs[CONFIG_SKILL_GAIN_CRAFTING]       = sConfigMgr->GetIntDefault("SkillGain.Crafting", 1);
        m_int_configs[CONFIG_SKILL_GAIN_DEFENSE]        = sConfigMgr->GetIntDefault("SkillGain.Defense", 1);
        m_int_configs[CONFIG_SKILL_GAIN_GATHERING]      = sConfigMgr->GetIntDefault("SkillGain.Gathering", 1);
        m_int_configs[CONFIG_SKILL_GAIN_WEAPON]         = sConfigMgr->GetIntDefault("SkillGain.Weapon", 1);
        m_int_configs[CONFIG_SKILL_CHANCE_ORANGE]       = sConfigMgr->GetIntDefault("SkillChance.Orange", 100);
        m_int_configs[CONFIG_SKILL_CHANCE_YELLOW]       = sConfigMgr->GetIntDefault("SkillChance.Yellow", 75);
        m_int_configs[CONFIG_SKILL_CHANCE_GREEN]        = sConfigMgr->GetIntDefault("SkillChance.Green", 25);
        m_int_configs[CONFIG_SKILL_CHANCE_GREY]         = sConfigMgr->GetIntDefault("SkillChance.Grey", 0);
        m_int_configs[CONFIG_SKILL_CHANCE_MINING_STEPS]  = sConfigMgr->GetIntDefault("SkillChance.MiningSteps", 75);
        m_int_configs[CONFIG_SKILL_CHANCE_SKINNING_STEPS]   = sConfigMgr->GetIntDefault("SkillChance.SkinningSteps", 75);
    }
}

/// Initialize logonserver
void Logon::SetInitialLogonSettings()
{
    ///- Server startup begin
    uint32 startupBegin = getMSTime();

    ///- Initialize the random number generator
    srand((unsigned int)time(NULL));

    sLog->outString();
    sLog->outString("Deleting expired bans...");
    LoginDatabase.Execute("DELETE FROM ip_banned WHERE unbandate <= UNIX_TIMESTAMP() AND unbandate<>bandate");      // One-time query

    ///- Loading strings. Getting no records means core load has to be canceled because no error message can be output.
    sLog->outString();
    sLog->outString("Loading localization template...");
    if (!sObjectMgr->LoadTrinityStrings())
        exit(1);                                            // Error message displayed in function already

    //Update Realmlist
    uint32 server_type;
    if (IsFFAPvPRealm())
        server_type = REALM_TYPE_PVP;
    else
        server_type = getIntConfig(CONFIG_GAME_TYPE);
    uint32 realm_zone = getIntConfig(CONFIG_REALM_ZONE);
    LoginDatabase.PExecute("UPDATE realmlist SET icon = %u, timezone = %u WHERE id = '%d'", server_type, realm_zone, realmID);

    ///- Init highest guids before any table loading to prevent using not initialized guids in some code.
    sLog->outString("Calculating highest guids...");
    sObjectMgr->SetHighestGuids();

    //sLog->outString("Loading Character-Item GUIDs...");
    //sGuidHolder->Initialize();

    ///- Load the DBC files
    LoadDBCStores(m_dataPath);
    DetectDBCLang();
    
    // Global Storage, should be loaded asap
    sLog->outString("Loading Global Player Data...");
    sLogon->LoadGlobalPlayerDataStore();
    
    sLocalesMgr->LoadLocales();
    sAddonMgr->LoadFromDB();
    sGameObjects->WarmingCache();
    sCreatures->WarmingCache();
    sQuest->WarmingCache();
    sLog->outString();
    sItems->WarmingCache();
    sLog->outString();
    sAchievementMgr->LoadAchievementReferenceList();
    sAchievementMgr->LoadAchievementCriteriaList();
    sAchievementMgr->LoadAchievementCriteriaData();
    sAchievementMgr->LoadRewards();
    sAchievementMgr->LoadRewardLocales();
    sAchievementMgr->LoadCompletedAchievements();
    LoadLogonStates();
    sRoutingHelper->LoadNodeList();
    sRoutingHelper->LoadNodeMap();
    InitAntiSpam();
    sClientSessionMgr->Initialize();
    sTicketMgr->LoadTickets();
    sTicketMgr->Initialize();
    sWardenCheckMgr->LoadWardenChecks();
    sWardenCheckMgr->LoadWardenOverrides();
    sTicketMgr->LoadSurveys();
    sGMQualityManager->Load();
    sBattlegroundMgr->CreateInitialBattlegrounds(); 
    //sGroupMgr->LoadGroups();
    sGuildMgr->LoadGuilds();
    
    
    m_gameTime = time(NULL);
    m_startTime=m_gameTime;

    tm local;
    time_t curr;
    time(&curr);
    local=*(localtime(&curr));                              // dereference and assign
    char isoDate[128];
    sprintf(isoDate, "%04d-%02d-%02d %02d:%02d:%02d",
        local.tm_year+1900, local.tm_mon+1, local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);

    m_timers[LUPDATE_UPTIME].SetInterval(m_int_configs[CONFIG_UPTIME_UPDATE]*MINUTE*IN_MILLISECONDS);
    m_timers[LUPDATE_PINGDB].SetInterval(getIntConfig(CONFIG_DB_PING_INTERVAL)*MINUTE*IN_MILLISECONDS);    // Mysql ping time in minutes
    m_timers[LUPDATE_RECONNECT].SetInterval((MINUTE*IN_MILLISECONDS)/2);
    m_timers[LUPDATE_BAN].SetInterval(5*MINUTE*IN_MILLISECONDS); // Update expired bans

    //m_timers[LUPDATE_RECONNECT].SetInterval((IN_MILLISECONDS));

    //RebaseTimer
    sRebase->InitAutomaticRebase();

    uint32 startupDuration = GetMSTimeDiffToNow(startupBegin);
    sLog->outString();
    sLog->outString("Proxy Server initialized in %u minutes %u seconds", (startupDuration / 60000), ((startupDuration % 60000) / 1000) );
    sLog->outString("We are now accepting connections."); 
    sLog->outString();
}

void Logon::Update(uint32 diff)
{
    m_updateTime = diff;

    ///- Update the different timers
    for (int i = 0; i < LUPDATE_COUNT; ++i)
    {
        if (m_timers[i].GetCurrent() >= 0)
            m_timers[i].Update(diff);
        else
            m_timers[i].SetCurrent(0);
    }

    ///- Update the game time and check for shutdown time
    _UpdateGameTime();

    ///- Refreshing onlinelist
    if (m_timers[LUPDATE_RECONNECT].Passed())
    {
        m_timers[LUPDATE_RECONNECT].Reset();
        sRoutingHelper->RefreshOnlineStat();

        //30 Sek should be good enough for rebasecheck
        sRebase->UpdateTimers(diff);
    }

    ///- Ping to keep MySQL connections alive
    if (m_timers[LUPDATE_PINGDB].Passed())
    {
        m_timers[LUPDATE_PINGDB].Reset();
        sLog->outDetail("Ping MySQL to keep connection alive");
        CharacterDatabase.KeepAlive();
        LogonDatabase.KeepAlive();
        LoginDatabase.KeepAlive();
        WorldDatabase.KeepAlive();
    }

    /// <li> Update uptime table
    if (m_timers[LUPDATE_UPTIME].Passed())
    {
        uint32 tmpDiff = uint32(m_gameTime - m_startTime);
        uint32 maxOnlinePlayers = sClientSessionMgr->GetMaxPlayerCount();
        uint32 onlinePlayers = sClientSessionMgr->GetPlayerCount();
        uint32 allyPlayers = sClientSessionMgr->GetAllyCount();
        uint32 hordePlayers = sClientSessionMgr->GetHordeCount();

        m_timers[LUPDATE_UPTIME].Reset();

        PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_UPTIME_PLAYERS);

        stmt->setUInt64(0, uint64(tmpDiff));
        stmt->setUInt16(1, uint16(maxOnlinePlayers));
        stmt->setUInt32(2, realmID);
        stmt->setUInt64(3, uint64(m_startTime));

        LoginDatabase.Execute(stmt);
    }

    /// Update expired bans
    if (m_timers[LUPDATE_BAN].Passed())
    {
        // Delete expired ip bans
        LoginDatabase.Execute(LoginDatabase.GetPreparedStatement(LOGIN_DEL_EXPIRED_IP_BANS));
        // Update expired account bans
        LoginDatabase.Execute(LoginDatabase.GetPreparedStatement(LOGIN_UPD_EXPIRED_ACCOUNT_BANS));

        m_timers[LUPDATE_BAN].Reset();
    }

    sObjectMgr->Update();
    sControlSessionMgr->Update(diff);
    sClientSessionMgr->Update(diff);

    ProcessQueryCallbacks();
    ProcessCliCommands();
}

void Logon::ProcessQueryCallbacks()
{
    PreparedQueryResult result;

    while (!m_realmCharCallbacks.is_empty())
    {
        ACE_Future<PreparedQueryResult> lResult;
        ACE_Time_Value timeout = ACE_Time_Value::zero;
        if (m_realmCharCallbacks.next_readable(lResult, &timeout) != 1)
            break;

        if (lResult.ready())
        {
            lResult.get(result);
            _UpdateRealmCharCount(result);
            lResult.cancel();
        }
    }
}

void Logon::UpdateRealmCharCount(uint32 accountId)
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_COUNT);
    stmt->setUInt32(0, accountId);
    PreparedQueryResultFuture result = CharacterDatabase.AsyncQuery(stmt);
    m_realmCharCallbacks.insert(result);
}

void Logon::_UpdateRealmCharCount(PreparedQueryResult resultCharCount)
{
    if (resultCharCount)
    {
        Field* fields = resultCharCount->Fetch();
        uint32 accountId = fields[0].GetUInt32();
        uint32 charCount = fields[1].GetUInt32();

        PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_REALM_CHARACTERS);
        stmt->setUInt32(0, accountId);
        stmt->setUInt32(1, realmID);
        LoginDatabase.Execute(stmt);

        stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_REALM_CHARACTERS);
        stmt->setUInt32(0, charCount);
        stmt->setUInt32(1, accountId);
        stmt->setUInt32(2, realmID);
        LoginDatabase.Execute(stmt);
    }
}

/// Update the game time
void Logon::_UpdateGameTime()
{
    ///- update the time
    time_t thisTime = time(NULL);
    uint32 elapsed = uint32(thisTime - m_gameTime);
    m_gameTime = thisTime;

    if (!m_haltEvent && m_HaltTimer > 0 && elapsed > 0)
    {
        ///- ... and it is overdue, stop the world (set m_stopEvent)
        if (m_HaltTimer <= elapsed)
        {
            LoginDatabase.DirectPExecute("UPDATE realmlist SET flag = 2 WHERE id = '%d'", 2, 1, realmID);
            m_HaltTimer = 0;
            m_haltEvent = true;
        }
        else
        {
            m_HaltTimer -= elapsed;

            sClientSessionMgr->HaltMsg(m_HaltTimer, m_ShutdownMask);
        }
    }

    if (!m_stopEvent && m_ShutdownTimer > 0 && elapsed > 0)
    {
        ///- ... and it is overdue, stop the world (set m_stopEvent)
        if (m_ShutdownTimer <= elapsed)
        {
            if (!(m_ShutdownMask & SHUTDOWN_MASK_IDLE) || sClientSessionMgr->GetActiveAndQueuedSessionCount() == 0)
                m_stopEvent = true;                         // exist code already set
            else
                m_ShutdownTimer = 1;                        // minimum timer value to wait idle state
        }
        ///- ... else decrease it and if necessary display a shutdown countdown to the users
        else
        {
            m_ShutdownTimer -= elapsed;

            sClientSessionMgr->ShutdownMsg(m_ShutdownTimer, m_ShutdownMask);
        }
    }
}

/// Halt the server
void Logon::HaltServ(uint32 time, uint32 options)
{
    m_ShutdownMask = options;
    if (time == 0)  ///- If the halt time is 0, set m_haltEvent
        m_haltEvent = true;
    else            ///- Else set the halt timer and warn users
    {
        m_HaltTimer = time;
        sClientSessionMgr->ShutdownMsg(m_ShutdownTimer, m_ShutdownMask, true);
    }
}

/// Shutdown the server
void Logon::ShutdownServ(uint32 time, uint32 options, uint8 exitcode)
{
    // ignore if server shutdown at next tick
    if (m_stopEvent)
        return;

    m_ShutdownMask = options;
    m_ExitCode = exitcode;

    ///- If the shutdown time is 0, set m_stopEvent (except if shutdown is 'idle' with remaining sessions)
    if (time == 0)
    {
        if (!(options & SHUTDOWN_MASK_IDLE) || sClientSessionMgr->GetActiveAndQueuedSessionCount() == 0)
            m_stopEvent = true;                             // exist code already set
        else
            m_ShutdownTimer = 1;                            //So that the session count is re-evaluated at next world tick
    }
    ///- Else set the shutdown timer and warn users
    else
    {
        m_ShutdownTimer = time;
        sClientSessionMgr->ShutdownMsg(m_ShutdownTimer, m_ShutdownMask, true);
    }
}

/// Cancel a planned server shutdown
void Logon::ShutdownCancel()
{
    // nothing cancel or too later
    if (!m_ShutdownTimer || m_stopEvent)
        return;

    ServerMessageType msgid = (m_ShutdownMask & SHUTDOWN_MASK_RESTART) ? SERVER_MSG_RESTART_CANCELLED : SERVER_MSG_SHUTDOWN_CANCELLED;

    m_ShutdownMask = 0;
    m_ShutdownTimer = 0;
    m_ExitCode = SHUTDOWN_EXIT_CODE;                       // to default value
    sClientSessionMgr->SendServerMessage(msgid);

    sLog->outStaticDebug("Server %s cancelled.",(m_ShutdownMask & SHUTDOWN_MASK_RESTART ? "restart" : "shuttingdown"));
}

void Logon::ProcessCliCommands()
{
    CliCommandHolder::Print* zprint = NULL;
    void* callbackArg = NULL;
    CliCommandHolder* command;
    while (cliCmdQueue.next(command))
    {
        sLog->outDetail("CLI command under processing...");
        zprint = command->m_print;
        callbackArg = command->m_callbackArg;
        CliHandler handler(callbackArg, zprint);
        handler.ParseCommands(command->m_command);
        if(command->m_commandFinished)
            command->m_commandFinished(callbackArg, !handler.HasSentErrorMessage());
        delete command;
    }
}

void Logon::DetectDBCLang()
{
    uint8 m_lang_confid = sConfigMgr->GetIntDefault("DBC.Locale", 255);

    if (m_lang_confid != 255 && m_lang_confid >= TOTAL_LOCALES)
    {
        sLog->outError("Incorrect DBC.Locale! Must be >= 0 and < %d (set to 0)", TOTAL_LOCALES);
        m_lang_confid = LOCALE_enUS;
    }

    ChrRacesEntry const* race = sChrRacesStore.LookupEntry(1);

    std::string availableLocalsStr;

    uint8 default_locale = TOTAL_LOCALES;
    for (uint8 i = default_locale-1; i < TOTAL_LOCALES; --i)  // -1 will be 255 due to uint8
    {
        if (race->name[i][0] != '\0')                     // check by race names
        {
            default_locale = i;
            m_availableDbcLocaleMask |= (1 << i);
            availableLocalsStr += localeNames[i];
            availableLocalsStr += " ";
        }
    }

    if (default_locale != m_lang_confid && m_lang_confid < TOTAL_LOCALES &&
        (m_availableDbcLocaleMask & (1 << m_lang_confid)))
    {
        default_locale = m_lang_confid;
    }

    if (default_locale >= TOTAL_LOCALES)
    {
        sLog->outError("Unable to determine your DBC Locale! (corrupt DBC?)");
        exit(1);
    }

    m_defaultDbcLocale = LocaleConstant(default_locale);

    sLog->outString("Using %s DBC Locale as default. All available DBC locales: %s", localeNames[m_defaultDbcLocale], availableLocalsStr.empty() ? "<none>" : availableLocalsStr.c_str());
    sLog->outString();
}

//LogonState Routines
void Logon::LoadLogonStates()
{
    uint32 oldMSTime = getMSTime();

	QueryResult result = LogonDatabase.PQuery("SELECT entry, value FROM logonstates WHERE realmid = '%u'", realmID);

    if (!result)
    {
        sLog->outString(">> Loaded 0 logon states. DB table `logonstates` is empty!");
        sLog->outString();
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();
        m_logonstates[fields[0].GetUInt32()] = fields[1].GetUInt32();
        ++count;
    }
    while (result->NextRow());

    sLog->outString(">> Loaded %u logon states in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    sLog->outString();
}

void Logon::setLogonState(uint32 index, uint64 value)
{
    LogonStatesMap::const_iterator it = m_logonstates.find(index);
    if (it != m_logonstates.end())
    {
        PreparedStatement* stmt = LogonDatabase.GetPreparedStatement(LOGON_UPD_LOGON_STATE);

        stmt->setUInt32(0, uint32(value));
        stmt->setUInt32(1, index);
		stmt->setUInt32(2, realmID);

        LogonDatabase.Execute(stmt);
    }
    else
    {
        PreparedStatement* stmt = LogonDatabase.GetPreparedStatement(LOGON_INS_LOGON_STATE);

		stmt->setUInt32(0, realmID);
        stmt->setUInt32(1, index);
        stmt->setUInt32(2, uint32(value));

        LogonDatabase.Execute(stmt);
    }
    m_logonstates[index] = value;
}

uint64 Logon::getLogonState(uint32 index) const
{
    LogonStatesMap::const_iterator it = m_logonstates.find(index);
    return it != m_logonstates.end() ? it->second : 0;
}

//Bans
/// Ban an account or ban an IP address, duration will be parsed using TimeStringToSecs if it is positive, otherwise permban
BanReturn Logon::BanAccount(BanMode mode, std::string nameOrIP, std::string duration, std::string reason, std::string author)
{
    uint32 duration_secs = TimeStringToSecs(duration);
    PreparedQueryResult resultAccounts = PreparedQueryResult(NULL); //used for kicking
    PreparedStatement* stmt = NULL;

    ///- Update the database with ban information
    switch (mode)
    {
        case BAN_IP:
            // No SQL injection with prepared statements
            stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_BY_IP);
            stmt->setString(0, nameOrIP);
            resultAccounts = LoginDatabase.Query(stmt);
            stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_IP_BANNED);
            stmt->setString(0, nameOrIP);
            stmt->setUInt32(1, duration_secs);
            stmt->setString(2, author);
            stmt->setString(3, reason);
            LoginDatabase.Execute(stmt);
            break;
        case BAN_ACCOUNT:
            // No SQL injection with prepared statements
            stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_ID_BY_NAME);
            stmt->setString(0, nameOrIP);
            resultAccounts = LoginDatabase.Query(stmt);
            break;
        case BAN_CHARACTER:
            // No SQL injection with prepared statements
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ACCOUNT_BY_NAME);
            stmt->setString(0, nameOrIP);
            resultAccounts = CharacterDatabase.Query(stmt);
            break;
        default:
            return BAN_SYNTAX_ERROR;
    }

    if (!resultAccounts)
    {
        if (mode == BAN_IP)
            return BAN_SUCCESS;                             // ip correctly banned but nobody affected (yet)
        else
            return BAN_NOTFOUND;                            // Nobody to ban
    }

    ///- Disconnect all affected players (for IP it can be several)
    SQLTransaction trans = LoginDatabase.BeginTransaction();
    do
    {
        Field* fieldsAccount = resultAccounts->Fetch();
        uint32 account = fieldsAccount[0].GetUInt32();

        if (mode != BAN_IP)
        {
            // make sure there is only one active ban
            stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_NOT_BANNED);
            stmt->setUInt32(0, account);
            trans->Append(stmt);
            // No SQL injection with prepared statements
            stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_ACCOUNT_BANNED);
            stmt->setUInt32(0, account);
            stmt->setUInt32(1, duration_secs);
            stmt->setString(2, author);
            stmt->setString(3, reason);
            trans->Append(stmt);
        }

        if (ClientSession* sess = sClientSessionMgr->FindSession(account))
            if (std::string(sess->GetPlayer()->GetPlayerName()) != author)
                sess->KickPlayer();
    } while (resultAccounts->NextRow());

    LoginDatabase.CommitTransaction(trans);

    return BAN_SUCCESS;
}

/// Ban an account or ban an IP address, duration will be parsed using TimeStringToSecs if it is positive, otherwise permban
BanReturn Logon::BanAccountbyId(uint32 accountId, int32 duration, std::string reason, std::string author)
{
    PreparedStatement* stmt = NULL;

    ///- Disconnect all affected players (for IP it can be several)
    SQLTransaction trans = LoginDatabase.BeginTransaction();
    // make sure there is only one active ban
    stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_NOT_BANNED);
    stmt->setUInt32(0, accountId);
    trans->Append(stmt);
    // No SQL injection with prepared statements
    stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_ACCOUNT_BANNED);
    stmt->setUInt32(0, accountId);
    stmt->setUInt32(1, duration);
    stmt->setString(2, author);
    stmt->setString(3, reason);
    trans->Append(stmt);

    if (ClientSession* sess = sClientSessionMgr->FindSession(accountId))
        if (Player *plr = sess->GetPlayer())
            if (std::string(plr->GetName()) != author)
                sess->KickPlayer();

    LoginDatabase.CommitTransaction(trans);

    return BAN_SUCCESS;
}

/// Remove a ban from an account or IP address
bool Logon::RemoveBanAccount(BanMode mode, std::string nameOrIP)
{
    PreparedStatement* stmt = NULL;
    if (mode == BAN_IP)
    {
        stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_IP_NOT_BANNED);
        stmt->setString(0, nameOrIP);
        LoginDatabase.Execute(stmt);
    }
    else
    {
        uint32 account = 0;
        if (mode == BAN_ACCOUNT)
            account = AccountMgr::GetId(nameOrIP);
        else if (mode == BAN_CHARACTER)
            account = sObjectMgr->FindPlayerByName(nameOrIP.c_str())->GetSession()->GetAccountId();

        if (!account)
            return false;

        //NO SQL injection as account is uint32
        stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_NOT_BANNED);
        stmt->setUInt32(0, account);
        LoginDatabase.Execute(stmt);
    }
    return true;
}

/// Ban an account or ban an IP address, duration will be parsed using TimeStringToSecs if it is positive, otherwise permban
BanReturn Logon::BanCharacter(std::string name, std::string duration, std::string reason, std::string author)
{
    Player* pBanned = sObjectMgr->FindPlayerByName(name.c_str());
    uint32 guid = 0;

    uint32 duration_secs = TimeStringToSecs(duration);

    /// Pick a player to ban if not online
    if (!pBanned)
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GUID_BY_NAME_FILTER);
        stmt->setString(0, name);
        PreparedQueryResult resultCharacter = CharacterDatabase.Query(stmt);

        if (!resultCharacter)
            return BAN_NOTFOUND;                                    // Nobody to ban

        guid = (*resultCharacter)[0].GetUInt32();
    }
    else
        guid = pBanned->GetGUIDLow();

    // make sure there is only one active ban
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHARACTER_BAN);
    stmt->setUInt32(0, guid);
    CharacterDatabase.Execute(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER_BAN);
    stmt->setUInt32(0, guid);
    stmt->setUInt32(1, duration_secs);
    stmt->setString(2, author);
    stmt->setString(3, reason);
    CharacterDatabase.Execute(stmt);

    if (pBanned)
        pBanned->GetSession()->KickPlayer();

    return BAN_SUCCESS;
}

/// Remove a ban from a character
bool Logon::RemoveBanCharacter(std::string name)
{
    Player* pBanned = sObjectMgr->FindPlayerByName(name.c_str());
    uint32 guid = 0;

    /// Pick a player to ban if not online
    if (!pBanned)
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GUID_BY_NAME_FILTER);
        stmt->setString(0, name);
        PreparedQueryResult resultCharacter = CharacterDatabase.Query(stmt);

        if (!resultCharacter)
            return false;

        guid = (*resultCharacter)[0].GetUInt32();
    }
    else
        guid = pBanned->GetGUIDLow();

    if (!guid)
        return false;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHARACTER_BAN);
    stmt->setUInt32(0, guid);
    CharacterDatabase.Execute(stmt);
    return true;
}

void Logon::InitAntiSpam()
{
    return;
    TRINITY_GUARD(ACE_Thread_Mutex, _Mutex_);
    _antiSpam.clear();
    QueryResult result = LoginDatabase.Query("SELECT input FROM antispam");

    int count = 0;
    if (!result)
    {
        sLog->outString();
        sLog->outString(">> Loaded %u descriptions ", count);
        return;
    }

    do
    {
        Field *fields = result->Fetch();

        InsertSpamMsg(fields[0].GetString());
        ++count;

    }
    while (result->NextRow());

    _antiSpamReplace[0xc2a1] = 'i';
    _antiSpamReplace[0xc2a2] = 'c';
    _antiSpamReplace[0xc2a5] = 'y';
    _antiSpamReplace[0xc2a6] = 'i';
    _antiSpamReplace[0xc380] = 'a';
    _antiSpamReplace[0xc381] = 'a';
    _antiSpamReplace[0xc382] = 'a';
    _antiSpamReplace[0xc383] = 'a';
    _antiSpamReplace[0xc384] = 'a';
    _antiSpamReplace[0xc385] = 'a';
    _antiSpamReplace[0xc386] = 'a';
    _antiSpamReplace[0xc387] = 'c';
    _antiSpamReplace[0xc388] = 'e';
    _antiSpamReplace[0xc389] = 'e';
    _antiSpamReplace[0xc38a] = 'e';
    _antiSpamReplace[0xc38b] = 'e';
    _antiSpamReplace[0xc38c] = 'i';
    _antiSpamReplace[0xc38d] = 'i';
    _antiSpamReplace[0xc38e] = 'i';
    _antiSpamReplace[0xc38f] = 'i';
    _antiSpamReplace[0xc390] = 'd';
    _antiSpamReplace[0xc391] = 'n';
    _antiSpamReplace[0xc392] = 'o';
    _antiSpamReplace[0xc393] = 'o';
    _antiSpamReplace[0xc394] = 'o';
    _antiSpamReplace[0xc395] = 'o';
    _antiSpamReplace[0xc396] = 'o';
    _antiSpamReplace[0xc397] = 'x';
    _antiSpamReplace[0xc398] = 'o';
    _antiSpamReplace[0xc399] = 'u';
    _antiSpamReplace[0xc39a] = 'u';
    _antiSpamReplace[0xc39b] = 'u';
    _antiSpamReplace[0xc39c] = 'u';
    _antiSpamReplace[0xc39d] = 'y';
    _antiSpamReplace[0xc39e] = 'p';
    _antiSpamReplace[0xc39f] = 's';
    _antiSpamReplace[0xc3a0] = 'a';
    _antiSpamReplace[0xc3a1] = 'a';
    _antiSpamReplace[0xc3a2] = 'a';
    _antiSpamReplace[0xc3a3] = 'a';
    _antiSpamReplace[0xc3a4] = 'a';
    _antiSpamReplace[0xc3a5] = 'a';
    _antiSpamReplace[0xc3a6] = 'a';
    _antiSpamReplace[0xc3a7] = 'c';
    _antiSpamReplace[0xc3a8] = 'e';
    _antiSpamReplace[0xc3a9] = 'e';
    _antiSpamReplace[0xc3aa] = 'e';
    _antiSpamReplace[0xc3ab] = 'e';
    _antiSpamReplace[0xc3ac] = 'i';
    _antiSpamReplace[0xc3ad] = 'i';
    _antiSpamReplace[0xc3ae] = 'i';
    _antiSpamReplace[0xc3af] = 'i';
    _antiSpamReplace[0xc3b0] = 'd';
    _antiSpamReplace[0xc3b1] = 'n';
    _antiSpamReplace[0xc3b2] = 'o';
    _antiSpamReplace[0xc3b3] = 'o';
    _antiSpamReplace[0xc3b4] = 'o';
    _antiSpamReplace[0xc3b5] = 'o';
    _antiSpamReplace[0xc3b6] = 'o';
    _antiSpamReplace[0xc3b8] = 'o';
    _antiSpamReplace[0xc3b9] = 'u';
    _antiSpamReplace[0xc3ba] = 'u';
    _antiSpamReplace[0xc3bb] = 'u';
    _antiSpamReplace[0xc3bc] = 'u';
    _antiSpamReplace[0xc3bd] = 'y';
    _antiSpamReplace[0xc3be] = 'p';
    _antiSpamReplace[0xc3bf] = 'y';
}

char Logon::GetReplaceChar(uint16 c) const
{
    std::unordered_map<int, char>::const_iterator itr = _antiSpamReplace.find(c);

    if (itr == _antiSpamReplace.end())
        return c;

    return itr->second;
}

std::string Logon::NormalizeForAntiSpam(std::string str) const
{
    std::string normalized;

    uint16 special = 0;

    for (std::string::iterator itr = str.begin(); itr != str.end(); ++itr)
    {
        uint8 c = 0xFF & (*itr);

        if (c < 0x80 || special)
        {
            char newchar = GetReplaceChar(special | c);
            special = 0;

            if (isalnum(newchar))
                normalized += toupper(newchar);
        }
        else
            special = c << 8;
    }

    return normalized;
}

void Logon::InsertSpamMsg(std::string str)
{
    _antiSpam.insert(NormalizeForAntiSpam(str));
}

bool Logon::CheckForSpam(std::string data) const
{
    std::string str = NormalizeForAntiSpam(data);

    for (AntiSpam::iterator itr = _antiSpam.begin(); itr != _antiSpam.end(); ++itr)
    {
        std::size_t found = str.find(*itr);
        if (found != std::string::npos)
            return true;
    }

    return false;
}

void Logon::SetFakeMsg(const std::string& motd)
{
    _fakemsg = motd;
}

const char* Logon::GetFakeMsg() const
{
    return _fakemsg.c_str();
}

void Logon::Whisper(const std::string& text, uint32 language, Player* player)
{
    if (!player)
        return;
    WorldPacket data(SMSG_MESSAGECHAT, 200);
    data << uint8(CHAT_MSG_WHISPER_INFORM);
    data << uint32(language);
    data << uint64(657);
    data << uint32(language);                               //language 2.1.0 ?
    data << uint64(657);
    data << uint32(text.length() + 1);
    data << text;
    data << uint8(0);
    player->GetSession()->SendPacket(&data);
}


void Logon::LoadGlobalPlayerDataStore()
{
    uint32 oldMSTime = getMSTime();

    _globalPlayerDataStore.clear();
    QueryResult result = CharacterDatabase.Query("SELECT guid, account, name, gender, race, class, level FROM characters WHERE deleteDate IS NULL");
    if (!result)
    {
        sLog->outString();
        sLog->outErrorDb(">>  Loaded 0 Players data!");
        return;
    }

    uint32 count = 0;

    // query to load number of mails by receiver
    std::map<uint32, uint16> _mailCountMap;
    QueryResult mailCountResult = CharacterDatabase.Query("SELECT receiver, COUNT(receiver) FROM mail GROUP BY receiver");
    if (mailCountResult)
    {
        do
        {
            Field* fields = mailCountResult->Fetch();
            _mailCountMap[fields[0].GetUInt32()] = uint16(fields[1].GetUInt64());
        }
        while (mailCountResult->NextRow());
    }

    do
    {
        Field* fields = result->Fetch();
        uint32 guidLow = fields[0].GetUInt32();

        // count mails
        uint16 mailCount = 0;
        std::map<uint32, uint16>::const_iterator itr = _mailCountMap.find(guidLow);
        if (itr != _mailCountMap.end())
            mailCount = itr->second;

        AddGlobalPlayerData(
            guidLow,               /*guid*/
            fields[1].GetUInt32(), /*accountId*/
            fields[2].GetString(), /*name*/
            fields[3].GetUInt8(),  /*gender*/
            fields[4].GetUInt8(),  /*race*/
            fields[5].GetUInt8(),  /*class*/
            fields[6].GetUInt8(),  /*level*/
            mailCount,             /*mail count*/
            0                      /*guild id*/);

        ++count;
    }
    while (result->NextRow());

    sLog->outString(">> Loaded %d Players data in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    sLog->outString();
}

void Logon::AddGlobalPlayerData(uint32 guid, uint32 accountId, std::string const& name, uint8 gender, uint8 race, uint8 playerClass, uint8 level, uint16 mailCount, uint32 guildId)
{
    GlobalPlayerData data;

    data.guidLow = guid;
    data.accountId = accountId;
    data.name = name;
    data.level = level;
    data.race = race;
    data.playerClass = playerClass;
    data.gender = gender;
    data.mailCount = mailCount;
    data.guildId = guildId;
    data.groupId = 0;
    data.arenaTeamId[0] = 0;
    data.arenaTeamId[1] = 0;
    data.arenaTeamId[2] = 0;

    _globalPlayerDataStore[guid] = data;
    _globalPlayerNameStore[name] = guid;
}

void Logon::UpdateGlobalPlayerData(uint32 guid, uint8 mask, std::string const& name, uint8 level, uint8 gender, uint8 race, uint8 playerClass)
{
    GlobalPlayerDataMap::iterator itr = _globalPlayerDataStore.find(guid);
    if (itr == _globalPlayerDataStore.end())
        return;

    if (mask & PLAYER_UPDATE_DATA_LEVEL)
        itr->second.level = level;
    if (mask & PLAYER_UPDATE_DATA_RACE)
        itr->second.race = race;
    if (mask & PLAYER_UPDATE_DATA_CLASS)
        itr->second.playerClass = playerClass;
    if (mask & PLAYER_UPDATE_DATA_GENDER)
        itr->second.gender = gender;
    if (mask & PLAYER_UPDATE_DATA_NAME)
        itr->second.name = name;

    WorldPacket data(SMSG_INVALIDATE_PLAYER, 8);
    data << MAKE_NEW_GUID(guid, 0, HIGHGUID_PLAYER);
    SendGlobalMessage(&data);
}

void Logon::UpdateGlobalPlayerMails(uint32 guid, int16 count, bool add)
{
    GlobalPlayerDataMap::iterator itr = _globalPlayerDataStore.find(guid);
    if (itr == _globalPlayerDataStore.end())
        return;

    if (!add)
    {
        itr->second.mailCount = count;
        return;
    }

    int16 icount = (int16)itr->second.mailCount;
    if (count < 0 && abs(count) > icount)
        count = -icount;
    itr->second.mailCount = uint16(icount + count); // addition or subtraction
}

void Logon::UpdateGlobalPlayerGuild(uint32 guid, uint32 guildId)
{
    GlobalPlayerDataMap::iterator itr = _globalPlayerDataStore.find(guid);
    if (itr == _globalPlayerDataStore.end())
        return;

    itr->second.guildId = guildId;
}
void Logon::UpdateGlobalPlayerGroup(uint32 guid, uint32 groupId)
{
    GlobalPlayerDataMap::iterator itr = _globalPlayerDataStore.find(guid);
    if (itr == _globalPlayerDataStore.end())
        return;

    itr->second.groupId = groupId;
}

void Logon::UpdateGlobalPlayerArenaTeam(uint32 guid, uint8 slot, uint32 arenaTeamId)
{
    GlobalPlayerDataMap::iterator itr = _globalPlayerDataStore.find(guid);
    if (itr == _globalPlayerDataStore.end())
        return;

    itr->second.arenaTeamId[slot] = arenaTeamId;
}

void Logon::UpdateGlobalNameData(uint32 guidLow, std::string const& oldName, std::string const& newName)
{
    _globalPlayerNameStore.erase(oldName);
    _globalPlayerNameStore[newName] = guidLow;
}

void Logon::DeleteGlobalPlayerData(uint32 guid, std::string const& name)
{
    if (guid)
        _globalPlayerDataStore.erase(guid);
    if (!name.empty())
        _globalPlayerNameStore.erase(name);
}

GlobalPlayerData const* Logon::GetGlobalPlayerData(uint32 guid) const
{
    GlobalPlayerDataMap::const_iterator itr = _globalPlayerDataStore.find(guid);
    if (itr != _globalPlayerDataStore.end())
        return &itr->second;
    return NULL;
}

uint32 Logon::GetGlobalPlayerGUID(std::string const& name) const
{
    GlobalPlayerNameMap::const_iterator itr = _globalPlayerNameStore.find(name);
    if (itr != _globalPlayerNameStore.end())
        return itr->second;
    return 0;
}

/// Send a packet to all players (except self if mentioned)
void Logon::SendGlobalMessage(WorldPacket* packet, ClientSession* self, TeamId teamId)
{
    SessionMap::const_iterator itr;
    for (itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        if (itr->second &&
            itr->second->GetPlayer() &&
            //itr->second->GetPlayer()->IsInWorld() &&
            itr->second != self &&
            (teamId == TEAM_NEUTRAL || itr->second->GetPlayer()->GetTeamId() == teamId))
        {
            itr->second->SendPacket(packet);
        }
    }
}

/// Send a packet to all GMs (except self if mentioned)
void Logon::SendGlobalGMMessage(WorldPacket* packet, ClientSession* self, TeamId teamId)
{
    SessionMap::iterator itr;
    for (itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        if (itr->second &&
            itr->second->GetPlayer() &&
            //itr->second->GetPlayer()->IsInWorld() &&
            itr->second != self &&
            !AccountMgr::IsPlayerAccount(itr->second->GetSecurity()) &&
            (teamId == TEAM_NEUTRAL || itr->second->GetPlayer()->GetTeamId() == teamId))
        {
            itr->second->SendPacket(packet);
        }
    }
}
