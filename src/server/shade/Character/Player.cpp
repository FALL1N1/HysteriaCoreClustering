#include "AchievementMgr.h"
#include "AccountMgr.h"
#include "ClientSession.h"
#include "ClientSessionMgr.h"
#include "Player.h"
#include "ObjectMgr.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Language.h"
#include "SocialMgr.h"  
#include "Channel.h"
#include "ChannelMgr.h"
#include "Chat.h"
#include "Guilds.h"
#include "GuildsMgr.h"
#include "Creatures.h"
#include "Items.h"
#include "LoginHolderEnum.h"
#include "PetitionMgr.h"

enum CharacterFlags
{
    CHARACTER_FLAG_NONE                 = 0x00000000,
    CHARACTER_FLAG_UNK1                 = 0x00000001,
    CHARACTER_FLAG_UNK2                 = 0x00000002,
    CHARACTER_LOCKED_FOR_TRANSFER       = 0x00000004,
    CHARACTER_FLAG_UNK4                 = 0x00000008,
    CHARACTER_FLAG_UNK5                 = 0x00000010,
    CHARACTER_FLAG_UNK6                 = 0x00000020,
    CHARACTER_FLAG_UNK7                 = 0x00000040,
    CHARACTER_FLAG_UNK8                 = 0x00000080,
    CHARACTER_FLAG_UNK9                 = 0x00000100,
    CHARACTER_FLAG_UNK10                = 0x00000200,
    CHARACTER_FLAG_HIDE_HELM            = 0x00000400,
    CHARACTER_FLAG_HIDE_CLOAK           = 0x00000800,
    CHARACTER_FLAG_UNK13                = 0x00001000,
    CHARACTER_FLAG_GHOST                = 0x00002000,
    CHARACTER_FLAG_RENAME               = 0x00004000,
    CHARACTER_FLAG_UNK16                = 0x00008000,
    CHARACTER_FLAG_UNK17                = 0x00010000,
    CHARACTER_FLAG_UNK18                = 0x00020000,
    CHARACTER_FLAG_UNK19                = 0x00040000,
    CHARACTER_FLAG_UNK20                = 0x00080000,
    CHARACTER_FLAG_UNK21                = 0x00100000,
    CHARACTER_FLAG_UNK22                = 0x00200000,
    CHARACTER_FLAG_UNK23                = 0x00400000,
    CHARACTER_FLAG_UNK24                = 0x00800000,
    CHARACTER_FLAG_LOCKED_BY_BILLING    = 0x01000000,
    CHARACTER_FLAG_DECLINED             = 0x02000000,
    CHARACTER_FLAG_UNK27                = 0x04000000,
    CHARACTER_FLAG_UNK28                = 0x08000000,
    CHARACTER_FLAG_UNK29                = 0x10000000,
    CHARACTER_FLAG_UNK30                = 0x20000000,
    CHARACTER_FLAG_UNK31                = 0x40000000,
    CHARACTER_FLAG_UNK32                = 0x80000000
};

enum CharacterCustomizeFlags
{
    CHAR_CUSTOMIZE_FLAG_NONE            = 0x00000000,
    CHAR_CUSTOMIZE_FLAG_CUSTOMIZE       = 0x00000001,       // name, gender, etc...
    CHAR_CUSTOMIZE_FLAG_FACTION         = 0x00010000,       // name, gender, faction, etc...
    CHAR_CUSTOMIZE_FLAG_RACE            = 0x00100000        // name, gender, race, etc...
};

Player::Player (ClientSession *session, uint64 GUID): 
m_session(session), m_PackGUID(sizeof(uint64)+1), m_achievementMgr(this), m_GUID(GUID)
{
    m_GUIDLow = 0;
    m_valuesCount = PLAYER_END;

    m_PackGUID.appendPackGUID(m_GUID);

    m_ExtraFlags = 0;
    m_social = NULL;
    _IsInBattleGround = false;
    //SetGroupInvite(NULL);
}

