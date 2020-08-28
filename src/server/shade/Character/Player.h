#ifndef __PLAYER_H
#define __PLAYER_H

#include "AchievementMgr.h"
#include "ClientSession.h"
#include "ObjectDefines.h"
#include "UpdateFields.h"
//#include "GroupReference.h"
//#include "Group.h"
#include "../Guilds/Guilds.h"

class ClientSession;
class Channel;
class PlayerSocial;
//class Group;

enum DeathState
{
    ALIVE          = 0,
    JUST_DIED      = 1,
    CORPSE         = 2,
    DEAD           = 3,
    JUST_RESPAWNED = 4,
};

enum InventorySlots                                         // 4 slots
{
    INVENTORY_SLOT_BAG_START    = 19,
    INVENTORY_SLOT_BAG_END      = 23
};

enum InventoryPackSlots                                     // 16 slots
{
    INVENTORY_SLOT_ITEM_START   = 23,
    INVENTORY_SLOT_ITEM_END     = 39
};

enum PlayerFlags
{
    PLAYER_FLAGS_GROUP_LEADER      = 0x00000001,
    PLAYER_FLAGS_AFK               = 0x00000002,
    PLAYER_FLAGS_DND               = 0x00000004,
    PLAYER_FLAGS_GM                = 0x00000008,
    PLAYER_FLAGS_GHOST             = 0x00000010,
    PLAYER_FLAGS_RESTING           = 0x00000020,
    PLAYER_FLAGS_UNK6              = 0x00000040,
    PLAYER_FLAGS_UNK7              = 0x00000080,               // pre-3.0.3 PLAYER_FLAGS_FFA_PVP flag for FFA PVP state
    PLAYER_FLAGS_CONTESTED_PVP     = 0x00000100,               // Player has been involved in a PvP combat and will be attacked by contested guards
    PLAYER_FLAGS_IN_PVP            = 0x00000200,
    PLAYER_FLAGS_HIDE_HELM         = 0x00000400,
    PLAYER_FLAGS_HIDE_CLOAK        = 0x00000800,
    PLAYER_FLAGS_PLAYED_LONG_TIME  = 0x00001000,               // played long time
    PLAYER_FLAGS_PLAYED_TOO_LONG   = 0x00002000,               // played too long time
    PLAYER_FLAGS_IS_OUT_OF_BOUNDS  = 0x00004000,
    PLAYER_FLAGS_DEVELOPER         = 0x00008000,               // <Dev> prefix for something?
    PLAYER_FLAGS_UNK16             = 0x00010000,               // pre-3.0.3 PLAYER_FLAGS_SANCTUARY flag for player entered sanctuary
    PLAYER_FLAGS_TAXI_BENCHMARK    = 0x00020000,               // taxi benchmark mode (on/off) (2.0.1)
    PLAYER_FLAGS_PVP_TIMER         = 0x00040000,               // 3.0.2, pvp timer active (after you disable pvp manually)
    PLAYER_FLAGS_UNK19             = 0x00080000,
    PLAYER_FLAGS_UNK20             = 0x00100000,
    PLAYER_FLAGS_UNK21             = 0x00200000,
    PLAYER_FLAGS_COMMENTATOR2      = 0x00400000,
    PLAYER_ALLOW_ONLY_ABILITY      = 0x00800000,                // used by bladestorm and killing spree, allowed only spells with SPELL_ATTR0_REQ_AMMO, SPELL_EFFECT_ATTACK, checked only for active player
    PLAYER_FLAGS_UNK24             = 0x01000000,                // disabled all melee ability on tab include autoattack
    PLAYER_FLAGS_NO_XP_GAIN        = 0x02000000,
    PLAYER_FLAGS_UNK26             = 0x04000000,
    PLAYER_FLAGS_UNK27             = 0x08000000,
    PLAYER_FLAGS_UNK28             = 0x10000000,
    PLAYER_FLAGS_UNK29             = 0x20000000,
    PLAYER_FLAGS_UNK30             = 0x40000000,
    PLAYER_FLAGS_UNK31             = 0x80000000,
};

