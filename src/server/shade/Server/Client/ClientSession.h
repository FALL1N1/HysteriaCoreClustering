#ifndef __CLIENTSESSION_H
#define __CLIENTSESSION_H

#include <atomic>

#include "Proxy.h"
#include "AddonMgr.h"
#include "DatabaseEnv.h"
#include "ByteBuffer.h" 
#include "Warden.h"
#include "SharedDefines.h"

//DEPS
//#include "Group.h"

class WorldPacket;
class ByteBuffer;
class ClientSocket;
class NodeSocket;
class Player;
class LoginQueryHolder;
class Warden;

enum AccountDataType
{
    GLOBAL_CONFIG_CACHE             = 0,                    // 0x01 g
    PER_CHARACTER_CONFIG_CACHE      = 1,                    // 0x02 p
    GLOBAL_BINDINGS_CACHE           = 2,                    // 0x04 g
    PER_CHARACTER_BINDINGS_CACHE    = 3,                    // 0x08 p
    GLOBAL_MACROS_CACHE             = 4,                    // 0x10 g
    PER_CHARACTER_MACROS_CACHE      = 5,                    // 0x20 p
    PER_CHARACTER_LAYOUT_CACHE      = 6,                    // 0x40 p
    PER_CHARACTER_CHAT_CACHE        = 7,                    // 0x80 p
};

#define NUM_ACCOUNT_DATA_TYPES        8

#define GLOBAL_CACHE_MASK           0x15
#define PER_CHARACTER_CACHE_MASK    0xEA

struct AccountData
{
    AccountData() : Time(0), Data("") {}

    time_t Time;
    std::string Data;
};

enum PartyOperation
{
    PARTY_OP_INVITE = 0,
    PARTY_OP_UNINVITE = 1,
    PARTY_OP_LEAVE = 2,
    PARTY_OP_SWAP = 4
};

enum SessionFlags
{
    FLAG_ACCOUNT_RECONNECT      = 0x00000001,
    FLAG_ACCOUNT_WAIT_FOR_OK    = 0x00000002,
    FLAG_LOGOUT_REQUEST         = 0x00000004,
    FLAG_LOGOUT                 = 0x00000008,
    FLAG_TRANSFER_PENDING       = 0x00000010,
    FLAG_FORCE_TELEPORT         = 0x00000040,
    FLAG_INFORMED_SERVER_DOWN   = 0x00000080,
    FLAG_ACCOUNT_KICKED         = 0x00000100,
}; 

enum CharterTypes
{
    GUILD_CHARTER_TYPE = 9,
    ARENA_TEAM_CHARTER_2v2_TYPE = 2,
    ARENA_TEAM_CHARTER_3v3_TYPE = 3,
    ARENA_TEAM_CHARTER_5v5_TYPE = 5
};

//Temp use
struct ChannelT
{
    uint32 Channel_ID;
    std::string ChannelName;
    std::string ChannelPass;
};
typedef std::unordered_map<uint32, ChannelT> ChannelList;

class ClientSession
{
public:
    ClientSession(uint32 id, ClientSocket *sock, AccountTypes sec, uint8 expansion, time_t mute_time, LocaleConstant locale, uint32 recruiter, bool isRecruiter);
    ~ClientSession();

    ACE_Event_Handler::Reference_Count add_reference();
    ACE_Event_Handler::Reference_Count remove_reference();

    /*********************************************************/
    /***                  ACCOUNT SETTINGS                 ***/
    /*********************************************************/
    void LoadGlobalAccountData();
    void LoadAccountData(PreparedQueryResult result, uint32 mask);
    void SendAccountDataTimes(uint32 mask);
    void SetAccountData(AccountDataType type, time_t tm, std::string data);
    uint8 Expansion() const { return _expansion; }
    AccountData* GetAccountData(AccountDataType type) { return &_accountData[type]; }
    uint32 GetAccountId() const { return _accountId; }
    std::string const& GetRemoteAddress() { return _clientaddress; }
    AccountTypes GetSecurity() const { return _security; }
    bool IsAuthed() { return _authed; }
    void SetAuthed(bool sw) { _authed = sw; }
    void SetInQueue(bool state) { _inQueue = state; }
    time_t                                                  _muteTime;

