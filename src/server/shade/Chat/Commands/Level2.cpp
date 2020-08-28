#include "Chat.h"
#include "ChannelMgr.h"
#include "ObjectMgr.h"
#include "Language.h"
#include "Log.h"
#include "ClientSession.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "ClientSessionMgr.h"
#include "Language.h"

bool ChatHandler::HandleAntiSpamInsertCommand(const char *args)
{
    if (!*args)
        return false;

    char* tail1 = strtok((char*) args, " ");
    if (!tail1)
        return false;

    char* tin = extractQuotedArg(tail1);
    if (!tin)
        return false;

    std::string in = tin;
    std::transform(in.begin(), in.end(), in.begin(), toupper);

    LoginDatabase.EscapeString(in);
    LoginDatabase.DirectPExecute("REPLACE antispam (input) VALUES ('%s')", in.c_str());

    sLogon->InsertSpamMsg(in);
    return true;
}

bool ChatHandler::HandleAntiSpamReload(const char *args)
{
    sLogon->InitAntiSpam();
    PSendSysMessage("AntiSpam Reloaded");
    return true;
}

bool ChatHandler::HandleSetFakeMSGCommand(const char *args)
{
    if (!*args)
        return false;

    sLogon->SetFakeMsg(args);
    return true;
}

bool ChatHandler::HandleLoginCommand(const char *args)
{
    if (!*args)
        return false;

    m_session->SetAuthed(true);

    return true;
}

//mute player for some times
bool ChatHandler::HandleMuteCommand(const char* args)
{
    if (!*args)
        return false;

    char* nameStr;
    char* delayStr;
    extractOptFirstArg((char*)args, &nameStr, &delayStr);
    if (!delayStr && !nameStr)
        return false;

    char *mutereason = strtok(NULL, "\r");
    std::string mutereasonstr = "No reason";
    if (mutereason != NULL)
        mutereasonstr = mutereason;

    Player* target;
    uint64 target_guid;
    std::string target_name;
    if (!extractPlayerTarget(nameStr, &target, &target_guid, &target_name))
        return false;

    uint32 accountId = 0;
    if (IS_PLAYER_GUID(target_guid))
        accountId = sObjectMgr->GetPlayerAccountIdByGUID(target_guid);

    // find only player from same account if any
    if (!target)
        if (ClientSession* session = sClientSessionMgr->FindSession(accountId))
            target = session->GetPlayer();

    uint32 notspeaktime = (uint32) atoi(delayStr);

    // must have strong lesser security level
    if (HasLowerSecurity (NULL, target_guid, true))
        return false;

    PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_MUTE_TIME);

    if (target)
    {
        // Target is online, mute will be in effect right away.
        int64 muteTime = time(NULL) + notspeaktime * MINUTE;
        target->GetSession()->_muteTime = muteTime;

        stmt->setInt64(0, muteTime);

        ChatHandler(target).PSendSysMessage(LANG_YOUR_CHAT_DISABLED, notspeaktime, mutereasonstr.c_str());
    }
    else
    {
        // Target is offline, mute will be in effect starting from the next login.
        int32 muteTime = -int32(notspeaktime * MINUTE);

        stmt->setInt64(0, muteTime);
    }

    stmt->setUInt32(1, accountId);

    LoginDatabase.Execute(stmt);

    std::string nameLink = playerLink(target_name);

    PSendSysMessage(target ? LANG_YOU_DISABLE_CHAT : LANG_COMMAND_DISABLE_CHAT_DELAYED, nameLink.c_str(), notspeaktime, mutereasonstr.c_str());
    //m_session->SendCommandStringToNode(".mute %s", cargs);
    return true;
}

//unmute player
bool ChatHandler::HandleUnmuteCommand(const char* args)
{
    Player* target;
    uint64 target_guid;
    std::string target_name;
    if (!extractPlayerTarget((char*)args, &target, &target_guid, &target_name))
        return false;

    uint32 accountId = 0;
    if (IS_PLAYER_GUID(target_guid))
        accountId = sObjectMgr->GetPlayerAccountIdByGUID(target_guid);

    // find only player from same account if any
    if (!target)
        if (ClientSession* session = sClientSessionMgr->FindSession(accountId))
            target = session->GetPlayer();

    // must have strong lesser security level
    if (HasLowerSecurity (NULL, target_guid, true))
        return false;

    if (target)
    {
        if (target->CanSpeak())
        {
            SendSysMessage(LANG_CHAT_ALREADY_ENABLED);
            SetSentErrorMessage(true);
            return false;
        }

        target->GetSession()->_muteTime = 0;
    }

    PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_MUTE_TIME);

    stmt->setInt64(0, 0);
    stmt->setUInt32(1, accountId);

    LoginDatabase.Execute(stmt);

    if (target)
        ChatHandler(target).PSendSysMessage(LANG_YOUR_CHAT_ENABLED);

    std::string nameLink = playerLink(target_name);

    PSendSysMessage(LANG_YOU_ENABLE_CHAT, nameLink.c_str());
    //m_session->SendCommandStringToNode(".unmute %s", cargs);
    return true;
}

bool ChatHandler::HandleChannelSetOwnership(const char *args)
{
    if (!*args)
        return false;

    /*    char *channel = strtok((char*)args, " ");
        char *argstr = strtok(NULL, "");

        if (!channel || !argstr)
            return false;

        Player* player = m_session->GetPlayer();
        Channel* chn = NULL;

        if (ChannelMgr* cMgr = channelMgr(player->GetTeam()))
            chn = cMgr->GetChannel(channel, player);

        if (strcmp(argstr, "on") == 0)
        {
            if (chn)
                chn->SetOwnership(true);
            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHANNEL_OWNERSHIP);
            stmt->setUInt8 (0, 1);
            stmt->setString(1, channel);
            CharacterDatabase.Execute(stmt);
            PSendSysMessage(LANG_CHANNEL_ENABLE_OWNERSHIP, channel);
        }
        else if (strcmp(argstr, "off") == 0)
        {
            if (chn)
                chn->SetOwnership(false);
            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHANNEL_OWNERSHIP);
            stmt->setUInt8 (0, 0);
            stmt->setString(1, channel);
            CharacterDatabase.Execute(stmt);
            PSendSysMessage(LANG_CHANNEL_DISABLE_OWNERSHIP, channel);
        }
        else
            return false;
    */
    return true;
}

bool ChatHandler::HandleKickCommand(const char* args)
{
    if (!*args)
        return false;

    Player* target;
    uint64 target_guid;
    std::string target_name;
    if (!extractPlayerTarget((char*)args, &target, &target_guid, &target_name))
        return false;

    uint32 accountId = target ? target->GetSession()->GetAccountId() : sObjectMgr->GetPlayerAccountIdByGUID(target_guid);

    // find only player from same account if any
    if (!target)
        if (ClientSession* session = sClientSessionMgr->FindSession(accountId))
            target = session->GetPlayer();

    // must have strong lesser security level
    if (HasLowerSecurity (target, target_guid, true))
        return false;

    if (target)
        target->GetSession()->KickPlayer();

    return true;
}