Player::~Player()
{}

void Player::Logout(uint32 accountId)
{
    ///- Broadcast a logout message to the player's friends
    sSocialMgr->SendFriendStatus(this, FRIEND_OFFLINE, GetGUIDLow(), true);
    sSocialMgr->RemovePlayerSocial(GetGUIDLow());
    /*if (GetGroup()) {
        sLog->outString("GROUP UPDATE ON LOGOUT FOR %s(%u)", GetName(), GetGUID());
        GetGroup()->SendUpdate();
        // GetGroup()->SendUpdateToPlayer(); // we don't need it here
        SetGroupUpdateFlag(GROUP_UPDATE_FULL);
    }*/
         
    ///- Leave all Channels before Logout
    CleanupChannels();
}

bool Player::LoadFromDB(uint32 guid, SQLQueryHolder *holder)
{
    PreparedQueryResult result = holder->GetPreparedResult(PLAYER_LOGIN_QUERY_LOADFROM);

    if (!result)
    {
        sLog->outError("Player (GUID: %u) not found in table `characters`, can't load. ", guid);
        return false;
    }

    Field* fields = result->Fetch();
    uint32 dbAccountId = fields[1].GetUInt32();

    // check if the character's account in the db and the logged in account match.
    // player should be able to load/delete character only with correct account!
    if (dbAccountId != GetSession()->GetAccountId())
    {
        sLog->outError("Player (GUID: %u) loading from wrong account (is: %u, should be: %u)", guid, GetSession()->GetAccountId(), dbAccountId);
        return false;
    }

    if (holder->GetPreparedResult(PLAYER_LOGIN_QUERY_LOADBANNED))
    {
        sLog->outError("Player (GUID: %u) is banned, can't load.", guid);
        return false;
    }
    m_name = fields[2].GetString();
    m_GUIDLow = guid;

    _race  = fields[3].GetUInt8();                         // race
    _class = fields[4].GetUInt8();                         // class

    _team = TeamForRace(_race);
    _level = fields[6].GetUInt8();
    m_flags = fields[11].GetUInt32();
    _mapId = fields[15].GetUInt16();

    
    // GM state
    uint32 extraflags = fields[31].GetUInt16();
    if (!AccountMgr::IsVIPorPlayer(GetSession()->GetSecurity()))
    {
        switch (sLogon->getIntConfig(CONFIG_GM_LOGIN_STATE))
        {
            default:
            case 0:                      break;             // disable
            case 1: SetGameMaster(true); break;             // enable
            case 2:                                         // save state*/
                if (extraflags & PLAYER_EXTRA_GM_ON)
                    SetGameMaster(true);
                break;
        }

        switch (sLogon->getIntConfig(CONFIG_GM_VISIBLE_STATE))
        {
            default:
            case 0: SetGMVisible(false); break;             // invisible
            case 1:                      break;             // visible
            case 2:                                         // save state
                if (extraflags & PLAYER_EXTRA_GM_INVISIBLE)
                    SetGMVisible(false);
                break;
        }

        switch (sLogon->getIntConfig(CONFIG_GM_CHAT))
        {
            default:
            case 0:                  break;                 // disable
            case 1: SetGMChat(true); break;                 // enable
            case 2:                                         // save state
                if (extraflags & PLAYER_EXTRA_GM_CHAT)
                    SetGMChat(true);
                break;
        }

        switch (sLogon->getIntConfig(CONFIG_GM_WHISPERING_TO))
        {
            default:
            case 0:                          break;         // disable
            case 1: SetAcceptWhispers(true); break;         // enable
            case 2:                                         // save state
                if (extraflags & PLAYER_EXTRA_ACCEPT_WHISPERS)
                    SetAcceptWhispers(true);
                break;
        }
    }
    else
    {
        SetGameMaster(false);
        SetGMVisible(true);
        SetGMChat(false);
        SetAcceptWhispers(true);
    }

    // load achievements before anything else to prevent multiple gains for the same achievement/criteria on every loading (as loading does call UpdateAchievementCriteria)
    m_achievementMgr.LoadFromDB(holder->GetPreparedResult(PLAYER_LOGIN_QUERY_LOADACHIEVEMENTS), holder->GetPreparedResult(PLAYER_LOGIN_QUERY_LOADCRITERIAPROGRESS));
    // load the socials
    m_social = sSocialMgr->LoadFromDB(holder->GetPreparedResult(PLAYER_LOGIN_QUERY_LOADSOCIALLIST), GetGUIDLow());
    //_LoadGroup();
    return true;

}
/*
void Player::_LoadGroup()
{
    if (uint32 groupId = GetGroupIdFromStorage(GetGUIDLow()))
        if (Group* group = sGroupMgr->GetGroupByGUID(groupId))
            if (group->GetMemberGroup(GetGUID()) <= MAX_RAID_SUBGROUPS)
            {
                if (group->IsLeader(GetGUID()))
                    SetFlag(PLAYER_FLAGS_GROUP_LEADER);

                uint8 subgroup = group->GetMemberGroup(GetGUID());
                SetGroup(group, subgroup);

                // the group leader may change the instance difficulty while the player is offline
                SetDungeonDifficulty(group->GetDungeonDifficulty());
                SetRaidDifficulty(group->GetRaidDifficulty());
            }

    if (!GetGroup() || !GetGroup()->IsLeader(GetGUID()))
        RemoveFlag(PLAYER_FLAGS_GROUP_LEADER);
}

*/ 