// 2^n values
enum AtLoginFlags
{
    AT_LOGIN_NONE              = 0x00,
    AT_LOGIN_RENAME            = 0x01,
    AT_LOGIN_RESET_SPELLS      = 0x02,
    AT_LOGIN_RESET_TALENTS     = 0x04,
    AT_LOGIN_CUSTOMIZE         = 0x08,
    AT_LOGIN_RESET_PET_TALENTS = 0x10,
    AT_LOGIN_FIRST             = 0x20,
    AT_LOGIN_CHANGE_FACTION    = 0x40,
    AT_LOGIN_CHANGE_RACE       = 0x80
};

// 2^n values
enum PlayerExtraFlags
{
    // gm abilities
    PLAYER_EXTRA_GM_ON              = 0x0001,
    PLAYER_EXTRA_ACCEPT_WHISPERS    = 0x0004,
    PLAYER_EXTRA_TAXICHEAT          = 0x0008,
    PLAYER_EXTRA_GM_INVISIBLE       = 0x0010,
    PLAYER_EXTRA_GM_CHAT            = 0x0020,               // Show GM badge in chat messages
    PLAYER_EXTRA_HAS_310_FLYER      = 0x0040,               // Marks if player already has 310% speed flying mount

    // other states
    PLAYER_EXTRA_PVP_DEATH          = 0x0100                // store PvP death status until corpse creating.
};

enum PlayerDelayedOperations
{
    DELAYED_SAVE_PLAYER         = 0x01,
    DELAYED_RESURRECT_PLAYER    = 0x02,
    DELAYED_SPELL_CAST_DESERTER = 0x04,
    DELAYED_BG_MOUNT_RESTORE    = 0x08,                     ///< Flag to restore mount state after teleport from BG
    DELAYED_BG_TAXI_RESTORE     = 0x10,                     ///< Flag to restore taxi state after teleport from BG
    DELAYED_END
};

// Player summoning auto-decline time (in secs)
#define MAX_PLAYER_SUMMON_DELAY                   (2*MINUTE)
#define MAX_MONEY_AMOUNT                       (0x7FFFFFFF-1)

enum PlayerChatTag
{
    CHAT_TAG_NONE       = 0x00,
    CHAT_TAG_AFK        = 0x01,
    CHAT_TAG_DND        = 0x02,
    CHAT_TAG_GM         = 0x04,
    CHAT_TAG_COM        = 0x08, // Commentator
    CHAT_TAG_DEV        = 0x10,
};

typedef std::list<uint64> WhisperListContainer;

struct PlayerInfo
{
                                                            // existence checked by displayId != 0
    PlayerInfo() : displayId_m(0),displayId_f(0)            //,levelInfo(NULL)
    {
    }
    uint32 health;
    uint32 power;
    Powers powertype;
    uint32 maxhealth;
    uint32 maxpower;
    DeathState _state;
     
    uint32 mapId;
    uint32 areaId;
    uint32 zoneId;
    float positionX;
    float positionY;
    float positionZ;
    float orientation;
    uint16 displayId_m;
    uint16 displayId_f;

};

// holder for Battleground data (pussywizard: not stored in db)
struct BGData
{
    BGData() : bgInstanceID(0), bgTypeID(BATTLEGROUND_TYPE_NONE), bgTeamId(TEAM_NEUTRAL), bgQueueSlot(PLAYER_MAX_BATTLEGROUND_QUEUES), isInvited(false), bgIsRandom(false), bgAfkReportedCount(0), bgAfkReportedTimer(0) {}

    uint32 bgInstanceID;
    BattlegroundTypeId bgTypeID;
    TeamId bgTeamId;
    uint32 bgQueueSlot;
    bool isInvited;
    bool bgIsRandom;

    std::set<uint32>   bgAfkReporter;
    uint8              bgAfkReportedCount;
    time_t             bgAfkReportedTimer;
};


class Player
{
    friend class ClientSession;
    public:
        explicit Player (ClientSession *session, uint64 GUID);
        ~Player ();