    void SetBanned(bool yes)
    {
        KickPlayer();
        _banned = true;
    }

    /*********************************************************/
    /***                     ADDON                         ***/
    /*********************************************************/
    void ReadAddonsInfo(WorldPacket &data);
    void SendAddonsInfo();

    /*********************************************************/
    /***                  I/O HANDLING                     ***/
    /*********************************************************/

    enum PacketProcedure
    {
        PACKET_SEND_CLIENT = 0x1,
        PACKET_SEND_NODE = 0x2,
        PACKET_PROCESS = 0x4
    };

    void HandlePacket(WorldPacket *packet, uint16 opcode, uint8 procedure);

    void UpdateSingleNodeIO(uint32 diff);
    void UpdateSingleClientIO(uint32 diff);

    bool UpdateSingle(uint32 diff);

    void UpdateMultiNodeIO(uint32 diff);
    void UpdateMultiClientIO(uint32 diff);

    bool UpdateMulti(uint32 diff);

    void CloseSocket();
    void CloseSubSocket();

    /*********************************************************/
    /***                 SEND HANDLING                     ***/
    /*********************************************************/
    void SendPacket(WorldPacket const* packet);
    void SendPacketToNode(WorldPacket const* packet);

    void SendAuthWaitQue(uint32 position);
    void SendCommandStringToNode(const char *args, ...) ATTR_PRINTF(2, 3);
    void SendTutorialsData();
    void SendAuthResponse(uint8 code, bool shortForm, uint32 queuePos = 0);
    void SendClientCacheVersion(uint32 version);
    void SendPlayerNotFoundNotice(std::string name);
    void SendWrongFactionNotice();
    void SendNotification(const char *format, ...) ATTR_PRINTF(2, 3);
    void SendNotification(uint32 string_id, ...);
    void SendPartyResult(PartyOperation operation, const std::string& member, PartyResult res, uint32 val = 0);
    void SendPlayerLogin();

    /*********************************************************/
    /***                  PLAYER STUFF                     ***/
    /*********************************************************/
    void KickPlayer();
    void PlayerLogout();
    bool PlayerLoggingOut() const { return m_playerLogout; }
    void SetPlayer(Player *plr);
    Player* GetPlayer() const { return _player; }
    uint64 GetGuid() { return _GUID; }

    /*********************************************************/
    /***                      FLAGS                        ***/
    /*********************************************************/
    void SetFlag(uint32 newFlag);
    void RemoveFlag(uint32 oldFlag);

    void ToggleFlag(uint32 flag)
    {
        if (HasFlag(flag))
            RemoveFlag(flag);
        else
            SetFlag(flag);
    }

    bool HasFlag(uint32 flag) const { return (_sessionflags & flag) != 0; }

    /*********************************************************/
    /***                    LOCALES                        ***/
    /*********************************************************/
    LocaleConstant GetSessionDbcLocale() const { return _sessionDbcLocale; }
    LocaleConstant GetSessionDbLocaleIndex() const { return _sessionDbLocaleIndex; }
    const char *GetTrinityString(int32 entry) const;

    /*********************************************************/
    /***                   TUTORIALS                       ***/
    /*********************************************************/
    void LoadTutorialsData();
    uint32 GetTutorialInt(uint32 intId)
    {
        return _Tutorials[intId];
    }

    void SetTutorialInt(uint32 intId, uint32 value)
    {
        if (_Tutorials[intId] != value)
        {
            _Tutorials[intId] = value;
            _TutorialsChanged = true;
        }
    }

    void SaveTutorialsData(SQLTransaction& trans);

    //Warden
    void InitWarden(BigNumber* K, std::string const& os);
    void SendServerMessageToPlayer(Player* player, const char* msg);
    void SetStunned(bool apply);