void Player::SendInitialPackets()
{
    m_achievementMgr.SendAllAchievementData();
}

void Player::SendDirectMessage(WorldPacket *data)
{
    m_session->SendPacket(data);
}

void Player::UpdateAchievementCriteria(AchievementCriteriaTypes type, uint32 miscValue1 /*= 0*/, uint32 miscValue2 /*= 0*/, Unit* unit /*= NULL*/)
{
   // GetAchievementMgr().UpdateAchievementCriteria(type, miscValue1, miscValue2, unit);
}

void Player::CompletedAchievement(AchievementEntry const* entry)
{
    GetAchievementMgr().CompletedAchievement(entry);
}

void Player::ResetAchievementCriteria(AchievementCriteriaTypes type, uint32 miscvalue1, uint32 miscvalue2, bool evenIfCriteriaComplete)
{
    GetAchievementMgr().ResetAchievementCriteria(AchievementCriteriaTypes(type), miscvalue1, miscvalue2, evenIfCriteriaComplete);
}

void Player::ResetAchievement()
{
    GetAchievementMgr().Reset();
}

void Player::SetCriteriaProgress(AchievementCriteriaEntry const* entry, uint32 changeValue, uint8 ptype)
{
    GetAchievementMgr().SetCriteriaProgress(entry, changeValue, ptype);
}

//FlagControl
void Player::SetFlag(uint32 newFlag)
{
    uint32 oldval = m_flags;
    uint32 newval = oldval | newFlag;

    if (oldval != newval)
        m_flags = newval;
}

void Player::RemoveFlag(uint32 oldFlag)
{
    uint32 oldval = m_flags;
    uint32 newval = oldval & ~oldFlag;

    if (oldval != newval)
        m_flags = newval;
}

//Zone/Area-Related things
void ClientSession::Handle_NODE_PLAYER_CHANGED_ZONE(WorldPacket& recvPacket)
{
    uint32 zone, area, mapid;
    recvPacket >> zone;
    recvPacket >> area;
    recvPacket >> mapid;

    GetPlayer()->UpdateZone(zone,area, mapid);
}