        // data from node
        void SetHealth(uint32 v) { health = v; };
        void SetMaxHealth(uint32 v) { maxhealth = v; };
        uint32 GetHealth() { return health; };
        uint32 GetMaxHealth() { return maxhealth; };

        void SetPower(uint32 v) { power = v; };
        void SetMaxPower(uint32 v) { maxpower = v; };
        void SetPowerType(Powers p) { powertype = p; };
        uint32 GetPower() { return power; };
        uint32 GetMaxPower() { return maxpower; };
        Powers GetPowerType() { return powertype; };

        void SetPosition(float x, float y, float z, float o) { positionX = x; positionY = y; positionZ = z; orientation = o; };
        float GetPositionX() { return positionX; };
        float GetPositionY() { return positionY; };
        float GetPositionZ() { return positionZ; };
        float GetOrientation() { return orientation; };
        void SetPositionX(float x) { positionX = x; };
        void SetPositionY(float y) { positionY = y; };
        void SetPositionZ(float z) { positionZ = z; };
        void SetOrientation(float o) { orientation = o; };
     
        void setDeathState(DeathState s, bool despawn = false);           // overwrited in Creature/Player/Pet
        bool IsAlive() const { return (_deathState == ALIVE); };
        bool isDying() const { return (_deathState == JUST_DIED); };
        bool isDead() const { return (_deathState == DEAD || _deathState == CORPSE); };
        DeathState getDeathState() { return _deathState; };


        //void SetInGuild(uint32 GuildId) { SetUInt32Value(PLAYER_GUILDID, GuildId);  sLogon->UpdateGlobalPlayerGuild(GetGUIDLow(), GuildId); }
        void SetInGuild(uint32 GuildId) 
        { 
            //SetUInt32Value(PLAYER_GUILDID, GuildId);  
            SetGuildId(GuildId);
            sLogon->UpdateGlobalPlayerGuild(GetGUIDLow(), GuildId); 
        }
        //void SetRank(uint8 rankId) { SetUInt32Value(PLAYER_GUILDRANK, rankId); }
        void SetRank(uint8 rankId) { m_GuildRankid = rankId; }
        //uint8 GetRank() const { return uint8(GetUInt32Value(PLAYER_GUILDRANK)); }
        void SetGuildIdInvited(uint32 GuildId) { m_GuildIdInvited = GuildId; }
        //uint32 GetGuildId() const { return GetUInt32Value(PLAYER_GUILDID);  }
        uint32 GetGuildId() const { return m_GuildId;  }
        void SetGuildId(uint32 v) { m_GuildId = v; }
        Guild* GetGuild() const;
        static uint32 GetGuildIdFromStorage(uint32 guid); 
        uint32 GetGuildIdInvited() { return m_GuildIdInvited; }
        static void RemovePetitionsAndSigns(uint64 guid, uint32 type);



        void Logout(uint32 accountId);

        PlayerInfo p_info;
        void UpdateMapID(uint32 mapID) { _mapId = mapID; }

        // bg
        BattlegroundQueueTypeId GetBattlegroundQueueTypeId(uint32 index) const { return m_bgBattlegroundQueueID[index]; }

