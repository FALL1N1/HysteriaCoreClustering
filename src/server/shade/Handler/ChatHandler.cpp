#include <cctype>
#include <algorithm>
#include "Chat.h"
#include "ClientSession.h"
#include "ClientSessionMgr.h"
#include "WorldPacket.h"
#include "Common.h"
#include "Opcodes.h"
#include "Proxy.h"
#include "Language.h"
#include "AccountMgr.h"
#include "ChannelMgr.h"
#include "Channel.h"
#include "LocalesMgr.h"
#include "GMQualityManager.h"

std::string GenerateGMResponse(std::string msg, Player* plr)
{
    if (plr->isGMChat())
    {
        return "|cff4fdeff" /* light blue */ + msg + "|r";
    }
    else return msg;
}


inline bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

bool ClientSession::checkMsgForMute(Player* sender, WorldPacket & recv_data)
{
    if (!sender->CanSpeak())
    {
        std::string timeStr = secsToTimeString(_muteTime - time(NULL));
        SendNotification(GetTrinityString(LANG_WAIT_BEFORE_SPEAKING), timeStr.c_str());
        recv_data.rfinish(); // Prevent warnings
        return true;
    }
    return false;
}

bool ClientSession::checkMsgForCommand(std::string msg, WorldPacket & recv_data)
{
    if (msg.c_str()[0] == '!' || msg.c_str()[0] == '.')
    {
        if (strContains(msg, ".mute", false) || strContains(msg, ".unmute", false))
        {
            if (ChatHandler(this).ParseCommands(msg.c_str()) > 0)
                SendPacketToNode(&recv_data);
        }
        else if (strContains(msg, ".ticket", false))
            ChatHandler(this).ParseCommands(msg.c_str());
        else
            SendPacketToNode(&recv_data);
        return true;
    }
    return false;
}

void StripLineInvisibleChars(std::string &str)
{
    static std::string const invChars = " \t\7\n";

    size_t wpos = 0;

    bool space = false;
    for (size_t pos = 0; pos < str.size(); ++pos)
    {
        if (invChars.find(str[pos]) != std::string::npos)
        {
            if (!space)
            {
                str[wpos++] = ' ';
                space = true;
            }
        }
        else
        {
            if (wpos != pos)
                str[wpos++] = str[pos];
            else
                ++wpos;
            space = false;
        }
    }

    if (wpos < str.size())
        str.erase(wpos, str.size());
    if (str.find("|TInterface") != std::string::npos)
        str.clear();
}