void Player::UpdateZone(uint32 newZone, uint32 newArea, uint32 mapId)
{
    // we'll see if we would separate wintergrasp, let's not do it for now
    bool isWintergrasp = newZone == 4197 ? true : false;
    int32 WGNode = 12345;
/*
    // routing to the new map node (if any)
    // check if we changed zone
    if (_mapId != mapId)
    {
        int32 mapNode = -1; mapNode = sRoutingHelper->GetNodeForMap(mapId);
        if (mapNode >= 0)
        {
            sLog->outString("mapNode: %u != curnode: %u", mapNode, GetSession()->GetCurrentNode());
            if (mapNode != GetSession()->GetCurrentNode())
            {
                sLog->outString("[NodeSWAP] Player %s(%u) changed zone/map and moved to appropriate node.");
                sLog->outString("[NodeSWAP] Old node: %u   New node: %u", GetSession()->GetCurrentNode(), sRoutingHelper->GetNodeForMap(mapId));
                GetSession()->SendToNode(mapNode, mapId);
            }
        }
    } 
*/
    //This could be critical
    TRINITY_GUARD(ACE_Thread_Mutex, Lock);
    {
        _areaId = newArea;
        _zoneId = newZone;
        _mapId = mapId;
    }

    // group update
    /*if (Group* grp = GetGroup())
    {
        //sLog->outString("has group, %u : %s", GetGUID(), GetName());
        grp->SendUpdate();
        grp->SendUpdateToPlayer(GetGUID());
        SetGroupUpdateFlag(GROUP_UPDATE_FULL);
    } */

    sLog->outString("PLAYER MOVED TO A NEW ZONE/AREA: %u, %u, %u", newArea, newZone, mapId);



    if (Guild* guild = GetGuild())
        guild->UpdateMemberData(this, GUILD_MEMBER_DATA_ZONEID, newZone);
}

void ClientSession::Handle_SMSG_LEVELUP_INFO(WorldPacket& recvPacket)
{
    uint8 level;
    recvPacket >> level;
    _player->SetLevelUp(level);
    recvPacket.rfinish();

    if (Guild* guild = _player->GetGuild())
        guild->UpdateMemberData(_player, GUILD_MEMBER_DATA_LEVEL, level);
}

/************************************************************\
|***************** GAMEMASTER related stuff *****************|
\************************************************************/
void Player::SetGMVisible(bool on)
{
    if (on)
        m_ExtraFlags &= ~PLAYER_EXTRA_GM_INVISIBLE;         //remove flag
    else
    {
        m_ExtraFlags |= PLAYER_EXTRA_GM_INVISIBLE;          //add flag
        SetAcceptWhispers(false);
    }
}

void Player::SetGameMaster(bool on)
{
    if (on)
        m_ExtraFlags |= PLAYER_EXTRA_GM_ON;                 //add flag
    else
        m_ExtraFlags &= ~PLAYER_EXTRA_GM_ON;                //remove flag
}

/************************************************************\
|******************** TEAM related stuff ********************|
\************************************************************/
uint32 Player::TeamForRace(uint8 race)
{
    if (ChrRacesEntry const* rEntry = sChrRacesStore.LookupEntry(race))
    {
        switch (rEntry->TeamID)
        {
            case 1: return HORDE;
            case 7: return ALLIANCE;
        }
        sLog->outError("Race (%u) has wrong teamid (%u) in DBC: wrong DBC files?", uint32(race), rEntry->TeamID);
    }
    else
        sLog->outError("Race (%u) not found in DBC: wrong DBC files?", uint32(race));

    return ALLIANCE;
}

void Player::setFactionForRace(uint8 race)
{
    _team = TeamForRace(race);
}