    //The OpcodeHandling
public:
    void Handle_NULL(WorldPacket& recvPacket);
    void Handle_ServerSide(WorldPacket& recvPacket);
    void HandleCharEnumOpcode(WorldPacket& recvPacket); //Dummy
    void HandleCharEnum(PreparedQueryResult result);
    void HandlePlayerLoginOpcode(WorldPacket& recvPacket);
    void HandlePlayerLogin(LoginQueryHolder * holder);
    void HandlePlayerLogoutRequest(WorldPacket& recv_data);
    void HandleLogoutCancelOpcode(WorldPacket& recv_data);
    void HandleWardenDataOpcode(WorldPacket& recv_data);
    void HandleWorldStateUITimerUpdate(WorldPacket& /*recv_data*/);

    //QueryCache
    void HandleGameObjectQueryOpcode(WorldPacket & recv_data);
    void HandleCreatureQueryOpcode(WorldPacket & recv_data);
    void HandleItemQuerySingleOpcode(WorldPacket & recv_data);
    void HandleQuestQueryOpcode(WorldPacket & recv_data);

    //Temp use
    void SMSG_LOGIN_VERIFY_WORLD(WorldPacket& recvPacket);

    //Groups
    void HandleGroupInviteOpcode(WorldPacket& recvPacket); 
    void HandleGroupAcceptOpcode(WorldPacket& recvPacket);
    void HandleGroupDeclineOpcode(WorldPacket& recvPacket);
    void HandleGroupUninviteOpcode(WorldPacket& recvPacket);
    void HandleGroupUninviteGuidOpcode(WorldPacket& recvPacket);
    void HandleGroupSetLeaderOpcode(WorldPacket& recvPacket);
    void HandleGroupDisbandOpcode(WorldPacket& recvPacket);
    void HandleOptOutOfLootOpcode(WorldPacket& recvData);
    void HandleLootMethodOpcode(WorldPacket& recvPacket);
    void HandleLootRoll(WorldPacket& recvData);
    void HandleRequestPartyMemberStatsOpcode(WorldPacket& recvData);
    void HandleRaidTargetUpdateOpcode(WorldPacket& recvData);
    void HandleRaidReadyCheckOpcode(WorldPacket& recvData);
    void HandleRaidReadyCheckFinishedOpcode(WorldPacket& recvData);
    void HandleGroupRaidConvertOpcode(WorldPacket& recvData);
    void HandleGroupChangeSubGroupOpcode(WorldPacket& recvData);
    void HandleGroupSwapSubGroupOpcode(WorldPacket& recv_data);
    void HandleGroupAssistantLeaderOpcode(WorldPacket& recvData);
    void HandlePartyAssignmentOpcode(WorldPacket& recvData); 
    void HandleRequestRaidInfoOpcode(WorldPacket& recvData);
    void BuildPartyMemberStatsChangedPacket(Player* player, WorldPacket* data);

    //Channels
    void HandleJoinChannel(WorldPacket& recvPacket);
    void HandleLeaveChannel(WorldPacket& recvPacket);
    void HandleChannelList(WorldPacket& recvPacket);
    void HandleChannelPassword(WorldPacket& recvPacket);
    void HandleChannelSetOwner(WorldPacket& recvPacket);
    void HandleChannelOwner(WorldPacket& recvPacket);
    void HandleChannelModerator(WorldPacket& recvPacket);
    void HandleChannelUnmoderator(WorldPacket& recvPacket);
    void HandleChannelMute(WorldPacket& recvPacket);
    void HandleChannelUnmute(WorldPacket& recvPacket);
    void HandleChannelInvite(WorldPacket& recvPacket);
    void HandleChannelKick(WorldPacket& recvPacket);
    void HandleChannelBan(WorldPacket& recvPacket);
    void HandleChannelUnban(WorldPacket& recvPacket);
    void HandleChannelAnnouncements(WorldPacket& recvPacket);
    void HandleChannelDisplayListQuery(WorldPacket& recvPacket);
    void HandleGetChannelMemberCount(WorldPacket& recvPacket);
    void HandleSetChannelWatch(WorldPacket& recvPacket);