        uint32 GetBattlegroundQueueIndex(BattlegroundQueueTypeId bgQueueTypeId) const
        {
            for (uint8 i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
                if (m_bgBattlegroundQueueID[i] == bgQueueTypeId)
                    return i;
            return PLAYER_MAX_BATTLEGROUND_QUEUES;
        }
    
        //Zone/Area
        void UpdateZone(uint32 newZone, uint32 newArea, uint32 mapId);

        void SetLevelUp(uint8 level){ _level = level; }
        uint32 GetMapId() { return _mapId; }

        //BG Stuff
        //bool InBattleground() { return _IsInBattleGround; }

        void RemoveBattlegroundQueueId(BattlegroundQueueTypeId val)
        {
            for (uint8 i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
                if (m_bgBattlegroundQueueID[i] == val)
                {
                    m_bgBattlegroundQueueID[i] = BATTLEGROUND_QUEUE_NONE;
                    return;
                }
        }
        bool HasFreeBattlegroundQueueId()
        {
            for (uint8 i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
                if (m_bgBattlegroundQueueID[i] == BATTLEGROUND_QUEUE_NONE)
                    return true;
            return false;
        }
        bool CanJoinToBattleground() const;

        bool InBattlegroundQueueForBattlegroundQueueType(BattlegroundQueueTypeId bgQueueTypeId) const
        {
            return GetBattlegroundQueueIndex(bgQueueTypeId) < PLAYER_MAX_BATTLEGROUND_QUEUES;
        }

        bool InBattlegroundQueue() const
        {
            for (uint8 i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
                if (m_bgBattlegroundQueueID[i] != BATTLEGROUND_QUEUE_NONE)
                    return true;
            return false;
        }

        uint32 AddBattlegroundQueueId(BattlegroundQueueTypeId val)
        {
            for (uint8 i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
                if (m_bgBattlegroundQueueID[i] == BATTLEGROUND_QUEUE_NONE || m_bgBattlegroundQueueID[i] == val)
                {
                    m_bgBattlegroundQueueID[i] = val;
                    return i;
                }
            return PLAYER_MAX_BATTLEGROUND_QUEUES;
        }

        // misc; to be moved
        bool isUsingLfg();
        bool inRandomLfgDungeon();
        Difficulty GetDifficulty(bool isRaid) const { return isRaid ? m_raidDifficulty : m_dungeonDifficulty; }
        Difficulty GetDungeonDifficulty() const { return m_dungeonDifficulty; }
        Difficulty GetRaidDifficulty() const { return m_raidDifficulty; }
        Difficulty GetStoredRaidDifficulty() const { return m_raidMapDifficulty; } // only for use in difficulty packet after exiting to raid map
        void SetDungeonDifficulty(Difficulty dungeon_difficulty) { m_dungeonDifficulty = dungeon_difficulty; }
        void SetRaidDifficulty(Difficulty raid_difficulty) { m_raidDifficulty = raid_difficulty; }
        //void StoreRaidMapDifficulty() { m_raidMapDifficulty = GetMap()->GetDifficulty(); }

        /************************************************************\
        |***************** GAMEMASTER related stuff *****************|
        \************************************************************/
        bool isGMChat() const { return m_ExtraFlags & PLAYER_EXTRA_GM_CHAT; }
        void SetGMChat(bool on) { if (on) m_ExtraFlags |= PLAYER_EXTRA_GM_CHAT; else m_ExtraFlags &= ~PLAYER_EXTRA_GM_CHAT; }
        bool isGMVisible() const { return !(m_ExtraFlags & PLAYER_EXTRA_GM_INVISIBLE); }
        void SetGMVisible(bool on);
        bool isGameMaster() { return m_ExtraFlags & PLAYER_EXTRA_GM_ON; }
        void SetGameMaster(bool on);


        /*********************************************************/
        /***                   GROUP SYSTEM                    ***/
        /*********************************************************/
        /*
        Group* GetGroupInvite() { return m_groupInvite; }
        void SetGroupInvite(Group* group) { m_groupInvite = group; }
        Group* GetGroup() { return m_group.getTarget(); }
        const Group* GetGroup() const { return (const Group*)m_group.getTarget(); }
        GroupReference& GetGroupRef() { return m_group; }
        void SetGroup(Group* group, int8 subgroup = -1);
        uint8 GetSubGroup() const { return m_group.getSubGroup(); }
        uint32 GetGroupUpdateFlag() const { return m_groupUpdateMask; }
        void SetGroupUpdateFlag(uint32 flag) { m_groupUpdateMask |= flag; }
        uint64 GetAuraUpdateMaskForRaid() const { return m_auraRaidUpdateMask; }
        void SetAuraUpdateMaskForRaid(uint8 slot) { m_auraRaidUpdateMask |= (uint64(1) << slot); }
        Player* GetNextRandomRaidMember(float radius);
        PartyResult CanUninviteFromGroup() const;
        
        bool IsGroupVisibleFor(Player const* p) const;
        bool IsInSameGroupWith(Player const* p) const;
        bool IsInSameRaidWith(Player const* p) const { return p == this || (GetGroup() != NULL && GetGroup() == p->GetGroup()); }
        void UninviteFromGroup();
        static void RemoveFromGroup(Group* group, uint64 guid, RemoveMethod method = GROUP_REMOVEMETHOD_DEFAULT, uint64 kicker = 0, const char* reason = NULL);
        void RemoveFromGroup(RemoveMethod method = GROUP_REMOVEMETHOD_DEFAULT) { RemoveFromGroup(GetGroup(), GetGUID(), method); }
        void SendUpdateToOutOfRangeGroupMembers();
        void SendRaidInfo();
        
        void _LoadGroup(); // used at login
        static uint32 GetGroupIdFromStorage(uint32 guid);
        */
        /************************************************************\
        |******************** TEAM related stuff ********************|
        \************************************************************/
        static uint32 TeamForRace(uint8 race);
        uint32 GetTeam() const { return _team; }
        TeamId GetTeamId() const { return _team == ALLIANCE ? TEAM_ALLIANCE : TEAM_HORDE; }
        void setFactionForRace(uint8 race);
        uint8 getRace() const { return _race; }
        uint8 getClass() const { return _class; }

        ClientSession* GetSession() const { return m_session; }

        /************************************************************\
        |****************** ACHIEVES related stuff ******************|
        \************************************************************/
        AchievementMgr& GetAchievementMgr() { return m_achievementMgr; }
        AchievementMgr const& GetAchievementMgr() const { return m_achievementMgr; }
        void UpdateAchievementCriteria(AchievementCriteriaTypes type, uint32 miscValue1 = 0, uint32 miscValue2 = 0, Unit* unit = NULL);
        void CompletedAchievement(AchievementEntry const* entry);
        void ResetAchievementCriteria(AchievementCriteriaTypes type, uint32 miscvalue1, uint32 miscvalue2, bool evenIfCriteriaComplete);
        void ResetAchievement();
        void SetCriteriaProgress(AchievementCriteriaEntry const* entry, uint32 changeValue, uint8 ptype);
        
        /************************************************************\
        |******************* WHISPER related stuff ******************|
        \************************************************************/
        bool isAcceptWhispers() const { return m_ExtraFlags & PLAYER_EXTRA_ACCEPT_WHISPERS; }
        void SetAcceptWhispers(bool on) { if (on) m_ExtraFlags |= PLAYER_EXTRA_ACCEPT_WHISPERS; else m_ExtraFlags &= ~PLAYER_EXTRA_ACCEPT_WHISPERS; }
        void Whisper(const std::string& text, const uint32 language, uint64 receiver);
        void ClearWhisperWhiteList() { WhisperList.clear(); }
        void AddWhisperWhiteList(uint64 guid) { WhisperList.push_back(guid); }
        bool IsInWhisperWhiteList(uint64 guid);

        /************************************************************\
        |******************** MISC related stuff ********************|
        \************************************************************/
        bool IsVisibleGloballyFor(Player* player) const;
        static bool BuildEnumData(PreparedQueryResult result, WorldPacket* data);
        static uint32 GetUInt32ValueFromArray(Tokenizer const& data, uint16 index);
        bool InArena();

        /************************************************************\
        |******************* LOADER related stuff *******************|
        \************************************************************/
        bool LoadFromDB(uint32 guid, SQLQueryHolder *holder);

        /************************************************************\
        |******************* CHANNEL related stuff ******************|
        \************************************************************/
        void JoinedChannel(Channel* c);
        void LeftChannel(Channel* c);
        void CleanupChannels();
        
        /************************************************************\
        |******************** CHAT related stuff ********************|
        \************************************************************/
        bool CanSpeak();
        bool ToggleAFK();
        bool ToggleDND();
        bool isAFK() const { return HasFlag(PLAYER_FLAGS_AFK); }
        bool isDND() const { return HasFlag(PLAYER_FLAGS_DND); }
        uint8 GetChatTag() const;
        void BuildPlayerChat(WorldPacket *data, uint8 msgtype, const std::string& text, uint32 language) const;
        std::string afkMsg;
        std::string dndMsg;

        uint32 GetGUIDLow() { return m_GUIDLow; }
        uint64 GetGUID() const { return m_GUID; }
        std::string GetPlayerName() { return m_name; }
        const char* GetName() const { return m_name.c_str(); }
        std::string GetPlayerLink() const { return m_session ? "|cffffffff|Hplayer:" + m_name + "|h[" + m_name + "]|h|r" : m_name; }
        
        void SendDirectMessage(WorldPacket *data);
        void SendInitialPackets();
  
        const ByteBuffer& GetPackGUID() const { return m_PackGUID; }

        //Flags
        void SetFlag(uint32 newFlag);
        void RemoveFlag(uint32 oldFlag);

        void ToggleFlag(uint32 flag)
        {
            if (HasFlag(flag))
                RemoveFlag(flag);
            else
                SetFlag(flag);
        }

        bool HasFlag(uint32 flag) const
        {
            return (m_flags & flag) != 0;
        }

        uint8 getLevel() { return _level; }
        uint32 GetZoneId() { return _zoneId; }
        uint32 getClass() { return _class; }  

        PlayerSocial *GetSocial() { return m_social; }

        // Battleground Group System
        /*
        void SetBattlegroundOrBattlefieldRaid(Group *group, int8 subgroup = -1);
        void RemoveFromBattlegroundOrBattlefieldRaid();
        Group* GetOriginalGroup() { return m_originalGroup.getTarget(); }
        GroupReference& GetOriginalGroupRef() { return m_originalGroup; }
        uint8 GetOriginalSubGroup() const { return m_originalGroup.getSubGroup(); }
        void SetOriginalGroup(Group* group, int8 subgroup = -1);*/


        bool IsPvPFlagged();
        bool IsPvP();
        bool IsFFAPvP();

        void SendToNode(uint32 n, uint32 m) { GetSession()->SendToNode(n, m); };
        uint32 GetGroupGUID() { return groupGUID; };
        uint32 SetGroupGUID(uint32 g) { groupGUID = g; };
    
    protected:
        // Gamemaster whisper whitelist
        WhisperListContainer WhisperList;
        BattlegroundQueueTypeId m_bgBattlegroundQueueID[PLAYER_MAX_BATTLEGROUND_QUEUES];
        BGData m_bgData;
        bool m_IsBGRandomWinner;

        Difficulty m_dungeonDifficulty;
        Difficulty m_raidDifficulty;
        Difficulty m_raidMapDifficulty;
    private:
        //Critical
        ACE_Thread_Mutex Lock;
        
        ByteBuffer m_PackGUID;
        ClientSession *m_session;
        uint32 m_GUIDLow;
        uint64 m_GUID;
        std::string m_name;

        uint8 _level;

        AchievementMgr m_achievementMgr; 

        uint16 m_valuesCount;
        union
        {
            int32  *m_int32Values;
            uint32 *m_uint32Values;
            float  *m_floatValues;
        };

        // received from nodes
        uint32 health;
        uint32 power;
        Powers powertype;
        uint32 maxhealth;
        uint32 maxpower;
        DeathState _deathState;

        //bool IsFFAPvP() const { return HasByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_FFA_PVP); }


        //Pos and Area
        uint32 _mapId;
        uint32 _areaId;
        uint32 _zoneId;
        float positionX;
        float positionY;
        float positionZ;
        float orientation;

        uint32 _team;
        uint8 _race;
        uint8 _class;

        //Channels
        typedef std::list<Channel*> JoinedChannelsList;
        JoinedChannelsList m_channels;
        uint32 m_flags;
        uint32 m_ExtraFlags;

        // Social
        PlayerSocial *m_social;
        
        // Groups
        /*
        GroupReference m_group;
        GroupReference m_originalGroup;
        Group* m_groupInvite;
        uint32 m_groupUpdateMask;
        uint64 m_auraRaidUpdateMask;
        bool m_bPassOnGroupLoot;*/
        uint32 groupGUID;

        //BattleGround
        bool _IsInBattleGround;

        // Guiilds, should be private
        uint32 m_GuildIdInvited;
        uint32 m_GuildId;
        uint32 m_GuildRankid;
};

#endif