/************************************************************\
|******************* WHISPER related stuff ******************|
\************************************************************/
void Player::Whisper(const std::string& text, uint32 language, uint64 receiver)
{
    if (language != LANG_ADDON)                             // if not addon data
        language = LANG_UNIVERSAL;                          // whispers should always be readable

    Player *rPlayer = sObjectMgr->GetPlayer(receiver);
    WorldPacket data;

    std::string _text(text);

    // when player you are whispering to is dnd, he cannot receive your message, unless you are in gm mode
    if (!rPlayer->isDND() || isGameMaster())
    {
        data.Initialize(SMSG_MESSAGECHAT, 200);
        BuildPlayerChat(&data, CHAT_MSG_WHISPER, _text, language);
        rPlayer->GetSession()->SendPacket(&data);

        // not send confirmation for addon messages
        if (language != LANG_ADDON)
        {
            data.Initialize(SMSG_MESSAGECHAT, 200);
            rPlayer->BuildPlayerChat(&data, CHAT_MSG_WHISPER_INFORM, _text, language);
            GetSession()->SendPacket(&data);
        }
    }
    else
        // announce to player that player he is whispering to is dnd and cannot receive his message
        ChatHandler(this).PSendSysMessage(LANG_PLAYER_DND, rPlayer->GetName(), rPlayer->dndMsg.c_str());

    if (!isAcceptWhispers() && !isGameMaster() && !rPlayer->isGameMaster())
    {
        SetAcceptWhispers(true);
        ChatHandler(this).SendSysMessage(LANG_COMMAND_WHISPERON);
    }

    // announce to player that player he is whispering to is afk
    if (rPlayer->isAFK())
        ChatHandler(this).PSendSysMessage(LANG_PLAYER_AFK, rPlayer->GetName(), rPlayer->afkMsg.c_str());

    // if player whisper someone, auto turn of dnd to be able to receive an answer
    if (isDND() && !rPlayer->isGameMaster())
        ToggleDND();
}

bool Player::IsInWhisperWhiteList(uint64 guid)
{
    for (WhisperListContainer::const_iterator itr = WhisperList.begin(); itr != WhisperList.end(); ++itr)
    {
        if (*itr == guid)
            return true;
    }
    return false;
}

/************************************************************\
|******************** MISC related stuff ********************|
\************************************************************/
bool Player::IsVisibleGloballyFor(Player* u) const
{
    if (!u)
        return false;

    // Always can see self
    if (u == this)
        return true;

    // Visible units, always are visible for all players
    //if (IsVisible())
    //    return true;

    // GMs are visible for higher gms (or players are visible for gms)
    if (!AccountMgr::IsVIPorPlayer(u->GetSession()->GetSecurity()))
        return GetSession()->GetSecurity() <= u->GetSession()->GetSecurity();

    // non faction visibility non-breakable for non-GMs
    //if (!IsVisible())
    //    return false;

    // non-gm stealth/invisibility not hide from global player lists
    return true;
}

