#include "Common.h"
#include "Proxy.h"
#include "WorldPacket.h"
#include "ClientSession.h"
#include "DatabaseEnv.h"
#include "ObjectDefines.h"
#include "Player.h"
#include "SocialMgr.h"
#include "RoutingHelper.h"
#include "Opcodes.h"
#include "LoginHolderEnum.h"
#include "GuildsMgr.h"
#include "Guilds.h"

class LoginQueryHolder : public SQLQueryHolder
{
    private:
        uint32 m_accountId;
        uint64 m_guid;
    public:
        LoginQueryHolder(uint32 accountId, uint64 guid)
            : m_accountId(accountId), m_guid(guid) { }
        uint64 GetGuid() const { return m_guid; }
        uint32 GetAccountId() const { return m_accountId; }
        bool Initialize();
};

bool LoginQueryHolder::Initialize()
{
    SetSize(MAX_PLAYER_LOGIN_QUERY);

    bool res = true;
    uint32 lowGuid = GUID_LOPART(m_guid);
    PreparedStatement* stmt = NULL;

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADFROM, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GROUP_MEMBER);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADGROUP, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_INSTANCE);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADBOUNDINSTANCES, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_AURAS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADAURAS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SPELL);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADSPELLS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADQUESTSTATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_DAILYQUESTSTATUS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADDAILYQUESTSTATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_WEEKLYQUESTSTATUS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADWEEKLYQUESTSTATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_REPUTATION);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADREPUTATION, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_INVENTORY);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADINVENTORY, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_ACTIONS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADACTIONS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_MAILCOUNT);
    stmt->setUInt32(0, lowGuid);
    stmt->setUInt64(1, uint64(time(NULL)));
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADMAILCOUNT, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_MAILDATE);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADMAILDATE, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SOCIALLIST);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADSOCIALLIST, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_HOMEBIND);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADHOMEBIND, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SPELLCOOLDOWNS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADSPELLCOOLDOWNS, stmt);

    if (sLogon->getBoolConfig(CONFIG_DECLINED_NAMES_USED))
    {
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_DECLINEDNAMES);
        stmt->setUInt32(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADDECLINEDNAMES, stmt);
    }

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GUILD_MEMBER);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADGUILD, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_ARENAINFO);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADARENAINFO, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_ACHIEVEMENTS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADACHIEVEMENTS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_CRITERIAPROGRESS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADCRITERIAPROGRESS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_EQUIPMENTSETS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADEQUIPMENTSETS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_ENTRY_POINT);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADBGDATA, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_GLYPHS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADGLYPHS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_TALENTS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADTALENTS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PLAYER_ACCOUNT_DATA);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADACCOUNTDATA, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SKILLS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADSKILLS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_RANDOMBG);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADRANDOMBG, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_BANNED);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADBANNED, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUSREW);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADQUESTSTATUSREW, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ACCOUNT_INSTANCELOCKTIMES);
    stmt->setUInt32(0, m_accountId);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADINSTANCELOCKTIMES, stmt);

    return res;
}

void ClientSession::HandleCharEnumOpcode(WorldPacket & /*recv_data*/)
{
    // remove expired bans
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_EXPIRED_BANS);
    CharacterDatabase.Execute(stmt);

    /// get all the data necessary for loading all characters (along with their pets) on the account

    if (sLogon->getBoolConfig(CONFIG_DECLINED_NAMES_USED))
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ENUM_DECLINED_NAME);
    else
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ENUM);

    stmt->setUInt8(0, 0);
    stmt->setUInt32(1, GetAccountId());

    _charEnumCallback = CharacterDatabase.AsyncQuery(stmt);
}

void ClientSession::HandleCharEnum(PreparedQueryResult result)
{
    WorldPacket data(SMSG_CHAR_ENUM, 100);                  // we guess size

    uint8 num = 0;

    data << num;

    _allowedCharsToLogin.clear();
    if (result && result->GetRowCount() > 0)
    {
        do
        {
            uint32 guidlow = (*result)[0].GetUInt32();
            if (_allowedCharsToLogin.find(guidlow) != _allowedCharsToLogin.end()) // Fixes a glitch when a player had multiple pets in the same slot
                continue;

            sLog->outDetail("Loading char guid %u from account %u.", guidlow, GetAccountId());
            if (Player::BuildEnumData(result, &data))
            {
                _allowedCharsToLogin.insert(guidlow);
                ++num;
            }
        }
        while (result->NextRow());
    }

    data.put<uint8>(0, num);

    SendPacket(&data);
}