void ClientSession::HandleMessagechatOpcode(WorldPacket & recv_data)
{
    uint32 type;
    uint32 lang;

    recv_data >> type;
    recv_data >> lang;

    if (type >= MAX_CHAT_MSG_TYPE)
    {
        sLog->outError("CHAT: Wrong message type received: %u", type);
        recv_data.rfinish();
        return;
    }

    Player* sender = GetPlayer();

    // prevent talking at unknown language (cheating)
    LanguageDesc const* langDesc = GetLanguageDescByID(lang);
    if (!langDesc)
    {
        SendNotification(LANG_UNKNOWN_LANGUAGE);
        recv_data.rfinish();
        return;
    }

    bool ignoreChecks = false;
    if (lang == LANG_ADDON)
    {
        // LANG_ADDON is only valid for the following message types
        switch (type)
        {
            case CHAT_MSG_PARTY:
            case CHAT_MSG_RAID:
            case CHAT_MSG_GUILD:
            case CHAT_MSG_BATTLEGROUND:
            case CHAT_MSG_WHISPER:
                ignoreChecks = true;
                break;
            default:
                sLog->outError("Player %s (GUID: %u) sent a chatmessage with an invalid language/message type combination",
                    GetPlayer()->GetName(), GetPlayer()->GetGUIDLow());

                recv_data.rfinish();
                return;
        }
    }

    std::string to, channel, msg, temp;
    switch (type)
    {
        case CHAT_MSG_SAY:
        case CHAT_MSG_EMOTE:
        case CHAT_MSG_YELL:
        case CHAT_MSG_PARTY:
        case CHAT_MSG_PARTY_LEADER:
        case CHAT_MSG_GUILD:
        case CHAT_MSG_OFFICER:
        case CHAT_MSG_RAID:
        case CHAT_MSG_RAID_LEADER:
        case CHAT_MSG_RAID_WARNING:
        case CHAT_MSG_BATTLEGROUND:
        case CHAT_MSG_BATTLEGROUND_LEADER:
            recv_data >> msg;
            break;
        case CHAT_MSG_WHISPER:
            recv_data >> to;
            recv_data >> msg;
            break;
        case CHAT_MSG_CHANNEL:
            recv_data >> channel;
            recv_data >> msg;
            break;
        case CHAT_MSG_AFK:
        case CHAT_MSG_DND:
            recv_data >> msg;
            if (checkMsgForCommand(msg, recv_data) || checkMsgForMute(sender, recv_data))
                return;
            ignoreChecks = true;
            break;
    }

    if (!ignoreChecks)
    {
        if (msg.empty())
            return;

        if (checkMsgForCommand(msg, recv_data) || checkMsgForMute(sender, recv_data))
        {
            if (!AccountMgr::IsVIPorPlayer(GetSecurity()))
            {
                /*if (Player* pPlayer = sender->GetSelectedPlayer())
                    sGMQualityManager->OnGMChat(CHAT_MSG_SYSTEM, sender, msg, pPlayer->GetName());
                else*/
                    sGMQualityManager->OnGMChat(CHAT_MSG_SYSTEM, sender, msg);
            }
            return;
        }

        StripLineInvisibleChars(msg);
    }

    switch (type)
    {
        case CHAT_MSG_SAY:
        case CHAT_MSG_EMOTE:
        case CHAT_MSG_YELL:
            if (lang != LANG_ADDON)
            {
                if (!AccountMgr::IsVIPorPlayer(GetSecurity()))
                    sGMQualityManager->OnGMChat(ChatMsg(type), sender, msg);
                else
                    sGMQualityManager->OnPlayerChat(ChatMsg(type), sender, msg);
            }
        case CHAT_MSG_GUILD:
        case CHAT_MSG_OFFICER:
        case CHAT_MSG_BATTLEGROUND:
        case CHAT_MSG_BATTLEGROUND_LEADER:
        {
            if (GetAntiSpamCounter() > LOGON_DOS_STRIKE_LIMIT || sLogon->CheckForSpam(msg))
            {
                IncAntiSpamCounter();
                SendFakeChatNotification(type, msg, lang);
                return;
            }

            SendPacketToNode(&recv_data);
            break;
        }
        case CHAT_MSG_PARTY:
        case CHAT_MSG_PARTY_LEADER:
        {
            // if player is in battleground, he cannot say to battleground members by /p
            /*Group* group = GetPlayer()->GetOriginalGroup();
            if (!group)
            {
                group = sender->GetGroup();
                if (!group || group->isBGGroup())
                    return;
            }

            if (type == CHAT_MSG_PARTY_LEADER && !group->IsLeader(sender->GetGUID()))
                return;

            ChatHandler::BuildChatPacket(recv_data, ChatMsg(type), Language(lang), sender->GetSession(), NULL, GenerateGMResponse(msg, sender));
            group->BroadcastPacket(&recv_data, false, group->GetMemberGroup(GetPlayer()->GetGUID()));
            //sLog->outChat("[Party][%s]: %s", sender->GetName().c_str(), msg.c_str());*/
            break;
        }
        case CHAT_MSG_RAID:
        {
            // if player is in battleground, he cannot say to battleground members by /ra
            /*Group* group = GetPlayer()->GetOriginalGroup();
            if (!group)
            {
                group = GetPlayer()->GetGroup();
                if (!group || group->isBGGroup() || !group->isRaidGroup())
                    return;
            }

            WorldPacket data;
            ChatHandler::BuildChatPacket(data, CHAT_MSG_RAID, Language(lang), sender->GetSession(), NULL, GenerateGMResponse(msg, sender));
            group->BroadcastPacket(&data, false);*/
        } break;
        case CHAT_MSG_RAID_LEADER:
        {
            // if player is in battleground, he cannot say to battleground members by /ra
            /*Group* group = GetPlayer()->GetOriginalGroup();
            if (!group)
            {
                group = GetPlayer()->GetGroup();
                if (!group || group->isBGGroup() || !group->isRaidGroup() || !group->IsLeader(sender->GetGUID()))
                    return;
            }

            WorldPacket data;
            ChatHandler::BuildChatPacket(data, CHAT_MSG_RAID_LEADER, Language(lang), sender->GetSession(), NULL, GenerateGMResponse(msg, sender));
            group->BroadcastPacket(&data, false);*/
        } break;
        case CHAT_MSG_RAID_WARNING:
        {
            /*Group* group = GetPlayer()->GetGroup();
            if (!group || !group->isRaidGroup() || !(group->IsLeader(GetPlayer()->GetGUID()) || group->IsAssistant(GetPlayer()->GetGUID())) || group->isBGGroup())
                return;

            WorldPacket data;
            //in battleground, raid warning is sent only to players in battleground - code is ok
            ChatHandler::BuildChatPacket(data, CHAT_MSG_RAID_WARNING, Language(lang), sender->GetSession(), NULL, GenerateGMResponse(msg, sender));
            group->BroadcastPacket(&data, false);*/
        } break;
        case CHAT_MSG_WHISPER:
        {
            if (GetAntiSpamCounter() > LOGON_DOS_STRIKE_LIMIT || sLogon->CheckForSpam(msg))
            {
                IncAntiSpamCounter();
                SendFakeChatNotification(type, msg, lang);
                return;
            }

            if (sender->getLevel() < sLogon->getIntConfig(CONFIG_CHAT_WHISPER_LEVEL_REQ))
            {
                SendNotification(GetTrinityString(LANG_WHISPER_REQ), sLogon->getIntConfig(CONFIG_CHAT_WHISPER_LEVEL_REQ));
                return;
            }

            if (!normalizePlayerName(to))
            {
                SendPlayerNotFoundNotice(to);
                return;
            }

            Player* receiver = sObjectMgr->FindPlayerByName(to.c_str());
            bool senderIsPlayer = AccountMgr::IsPlayerAccount(GetSecurity());
            bool receiverIsPlayer = AccountMgr::IsPlayerAccount(receiver ? receiver->GetSession()->GetSecurity() : SEC_PLAYER);

            if (!receiver || (senderIsPlayer && !receiverIsPlayer && !receiver->isAcceptWhispers() && !receiver->IsInWhisperWhiteList(sender->GetGUID())))
            {
                SendPlayerNotFoundNotice(to);
                return;
            }

            if (!sLogon->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_CHAT) && senderIsPlayer && receiverIsPlayer)
                if (GetPlayer()->GetTeam() != receiver->GetTeam())
                {
                    SendWrongFactionNotice();
                    return;
                }

            if (!GetPlayer()->CanSpeak() && !receiver->isGameMaster())
            {
                SendNotification(GetTrinityString(LANG_GM_SILENCE), GetPlayer()->GetName());
                return;
            }

            // If player is a Gamemaster and doesn't accept whisper, we auto-whitelist every player that the Gamemaster is talking to
            if (!senderIsPlayer && !sender->isAcceptWhispers() && !sender->IsInWhisperWhiteList(receiver->GetGUID()))
                sender->AddWhisperWhiteList(receiver->GetGUID());

            GetPlayer()->Whisper(msg, lang, receiver->GetGUID());

            if (sLogon->getBoolConfig(CONFIG_CHATLOG_WHISPER))
                sLog->outChat("[WHISPER] Player %s tells %s: %s", GetPlayer()->GetName(), receiver ? receiver->GetName() : "<unknown>", msg.c_str());

            if (lang != LANG_ADDON)
            {
                if (!AccountMgr::IsVIPorPlayer(GetSecurity()))
                    sGMQualityManager->OnGMChat(ChatMsg(type), sender, msg, to);
                else
                    sGMQualityManager->OnPlayerChat(ChatMsg(type), sender, msg, to);
            }
            break;
        }
        case CHAT_MSG_CHANNEL:
        {
                if (GetPlayer() && GetPlayer()->getLevel() < 10)
                    return;

                if (GetAntiSpamCounter() > LOGON_DOS_STRIKE_LIMIT || sLogon->CheckForSpam(msg))
                {
                    IncAntiSpamCounter();
                    SendFakeChannelNotification(channel, msg, lang);
                    return;
                }

                if (sLogon->getBoolConfig(CONFIG_CHATLOG_CHANNEL))
                    sLog->outChat("[CHANNEL] Player %s tells Channel [%s]: %s", GetPlayer()->GetName(), channel.c_str(), msg.c_str());
                if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
                {
                    if (Channel* chn = cMgr->GetChannel(channel, _player))
                        chn->Say(_player->GetGUID(), msg.c_str(), lang);
                }

            break;
        }
        case CHAT_MSG_AFK:
        {
            if ((msg.empty() || !_player->isAFK())) //&& !_player->isInCombat())
            {
                if (!_player->isAFK())
                {
                    if (msg.empty())
                        msg = GetTrinityString(LANG_PLAYER_AFK_DEFAULT);
                    _player->afkMsg = msg;
                }
                _player->ToggleAFK();
                if (_player->isAFK() && _player->isDND())
                    _player->ToggleDND();
            }

            break;
        }
        case CHAT_MSG_DND:
        {
            if (msg.empty() || !_player->isDND())
            {
                if (!_player->isDND())
                {
                    if (msg.empty())
                        msg = GetTrinityString(LANG_PLAYER_DND_DEFAULT);
                    _player->dndMsg = msg;
                }
                _player->ToggleDND();
                if (_player->isDND() && _player->isAFK())
                    _player->ToggleAFK();
            }

            break;
        }
        default:
            sLog->outError("CHAT: unknown message type %u, lang: %u", type, lang);
            break;
    }
}