bool Player::BuildEnumData(PreparedQueryResult result, WorldPacket* data)
{
    //             0               1                2                3                 4                  5                       6                        7
    //    "SELECT characters.guid, characters.name, characters.race, characters.class, characters.gender, characters.playerBytes, characters.playerBytes2, characters.level, "
    //     8                9               10                     11                     12                     13                    14
    //    "characters.zone, characters.map, characters.position_x, characters.position_y, characters.position_z, guild_member.guildid, characters.playerFlags, "
    //    15                    16                   17                     18                   19               20                     21
    //    "characters.at_login, character_pet.entry, character_pet.modelid, character_pet.level, characters.data, character_banned.guid, character_declinedname.genitive "

    Field* fields = result->Fetch();

    uint32 guid = fields[0].GetUInt32();
    uint8 plrRace = fields[2].GetUInt8();
    uint8 plrClass = fields[3].GetUInt8();
    uint8 gender = fields[4].GetUInt8();

    /*PlayerInfo const* info = sObjectMgr->GetPlayerInfo(plrRace, plrClass);
    if (!info)
    {
        sLog->outError("Player %u has incorrect race/class pair. Don't build enum.", guid);
        return false;
    }
    else if (!IsValidGender(gender))
    {
        sLog->outError("Player (%u) has incorrect gender (%hu), don't build enum.", guid, gender);
        return false;
    }*/

    *data << uint64(MAKE_NEW_GUID(guid, 0, HIGHGUID_PLAYER));
    *data << fields[1].GetString();                         // name
    *data << uint8(plrRace);                                // race
    *data << uint8(plrClass);                               // class
    *data << uint8(gender);                                 // gender

    uint32 playerBytes = fields[5].GetUInt32();
    *data << uint8(playerBytes);                            // skin
    *data << uint8(playerBytes >> 8);                       // face
    *data << uint8(playerBytes >> 16);                      // hair style
    *data << uint8(playerBytes >> 24);                      // hair color

    uint32 playerBytes2 = fields[6].GetUInt32();
    *data << uint8(playerBytes2 & 0xFF);                    // facial hair

    *data << uint8(fields[7].GetUInt8());                   // level
    *data << uint32(fields[8].GetUInt16());                 // zone
    *data << uint32(fields[9].GetUInt16());                 // map

    *data << fields[10].GetFloat();                         // x
    *data << fields[11].GetFloat();                         // y
    *data << fields[12].GetFloat();                         // z

    *data << uint32(fields[13].GetUInt32());                // guild id 

    uint32 charFlags = 0;
    uint32 playerFlags = fields[14].GetUInt32();
    uint16 atLoginFlags = fields[15].GetUInt16();
    if (playerFlags & PLAYER_FLAGS_HIDE_HELM)
        charFlags |= CHARACTER_FLAG_HIDE_HELM;
    if (playerFlags & PLAYER_FLAGS_HIDE_CLOAK)
        charFlags |= CHARACTER_FLAG_HIDE_CLOAK;
    if (playerFlags & PLAYER_FLAGS_GHOST)
        charFlags |= CHARACTER_FLAG_GHOST;
    if (atLoginFlags & AT_LOGIN_RENAME)
        charFlags |= CHARACTER_FLAG_RENAME;
    if (fields[20].GetUInt32())
        charFlags |= CHARACTER_FLAG_LOCKED_BY_BILLING;
    if (sLogon->getBoolConfig(CONFIG_DECLINED_NAMES_USED))
    {
        if (!fields[21].GetString().empty())
            charFlags |= CHARACTER_FLAG_DECLINED;
    }
    else
        charFlags |= CHARACTER_FLAG_DECLINED;

    *data << uint32(charFlags);                             // character flags

    // character customize flags
    if (atLoginFlags & AT_LOGIN_CUSTOMIZE)
        *data << uint32(CHAR_CUSTOMIZE_FLAG_CUSTOMIZE);
    else if (atLoginFlags & AT_LOGIN_CHANGE_FACTION)
        *data << uint32(CHAR_CUSTOMIZE_FLAG_FACTION);
    else if (atLoginFlags & AT_LOGIN_CHANGE_RACE)
        *data << uint32(CHAR_CUSTOMIZE_FLAG_RACE);
    else
        *data << uint32(CHAR_CUSTOMIZE_FLAG_NONE);

    // First login
    *data << uint8(atLoginFlags & AT_LOGIN_FIRST ? 1 : 0);

    // Pets info
    uint32 petDisplayId = 0;
    uint32 petLevel = 0;
    uint32 petFamily = 0;

    // show pet at selection character in character list only for non-ghost character
    if (result && !(playerFlags & PLAYER_FLAGS_GHOST) && (plrClass == CLASS_WARLOCK || plrClass == CLASS_HUNTER || plrClass == CLASS_DEATH_KNIGHT))
    {
        uint32 entry = fields[16].GetUInt32();
        CreatureTemplate const* creatureInfo = sCreatures->GetCreatureTemplate(entry);
        if (creatureInfo)
        {
            petDisplayId = fields[17].GetUInt32();
            petLevel = fields[18].GetUInt16();
            petFamily = creatureInfo->family;
        }
    }

    *data << uint32(petDisplayId);
    *data << uint32(petLevel);
    *data << uint32(petFamily);

    Tokenizer equipment(fields[19].GetString(), ' ');
    for (uint8 slot = 0; slot < INVENTORY_SLOT_BAG_END; ++slot)
    {
        uint32 visualBase = slot * 2;
        uint32 itemId = GetUInt32ValueFromArray(equipment, visualBase);
        ItemTemplate const* proto = sItems->GetItemTemplate(itemId);
        if (!proto)
        {
            *data << uint32(0);
            *data << uint8(0);
            *data << uint32(0);
            continue;
        }

        SpellItemEnchantmentEntry const* enchant = NULL;

        uint32 enchants = GetUInt32ValueFromArray(equipment, visualBase + 1);
        for (uint8 enchantSlot = PERM_ENCHANTMENT_SLOT; enchantSlot <= TEMP_ENCHANTMENT_SLOT; ++enchantSlot)
        {
            // values stored in 2 uint16
            uint32 enchantId = 0x0000FFFF & (enchants >> enchantSlot*16);
            if (!enchantId)
                continue;

            enchant = sSpellItemEnchantmentStore.LookupEntry(enchantId);
            if (enchant)
                break;
        }

        *data << uint32(proto->DisplayInfoID);
        *data << uint8(proto->InventoryType);
        *data << uint32(enchant ? enchant->aura_id : 0);
    }

    return true;
}

