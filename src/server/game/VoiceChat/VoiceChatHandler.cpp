/*
* PB
* Voice Chat Engine
* Copyright (C) 2016 JunkyBulgaria <emiliyan9604bg@gmail.com>
*/

#include "ScriptPCH.h"
#include "WorldSession.h"
#include "ChannelMgr.h"
#include "VoiceChatHandler.h"
#include "Common.h"
#include "DatabaseEnv.h"
#include "Opcodes.h"
#include "Log.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "ObjectMgr.h"
#include "GroupMgr.h"
#include "Pet.h"
#include "Player.h"
#include "Group.h"
#include "SocialMgr.h"
#include "Util.h"
#include "SpellAuras.h"
#include "Vehicle.h"
#include "Chat.h"
#include "Language.h"
#include "World.h"
#include "ByteBuffer.h"

void WorldSession::HandleEnableMicrophoneOpcode(WorldPacket& revData)
{
    bool voiceEnabled, micEnabled;
    revData >> voiceEnabled;
    revData >> micEnabled;
    revData.rfinish();
}

void WorldSession::HandleChannelVoiceQueryOpcode(WorldPacket& revData)
{
    return;
    std::string name;
    revData >> name;

    // custom channel
    ChannelMgr* cMgr = ChannelMgr::forTeam(_player->GetTeamId()); // crashy thrashy (crash here) @todo
    if (!cMgr)
        return;

    Channel* chn = cMgr->GetChannel(name.c_str(), _player);
    if(!chn)
        return;

    WorldPacket data(SMSG_AVAILABLE_VOICE_CHANNEL, 17 + chn->GetName().size());
    data << uint32(0x00002e57);
    data << uint32(0xe0e10000);
    data << uint8(0);        // 00=custom,03=party,04=raid
    data << chn->GetName();
    data << _player->GetGUID();
    SendPacket(&data);
    sLog->outString("WorldSession::HandleChannelVoiceQueryOpcode");
}

void WorldSession::HandleVoiceChatQueryOpcode(WorldPacket& revData)
{
    //if(!sVoiceChatHandler.CanUseVoiceChat())
        //return;

    uint32 type;
    std::string name;
    revData >> type >> name;
    if(type == 0)
    {
        // custom channel
        ChannelMgr* cMgr = ChannelMgr::forTeam(_player->GetTeamId());
        if (!cMgr)
            return;

        Channel* chn = cMgr->GetChannel(name.c_str(), _player);
        if (!chn)
            return;

        //if(!chn->Voice())
            //return;

        if (ChannelMgr* cMgr = ChannelMgr::forTeam(GetPlayer()->GetTeamId()))
            if (Channel* channel = cMgr->GetChannel(name.c_str(), GetPlayer()))
                channel->List(GetPlayer());

        chn->JoinVoiceChannel(_player);
    }
    sLog->outString("WorldSession::HandleVoiceChatQueryOpcode");
    //revData.hexlike(true);
}

void WorldSession::HandleChannelVoiceOnOpcode(WorldPacket& revData)
{
    sLog->outString("WORLD: Received CMSG_CHANNEL_VOICE_ON");
    revData.hexlike(true);
}

void WorldSession::HandleChannelVoiceOffOpcode(WorldPacket& revData)
{
    sLog->outString("WORLD: Received CMSG_CHANNEL_VOICE_OFF");
    revData.hexlike(true);
}

void WorldSession::HandleVoiceIgnorePlayerByName(WorldPacket& recvData)
{
    std::string name;
    recvData >> name;
     
    Player* ignored = sObjectAccessor->FindPlayerByName(name);
    if(!ignored)
        return;
    
    _player->GetSocial()->AddToSocialList(ignored->GetGUIDLow(), false, true);
    _player->GetSocial()->SendSocialList(_player);
    
    recvData.rfinish();
}
void WorldSession::HandleVoiceUnIgnorePlayerByName(WorldPacket& recvData)
{
    uint64 guid;
    recvData >> guid;

    Player* ignored = sObjectAccessor->FindPlayerInOrOutOfWorld(guid);

    _player->GetSocial()->RemoveFromSocialList(ignored->GetGUIDLow(), false, true);
    _player->GetSocial()->SendSocialList(_player);

    recvData.rfinish();
}

// BEGIN VOICE CHAT IMPLEMENTATION

VoiceChatHandler::VoiceChatHandler()
{
    request_high = 1; 
} 

void VoiceChatHandler::CreateVoiceChannel(Channel* chn, Player* player)
{
    if (!player)
        return;

    sLog->outString("VoiceChatHandler::CreateVoiceChannel %s", chn->GetName());
    VoiceChatChannelRequest req;

    //m_lock.Acquire();

    req.id = request_high++;
    req.channel_name = chn->GetName();
    req.team = TEAM_ALLIANCE;
    req.groupid = 0;
    m_requests.push_back(req);

    WorldPacket data(CMSG_CHANNEL_VOICE_ON, 5);
    data << (uint8)0;
    data << req.id;
    player->GetSession()->SendPacket(&data);
    //m_lock.Release();
} 

void VoiceChatHandler::Update()
{ 
    if (m_client == NULL)
    {
        if (sWorld->GetGameTime() > next_connect)
        {
            sLog->outString("VoiceChatHandler Attempting to connect to voicechat server %s:%u", ip_s.c_str(), port);
            //VoiceChatClientSocket * s = ConnectTCPSocket<VoiceChatClientSocket>(ip_s.c_str(), port);
            //if (s != NULL)
            //{
                // connected!
                //m_client = s;
                sLog->outString("VoiceChatHandler Connected to %s:%u.", ip_s.c_str(), port);
                WorldPacket data1(SMSG_VOICE_CHAT_STATUS, 2);
                data1 << uint8(2) << uint8(1);
                sWorld->SendGlobalMessage(&data1, NULL);

                //objmgr.GroupVoiceReconnected();
                sLog->outString("Voice services are back online.");
            //}
            //else
            //{
                sLog->outString("VoiceChatHandler: Could not connect. Will try again later.");
                m_client = NULL;
                next_connect = sWorld->GetGameTime() + 10;
            //}
        }
    }
    else
    {
        if (sWorld->GetGameTime() >= m_client->next_ping)
        {
            //m_client->next_ping = sWorld->GetGameTime() + 15;
            //WorldPacket data2(VOICECHAT_CMSG_PING, 4);
            //data2 << uint32(0);
            //m_client->SendPacket(&data2);
        }
        // because the above might kill m_client
        if (m_client != NULL && (sWorld->GetGameTime() - m_client->last_pong) > (15 * 3))
        {
            // ping timeout
            //printf("ping timeout on voice socket\n");
            //m_client->Disconnect();
        }
    }
}