void ClientSession::SendPlayerNotFoundNotice(std::string name)
{
    WorldPacket data(SMSG_CHAT_PLAYER_NOT_FOUND, name.size()+1);
    data << name;
    SendPacket(&data);
}

void ClientSession::SendWrongFactionNotice()
{
    WorldPacket data(SMSG_CHAT_WRONG_FACTION, 0);
    SendPacket(&data);
}

void ClientSession::SendNotification(const char *format, ...)
{
    if (format)
    {
        va_list ap;
        char szStr[1024];
        szStr[0] = '\0';
        va_start(ap, format);
        vsnprintf(szStr, 1024, format, ap);
        va_end(ap);

        WorldPacket data(SMSG_NOTIFICATION, (strlen(szStr) + 1));
        data << szStr;
        SendPacket(&data);
    }
}

void ClientSession::SendNotification(uint32 string_id, ...)
{
    char const* format = GetTrinityString(string_id);
    if (format)
    {
        va_list ap;
        char szStr[1024];
        szStr[0] = '\0';
        va_start(ap, string_id);
        vsnprintf(szStr, 1024, format, ap);
        va_end(ap);

        WorldPacket data(SMSG_NOTIFICATION, (strlen(szStr) + 1));
        data << szStr;
        SendPacket(&data);
    }
}