uint32 Player::GetUInt32ValueFromArray(Tokenizer const& data, uint16 index)
{
    if (index >= data.size())
        return 0;

    return (uint32)atoi(data[index]);
}

bool Player::InArena()
{
    switch (GetZoneId())
    {
        case 3702: // Blade's Edge Arena
        case 4378: // Dalaran Arena
        case 3968: // Ruins of Lordaeron
        case 3698: // Nagrand Arena
        case 4406: // The Ring of Valor
            return true;

        default:
            return false;
    }
}

/************************************************************\
|******************* CHANNEL related stuff ******************|
\************************************************************/
void Player::JoinedChannel(Channel* c)
{
    m_channels.push_back(c);
}

void Player::LeftChannel(Channel* c)
{
    m_channels.remove(c);
}

void Player::CleanupChannels()
{
    while (!m_channels.empty())
    {
        Channel* ch = *m_channels.begin();
        m_channels.erase(m_channels.begin());               // remove from player's channel list
        ch->Leave(GetGUID(), false);                        // not send to client, not remove from player's channel list
        if (ChannelMgr* cMgr = channelMgr(GetTeam()))
            cMgr->LeftChannel(ch->GetName());               // deleted channel if empty
    }
    sLog->outDebug(LOG_FILTER_CHATSYS, "Player: channels cleaned up!");
}

bool Player::IsPvPFlagged()
{
    return false;
    //return HasFlag(PLAYER_FLAGS_AFK) ? true : false;
}

bool Player::IsPvP()
{
    return false;
    //return HasFlag(PLAYER_FLAGS_AFK) ? true : false;
}

bool Player::IsFFAPvP()
{
    return false;
    //return HasFlag(PLAYER_FLAGS_AFK) ? true : false;
}

void Player::setDeathState(DeathState s, bool despawn)
{
    _deathState = s;
}


/************************************************************\
|******************** CHAT related stuff ********************|
\************************************************************/
bool Player::CanSpeak()
{
    return GetSession()->_muteTime <= time (NULL);
}

bool Player::ToggleAFK()
{
    ToggleFlag(PLAYER_FLAGS_AFK);

    bool state = HasFlag(PLAYER_FLAGS_AFK);

    // afk player not allowed in battleground
    /*if (state && InBattleground() && !InArena())
        LeaveBattleground();*/

    return state;
}

bool Player::ToggleDND()
{
    ToggleFlag(PLAYER_FLAGS_DND);

    return HasFlag(PLAYER_FLAGS_DND);
}