    // GM Tickets
    void HandleGMTicketCreateOpcode(WorldPacket& recvPacket);
    void HandleGMTicketUpdateOpcode(WorldPacket& recvPacket);
    void HandleGMTicketDeleteOpcode(WorldPacket& recvPacket);
    void HandleGMTicketGetTicketOpcode(WorldPacket& recvPacket);
    void HandleGMTicketSystemStatusOpcode(WorldPacket& recvPacket);
    void HandleGMSurveySubmit(WorldPacket& recvPacket);
    void HandleReportLag(WorldPacket& recvPacket);
    void HandleGMResponseResolve(WorldPacket& recvPacket);

    // Battlegrounds
    void HandleBattlemasterJoinOpcode(WorldPacket& recvPacket);
    void HandleBattleFieldPortOpcode(WorldPacket& recvData);
    void HandleBattlefieldListOpcode(WorldPacket& recvData);
    
    
    void HandleGuildQueryOpcode(WorldPacket& recvPacket);
    void HandleGuildCreateOpcode(WorldPacket& recvPacket);
    void HandleGuildInviteOpcode(WorldPacket& recvPacket);
    void HandleGuildRemoveOpcode(WorldPacket& recvPacket);
    void HandleGuildAcceptOpcode(WorldPacket& recvPacket);
    void HandleGuildDeclineOpcode(WorldPacket& recvPacket);
    void HandleGuildInfoOpcode(WorldPacket& recvPacket);
    void HandleGuildEventLogQueryOpcode(WorldPacket& recvPacket);
    void HandleGuildRosterOpcode(WorldPacket& recvPacket);
    void HandleGuildPromoteOpcode(WorldPacket& recvPacket);
    void HandleGuildDemoteOpcode(WorldPacket& recvPacket);
    void HandleGuildLeaveOpcode(WorldPacket& recvPacket);
    void HandleGuildDisbandOpcode(WorldPacket& recvPacket);
    void HandleGuildLeaderOpcode(WorldPacket& recvPacket);
    void HandleGuildMOTDOpcode(WorldPacket& recvPacket);
    void HandleGuildSetPublicNoteOpcode(WorldPacket& recvPacket);
    void HandleGuildSetOfficerNoteOpcode(WorldPacket& recvPacket);
    void HandleGuildRankOpcode(WorldPacket& recvPacket);
    void HandleGuildAddRankOpcode(WorldPacket& recvPacket);
    void HandleGuildDelRankOpcode(WorldPacket& recvPacket);
    void HandleGuildChangeInfoTextOpcode(WorldPacket& recvPacket);
    void HandleSaveGuildEmblemOpcode(WorldPacket& recvPacket);

    /************************************************************\
    |******************** SocialHandler stuff *******************|
    \************************************************************/
    void HandleContactListOpcode(WorldPacket& recvPacket);
    void HandleAddFriendOpcode(WorldPacket& recvPacket);
    void HandleAddFriendOpcodeCallBack(PreparedQueryResult result, std::string friendNote);
    void HandleDelFriendOpcode(WorldPacket& recvPacket);
    void HandleAddIgnoreOpcode(WorldPacket& recvPacket);
    void HandleAddIgnoreOpcodeCallBack(PreparedQueryResult result);
    void HandleDelIgnoreOpcode(WorldPacket& recvPacket);
    void HandleSetContactNotesOpcode(WorldPacket& recvPacket);

    //ClientHandlings
public:
    void HandleQueryInspectAchievements(WorldPacket & recv_data);
    void HandleWhoisOpcode(WorldPacket& recv_data);
    void HandleSendMail(WorldPacket & recv_data);

