#include "Chat.h"
#include "ChannelMgr.h"
#include "Language.h"
#include "Log.h"
#include "ClientSession.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Player.h"

bool ChatHandler::HandleGMCommand(const char *args)
{
    if (!*args)
    {
        if (m_session->GetPlayer()->isGameMaster())
            m_session->SendNotification(LANG_GM_ON);
        else
            m_session->SendNotification(LANG_GM_OFF);
        m_session->SendCommandStringToNode(".gm");
        return true;
    }

    std::string param = (char*)args;

    if (param == "on")
    {
        m_session->GetPlayer()->SetGameMaster(true);
        m_session->SendNotification(LANG_GM_ON);
        m_session->SendCommandStringToNode(".gm on");
        return true;
    }

    if (param == "off")
    {
        m_session->GetPlayer()->SetGameMaster(false);
        m_session->SendNotification(LANG_GM_OFF);
        m_session->SendCommandStringToNode(".gm off");
        return true;
    }

    SendSysMessage(LANG_USE_BOL);
    SetSentErrorMessage(true);
    return false;
}

bool ChatHandler::HandleToggleGmVisible(const char *args)
{
    if (!*args)
        return false;

    if (strcmp(args, "on") == 0)
    {
        m_session->GetPlayer()->SetGMVisible(true);
        m_session->SendNotification(LANG_INVISIBLE_VISIBLE);
        m_session->SendCommandStringToNode(".gm visible on");
        return true;
    }

    if (strcmp(args, "off") == 0)
    {
        m_session->SendNotification(LANG_INVISIBLE_INVISIBLE);
        m_session->GetPlayer()->SetGMVisible(false);
        m_session->SendCommandStringToNode(".gm visible off");
        return true;
    }
    return false;
}

bool ChatHandler::HandleGMChatCommand(const char *args)
{
    if (!*args)
        return false;

    if (strcmp(args, "on") == 0)
    {
        m_session->GetPlayer()->SetGMChat(true);
        m_session->SendNotification(LANG_GM_CHAT_ON);
        m_session->SendCommandStringToNode(".gm chat on");
    }
    else if (strcmp(args, "off") == 0)
    {
        m_session->GetPlayer()->SetGMChat(false);
        m_session->SendNotification(LANG_GM_CHAT_OFF);
        m_session->SendCommandStringToNode(".gm chat off");
    }
    return true;
}

//Enable\Dissable accept whispers (for GM)
bool ChatHandler::HandleWhispersCommand(const char* args)
{
    if (!*args)
    {
        PSendSysMessage(LANG_COMMAND_WHISPERACCEPTING, m_session->GetPlayer()->isAcceptWhispers() ?  GetTrinityString(LANG_ON) : GetTrinityString(LANG_OFF));
        return true;
    }

    std::string argstr = (char*)args;
    // whisper on
    if (argstr == "on")
    {
        m_session->GetPlayer()->SetAcceptWhispers(true);
        SendSysMessage(LANG_COMMAND_WHISPERON);
        return true;
    }

    // whisper off
    if (argstr == "off")
    {
        // Remove all players from the Gamemaster's whisper whitelist
        m_session->GetPlayer()->ClearWhisperWhiteList();
        m_session->GetPlayer()->SetAcceptWhispers(false);
        SendSysMessage(LANG_COMMAND_WHISPEROFF);
        return true;
    }

    SendSysMessage(LANG_USE_BOL);
    SetSentErrorMessage(true);
    return false;
}