void ClientSession::HandlePlayerLoginOpcode(WorldPacket & recv_data)
{ 
    // send player to char screen if current node is unavailable
    if (!sRoutingHelper->CheckNodeID(_nodeId) || _nodeId == 0 && m_playerLoading)
    {
        WorldPacket data(SMSG_CHARACTER_LOGIN_FAILED, 1);
        data << (uint8)CHAR_LOGIN_DUPLICATE_CHARACTER;
        SendPacket(&data);
        SendAuthResponse(CHAR_LOGIN_NO_WORLD, false, 0);
        return;
    }

    SetPlayer(NULL);
    if (/*PlayerLoading() ||*/ GetPlayer() != NULL)
    {
        sLog->outError("Player tries to login again, AccountId = %d", GetAccountId());
        return;
    }

    m_playerLoading = true;
    uint64 playerGuid = 0;
    sLog->outStaticDebug("WORLD: Recvd Player Logon Message");
    recv_data >> playerGuid;
    if (!CharCanLogin(GUID_LOPART(playerGuid)))
    {
        sLog->outError("Account (%u) can't login with that character (%u).", GetAccountId(), GUID_LOPART(playerGuid));
        KickPlayer();
        return;
    }

    LoginQueryHolder *holder = new LoginQueryHolder(GetAccountId(), playerGuid);
    if (!holder->Initialize())
    {
        delete holder;                                      // delete all unprocessed queries
        m_playerLoading = false;
        return;
    }

    _charLoginCallback = CharacterDatabase.DelayQueryHolder((SQLQueryHolder*)holder);
}

void ClientSession::HandlePlayerLogin(LoginQueryHolder * holder)
{
    uint64 playerGuid = holder->GetGuid();
    bool playerExists = false;

    Player* pCurrChar = new Player(this, playerGuid);

    // "GetAccountId() == db stored account id" checked in LoadFromDB (prevent login not own character using cheating tools)
    if (!pCurrChar->LoadFromDB(GUID_LOPART(playerGuid), holder))
    {
        KickPlayer();                                       // disconnect client, player no set to session and it will not deleted or saved at kick
        delete pCurrChar;                                   // delete it manually
        delete holder;                                      // delete all unprocessed queries
        m_playerLoading = false;
        return;
    }
    SetPlayer(pCurrChar);
    _GUID = playerGuid;

    //Add the Char to our ObjectMgr
    sObjectMgr->Player_Add(pCurrChar);

    sLog->outString("Player %s is logging in world node.", pCurrChar->GetPlayerName().c_str());
    LoadAccountData(holder->GetPreparedResult(PLAYER_LOGIN_QUERY_LOADACCOUNTDATA), PER_CHARACTER_CACHE_MASK);
    SendAccountDataTimes(PER_CHARACTER_CACHE_MASK);
    
    pCurrChar->SendInitialPackets();
    
    // friend status
    sSocialMgr->SendFriendStatus(pCurrChar, FRIEND_ONLINE, pCurrChar->GetGUIDLow(), true);

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_ONLINE);
    stmt->setUInt32(0, pCurrChar->GetGUIDLow());
    CharacterDatabase.Execute(stmt);

    if (uint32 guildId = Player::GetGuildIdFromStorage(pCurrChar->GetGUIDLow()))
    {
        Guild* guild = sGuildMgr->GetGuildById(guildId);
        Guild::Member const* member = guild ? guild->GetMember(pCurrChar->GetGUID()) : NULL;
        if (member)
        {
            pCurrChar->SetInGuild(guildId);
            pCurrChar->SetRank(member->GetRankId());
            guild->SendLoginInfo(this);
            pCurrChar->SetGuildId(guildId);
        }
        else
        {
            sLog->outError("Player %s (GUID: %u) marked as member of not existing guild (id: %u), removing guild membership for player.", pCurrChar->GetName(), pCurrChar->GetGUIDLow(), guildId);
            pCurrChar->SetInGuild(0);
            pCurrChar->SetRank(0);
        }
    }
    else
    {
        pCurrChar->SetInGuild(0);
        pCurrChar->SetRank(0);
    }

    // Now we are save to connect to our node
    SendPlayerLogin();
    m_playerLoading = false;
    delete holder;
}

void ClientSession::HandlePlayerLogoutRequest(WorldPacket& /*recv_data*/)
{
    SetFlag(FLAG_LOGOUT_REQUEST);
}

void ClientSession::HandleLogoutCancelOpcode(WorldPacket& /*recv_data*/)
{
    RemoveFlag(FLAG_LOGOUT_REQUEST);
}

void ClientSession::Handle_SMSG_LOGOUT_RESPONSE(WorldPacket& recv_data)
{
    uint8 reason;
    uint32 time;
    recv_data >> reason;
    recv_data >> time;
    if (reason != 0)
        RemoveFlag(FLAG_LOGOUT_REQUEST);
}

void ClientSession::Handle_SMSG_LOGOUT_COMPLETE(WorldPacket& recvPacket)
{
    if (!HasFlag(FLAG_LOGOUT_REQUEST))
        return;

    PlayerLogout();

    //Wird im laufe der Entwicklung geändert...
    WorldPacket data(SMSG_LOGOUT_COMPLETE, 0);
    SendPacket(&data);

    if (_nodeId != sRoutingHelper->ConnectToMaster())
    {
        //Initalisiere alles neu
        CloseSubSocket();
        _sessionflags = 0;
        _nodeId = 0;

        //Zurück zum Master
        _nodeId = sRoutingHelper->ConnectToMaster();
        if (_nodeId == 0)
            _reconnect_timer = 2000000;
        else
            _reconnect_timer = 100000; //Set the reconnect Timer

        SetFlag(FLAG_ACCOUNT_RECONNECT); //Die Verbindung baut nun der Reconnect auf
    }
}