    //Future
    bool checkMsgForMute(Player* sender, WorldPacket & recv_data);
    bool checkMsgForCommand(std::string msg, WorldPacket & recv_data);
    void HandleMessagechatOpcode(WorldPacket & recv_data);

    //NodeHandlings
public:
    void Handle_SMSG_LOGOUT_RESPONSE(WorldPacket& recvPacket);
    void Handle_SMSG_LOGOUT_COMPLETE(WorldPacket& recvPacket);
    void Handle_SMSG_AUTH_RESPONSE(WorldPacket& recvPacket);
    void Handle_SMSG_LEVELUP_INFO(WorldPacket& recvPacket);
    void Handle_SMSG_AUTH_CHALLENGE(WorldPacket& recvPacket);

    //Caching
    void HandleSMSG_GAMEOBJECT_QUERY(WorldPacket & recv_data);
    void HandleSMSG_CREATURE_QUERY(WorldPacket & recv_data);
    void HandleSMSG_ITEM_QUERY_SINGLE(WorldPacket & recv_data);

    void Handle_NODE_PLAYER_DATA(WorldPacket& recvPacket);
    void Handle_NODE_PLAYER_CHANGED_ZONE(WorldPacket& recvPacket);

    void Handle_NODE_TRANSFER_ACK(WorldPacket& recvPacket);

    void Handle_NODE_MISC_DATA(WorldPacket& recvPacket);

    //Characterthings
public:
    void ProcessQueryCallbacks();
    PreparedQueryResultFuture _charEnumCallback;
    QueryResultHolderFuture _charLoginCallback;
    QueryCallback<PreparedQueryResult, std::string> _addFriendCallback;
    PreparedQueryResultFuture _addIgnoreCallback;

    bool CharCanLogin(uint32 lowGUID) { return _allowedCharsToLogin.find(lowGUID) != _allowedCharsToLogin.end(); }
    std::set<uint32> _allowedCharsToLogin;
    void SendToNode(uint32 n, uint32 m);
    uint32 GetCurrentNode() { return _nodeId; }

private:

    ///- Sockets
    ClientSocket *_ClientSocket;
    NodeSocket   *_NodeSocket;

    uint32 _reconnect_timer;
    uint32 _callback_timer;

    ///- ACCOUNT SETTINGS
    std::atomic<uint32>     _sessionflags;
    uint32                  _accountId;
    std::string             _clientaddress;
    uint32                  _nodeId;
    bool                    _inQueue;
    bool                    _banned;
    bool                    _authed;
    uint8                   _expansion;
    AccountTypes            _security;
    AccountData             _accountData[NUM_ACCOUNT_DATA_TYPES];
    AddonsList              _addonsList;

    ///- TUTORIAL SETTINGS
    bool                    _TutorialsChanged;
    uint32                  _Tutorials[MAX_ACCOUNT_TUTORIAL_VALUES];

    ///- LOCALES SETTINGS
    LocaleConstant          _sessionDbcLocale;
    LocaleConstant          _sessionDbLocaleIndex;

    // Warden 
    Warden* m_Warden;

    //Temp
    ChannelList m_chlist;

    //Player
    uint64 _GUID;
    uint32 _GUIDLow;
    Player *_player;
    bool m_inQueue;                                     // session wait in auth.queue
    bool m_playerLoading;                               // code processed in LoginPlayer
    bool m_playerLogout;                                // code processed in LogoutPlayer
    bool m_playerSave;
    bool m_playerRecentlyLogout;
    uint32 m_loginDelay;
    uint32 m_DoSStrikes;

public:
    void IncAntiSpamCounter() { _antiSpam++; }
    uint8 GetAntiSpamCounter() { return _antiSpam; }

    void SendFakeChatNotification(uint8 type, std::string msg, uint32 lang);
    void SendFakeChannelNotification(std::string channel, std::string msg, uint32 lang);

private:
    uint8 _antiSpam;
    ACE_Event_Handler::Reference_Count _referenceCount;
};

#endif
