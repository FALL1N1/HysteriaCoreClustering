#include "ClientSession.h"
#include "WorldPacket.h"
#include "SocialMgr.h"
#include "Player.h"
#include "ObjectMgr.h"
#include "AccountMgr.h"
#include "Language.h"

void ClientSession::HandleContactListOpcode(WorldPacket & recv_data)
{
    uint32 unk;
    recv_data >> unk;
    //sLog->outString("WORLD: Received CMSG_CONTACT_LIST - Unk: %d", unk);
    _player->GetSocial()->SendSocialList(_player);
}

void ClientSession::HandleAddFriendOpcode(WorldPacket & recv_data)
{
    //sLog->outString("WORLD: Received CMSG_ADD_FRIEND");

    std::string friendName = GetTrinityString(LANG_FRIEND_IGNORE_UNKNOWN);
    std::string friendNote;

    recv_data >> friendName;

    recv_data >> friendNote;

    if (!normalizePlayerName(friendName))
        return;

    //sLog->outString("WORLD: %s asked to add friend : '%s'", GetPlayer()->GetPlayerName().c_str(), friendName.c_str());

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GUID_RACE_ACC_BY_NAME);

    stmt->setString(0, friendName);

    _addFriendCallback.SetParam(friendNote);
    _addFriendCallback.SetFutureResult(CharacterDatabase.AsyncQuery(stmt));
}

void ClientSession::HandleAddFriendOpcodeCallBack(PreparedQueryResult result, std::string friendNote)
{
    if (!GetPlayer())
        return;

    uint64 friendGuid;
    uint32 friendAccountId;
    uint32 team;
    FriendsResult friendResult;

    friendResult = FRIEND_NOT_FOUND;
    friendGuid = 0;

    if (result)
    {
        Field* fields = result->Fetch();

        friendGuid = MAKE_NEW_GUID(fields[0].GetUInt32(), 0, HIGHGUID_PLAYER);
        team = Player::TeamForRace(fields[1].GetUInt8());
        friendAccountId = fields[2].GetUInt32();

        if (!AccountMgr::IsVIPorPlayer(GetSecurity()) || sLogon->getBoolConfig(CONFIG_ALLOW_GM_FRIEND) || AccountMgr::IsVIPorPlayer(AccountMgr::GetSecurity(friendAccountId, realmID)))
        {
            if (friendGuid)
            {
                if (friendGuid == GetPlayer()->GetGUID())
                    friendResult = FRIEND_SELF;
                else if (GetPlayer()->GetTeam() != team && !sLogon->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_ADD_FRIEND) && AccountMgr::IsVIPorPlayer(GetSecurity()) && !AccountMgr::IsVIPAccount(GetSecurity()))
                    friendResult = FRIEND_ENEMY;
                else if (GetPlayer()->GetSocial()->HasFriend(GUID_LOPART(friendGuid)))
                    friendResult = FRIEND_ALREADY;
                else
                {
                    Player* pFriend = sObjectMgr->FindPlayer(friendGuid);
                    if (pFriend /*&& pFriend->IsInWorld()*/ && pFriend->IsVisibleGloballyFor(GetPlayer()))
                        friendResult = FRIEND_ADDED_ONLINE;
                    else
                        friendResult = FRIEND_ADDED_OFFLINE;
                    if (!GetPlayer()->GetSocial()->AddToSocialList(GUID_LOPART(friendGuid), false))
                    {
                        friendResult = FRIEND_LIST_FULL;
                        //sLog->outString("WORLD: %s's friend list is full.", GetPlayer()->GetName());
                    }
                }
                GetPlayer()->GetSocial()->SetFriendNote(GUID_LOPART(friendGuid), friendNote);
            }
        }
    }

    sSocialMgr->SendFriendStatus(GetPlayer(), friendResult, GUID_LOPART(friendGuid), false);

    //sLog->outString("WORLD: Sent (SMSG_FRIEND_STATUS)");
}

void ClientSession::HandleDelFriendOpcode(WorldPacket & recv_data)
{
    uint64 FriendGUID;

    //sLog->outString("WORLD: Received CMSG_DEL_FRIEND");

    recv_data >> FriendGUID;

    _player->GetSocial()->RemoveFromSocialList(GUID_LOPART(FriendGUID), false);

    sSocialMgr->SendFriendStatus(GetPlayer(), FRIEND_REMOVED, GUID_LOPART(FriendGUID), false);

    //sLog->outString("WORLD: Sent motd (SMSG_FRIEND_STATUS)");
}

void ClientSession::HandleAddIgnoreOpcode(WorldPacket & recv_data)
{
    //sLog->outString("WORLD: Received CMSG_ADD_IGNORE");

    std::string ignoreName = GetTrinityString(LANG_FRIEND_IGNORE_UNKNOWN);

    recv_data >> ignoreName;

    if (!normalizePlayerName(ignoreName))
        return;

    sLog->outString("LOGON: %s asked to Ignore: '%s'",
        GetPlayer()->GetPlayerName().c_str(), ignoreName.c_str());

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GUID_BY_NAME_FILTER);

    stmt->setString(0, ignoreName);

    _addIgnoreCallback = CharacterDatabase.AsyncQuery(stmt);
}


void ClientSession::HandleAddIgnoreOpcodeCallBack(PreparedQueryResult result)
{
    if (!GetPlayer())
        return;

    uint64 IgnoreGuid;
    FriendsResult ignoreResult;

    ignoreResult = FRIEND_IGNORE_NOT_FOUND;
    IgnoreGuid = 0;

    if (result)
    {
        IgnoreGuid = MAKE_NEW_GUID((*result)[0].GetUInt32(), 0, HIGHGUID_PLAYER);

        if (IgnoreGuid)
        {
            if (IgnoreGuid == GetPlayer()->GetGUID())              //not add yourself
                ignoreResult = FRIEND_IGNORE_SELF;
            else if (GetPlayer()->GetSocial()->HasIgnore(GUID_LOPART(IgnoreGuid)))
                ignoreResult = FRIEND_IGNORE_ALREADY;
            else
            {
                ignoreResult = FRIEND_IGNORE_ADDED;

                // ignore list full
                if (!GetPlayer()->GetSocial()->AddToSocialList(GUID_LOPART(IgnoreGuid), true))
                    ignoreResult = FRIEND_IGNORE_FULL;
            }
        }
    }

    sSocialMgr->SendFriendStatus(GetPlayer(), ignoreResult, GUID_LOPART(IgnoreGuid), false);

    sLog->outString("WORLD: Sent (SMSG_FRIEND_STATUS)");
}

void ClientSession::HandleDelIgnoreOpcode(WorldPacket & recv_data)
{
    uint64 IgnoreGUID;

    //sLog->outString("WORLD: Received CMSG_DEL_IGNORE");

    recv_data >> IgnoreGUID;

    _player->GetSocial()->RemoveFromSocialList(GUID_LOPART(IgnoreGUID), true);

    sSocialMgr->SendFriendStatus(GetPlayer(), FRIEND_IGNORE_REMOVED, GUID_LOPART(IgnoreGUID), false);

    sLog->outString("WORLD: Sent motd (SMSG_FRIEND_STATUS)");
}

void ClientSession::HandleSetContactNotesOpcode(WorldPacket & recv_data)
{
    sLog->outString("CMSG_SET_CONTACT_NOTES");
    uint64 guid;
    std::string note;
    recv_data >> guid >> note;
    _player->GetSocial()->SetFriendNote(GUID_LOPART(guid), note);
}