/*
 * PB
 * Voice Chat Engine
 * Copyright (C) 2016 JunkyBulgaria <emiliyan9604bg@gmail.com>
 */

#ifndef __VOICECHATHANDLER_H
#define __VOICECHATHANDLER_H

#include "Common.h"
#include "Config.h"
#include <ace/Singleton.h>
#include "WorldPacket.h"
#include "Channel.h"
#include "Player.h"
#include "VoiceChatClientSocket.h"

struct VoiceChatChannelRequest
{
    std::string channel_name;
    uint32 team;
    uint32 id;
    uint32 groupid;
};

struct VoiceChannel
{
    void* miscPointer;
    uint8 type;
    uint8 team;
    uint32 channelId;
};

enum VoiceChatOpcodes
{
	VOICECHAT_CMSG_CREATE_CHANNEL			= 1,
	VOICECHAT_SMSG_CHANNEL_CREATED			= 2,
	VOICECHAT_CMSG_ADD_MEMBER				= 3,
	VOICECHAT_CMSG_REMOVE_MEMBER			= 4,
	VOICECHAT_CMSG_DELETE_CHANNEL			= 5,
	VOICECHAT_CMSG_PING						= 6,
	VOICECHAT_SMSG_PONG						= 7,
	VOICECHAT_NUM_OPCODES					= 8,
};

class VoiceChatHandler
{ 
    friend class ACE_Singleton<VoiceChatHandler, ACE_Null_Mutex>;
    uint32 request_high;
    uint32 ip;
    uint16 port;
    time_t next_connect;
    bool enabled;
    std::string ip_s;
    VoiceChatClientSocket* m_client;
    std::vector<VoiceChatChannelRequest> m_requests;
    std::unordered_map<uint32, VoiceChannel*> m_voiceChannels;

    VoiceChatHandler();
    void Startup();
    void Update();
    void SocketDisconnected();
    void OnRead(WorldPacket* pck);

    uint32 GetVoiceServerIP() { return ip; }
    uint16 GetVoiceServerPort() { return port; }
    void CreateVoiceChannel(Channel * chn);
    void DestroyVoiceChannel(Channel * chn);
    bool CanCreateVoiceChannel(Channel * chn);
    bool CanUseVoiceChat();

    void CreateGroupChannel(Group * pGroup);
    void DestroyGroupChannel(Group * pGroup);
    void ActivateChannelSlot(uint16 channel_id, uint8 slot_id);
    void DeactivateChannelSlot(uint16 channel_id, uint8 slot_id);

public:
    //~VoiceChatHandler(); 
    void CreateVoiceChannel(Channel* channel, Player* player);
};

#define sVoiceChatHandler ACE_Singleton<VoiceChatHandler, ACE_Null_Mutex>::instance()

#endif
