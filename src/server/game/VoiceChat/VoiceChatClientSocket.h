/*
* PB
* Voice Chat Engine
* Copyright (C) 2016 JunkyBulgaria <emiliyan9604bg@gmail.com>
*/

#ifndef _VOICECHATCLIENTSOCKET_H
#define _VOICECHATCLIENTSOCKET_H

#include "ScriptPCH.h"

class VoiceChatClientSocket
{
    uint16 op;
    uint16 remaining;
public:
    VoiceChatClientSocket(uint32 fd);
    void OnDisconnect();
    void OnRead();
    void SendPacket(WorldPacket* data);
    time_t next_ping;
    time_t last_pong;
};

#endif        // _VOICECHATCLIENTSOCKET_H