uint8 Player::GetChatTag() const
{
    uint8 tag = CHAT_TAG_NONE;

    if (isGMChat())
        tag |= CHAT_TAG_GM;
    if (isDND())
        tag |= CHAT_TAG_DND;
    if (isAFK())
        tag |= CHAT_TAG_AFK;
    if (HasFlag(PLAYER_FLAGS_DEVELOPER))
        tag |= CHAT_TAG_DEV;

    return tag;
}

void Player::BuildPlayerChat(WorldPacket *data, uint8 msgtype, const std::string& text, uint32 language) const
{
    *data << uint8(msgtype);
    *data << uint32(language);
    *data << uint64(GetGUID());
    *data << uint32(language);                               //language 2.1.0 ?
    *data << uint64(GetGUID());
    *data << uint32(text.length() + 1);
    *data << text;
    *data << uint8(GetChatTag());
}

bool Player::CanJoinToBattleground() const
{
    // @nodemgr - hook to get player auras
    // check Deserter debuff
    //if (HasAura(26013))
        //return false;

    return true;
}

Guild* Player::GetGuild() const
{
    uint32 guildId = GetGuildId();
    return guildId ? sGuildMgr->GetGuildById(guildId) : NULL;
}

uint32 Player::GetGuildIdFromStorage(uint32 guid)
{
    if (GlobalPlayerData const* playerData = sLogon->GetGlobalPlayerData(guid))
        return playerData->guildId;
    return 0;
}

void Player::RemovePetitionsAndSigns(uint64 guid, uint32 type)
{
    SignatureContainer* signatureStore = sPetitionMgr->GetSignatureStore();
    uint32 playerGuid = GUID_LOPART(guid);

    for (SignatureContainer::iterator itr = signatureStore->begin(); itr != signatureStore->end(); ++itr)
    {
        SignatureMap::iterator signItr = itr->second.signatureMap.find(playerGuid);
        if (signItr != itr->second.signatureMap.end())
        {
            Petition const* petition = sPetitionMgr->GetPetition(itr->first);
            if (!petition || (type != 10 && type != petition->petitionType))
                continue;

            // erase this
            itr->second.signatureMap.erase(signItr);

            uint64 ownerguid = MAKE_NEW_GUID(petition->ownerGuid, 0, HIGHGUID_PLAYER);
            uint64 petitionguid = MAKE_NEW_GUID(petition->petitionGuid, 0, HIGHGUID_ITEM);

            // send update if charter owner in game
            //Player* owner = ObjectAccessor::FindPlayerInOrOutOfWorld(ownerguid);
            //if (owner)
                //owner->GetSession()->SendPetitionQueryOpcode(petitionguid);
        }
    }

    if (type == 10)
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ALL_PETITION_SIGNATURES);
        stmt->setUInt32(0, playerGuid);
        CharacterDatabase.Execute(stmt);
    }
    else
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_PETITION_SIGNATURE);
        stmt->setUInt32(0, playerGuid);
        stmt->setUInt8(1, uint8(type));
        CharacterDatabase.Execute(stmt);
    }

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    if (type == 10)
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_PETITION_BY_OWNER);
        stmt->setUInt32(0, playerGuid);
        trans->Append(stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_PETITION_SIGNATURE_BY_OWNER);
        stmt->setUInt32(0, playerGuid);
        trans->Append(stmt);

        // xinef: clear petition store
        sPetitionMgr->RemovePetitionByOwnerAndType(playerGuid, 0);
    }
    else
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_PETITION_BY_OWNER_AND_TYPE);
        stmt->setUInt32(0, playerGuid);
        stmt->setUInt8(1, uint8(type));
        trans->Append(stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_PETITION_SIGNATURE_BY_OWNER_AND_TYPE);
        stmt->setUInt32(0, playerGuid);
        stmt->setUInt8(1, uint8(type));
        trans->Append(stmt);

        // xinef: clear petition store
        sPetitionMgr->RemovePetitionByOwnerAndType(playerGuid, uint8(type));
    }
    CharacterDatabase.CommitTransaction(trans);
}
