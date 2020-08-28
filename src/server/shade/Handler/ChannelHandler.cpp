#include "Common.h"
#include "ClientSession.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Channel.h"
#include "ChannelMgr.h"
#include "AccountMgr.h"

void ClientSession::HandleJoinChannel(WorldPacket& recvPacket)
{
    ////sLog->outString("Opcode %u", recvPacket.GetOpcode());

    uint32 channel_id;
    uint8 unknown1, unknown2;
    std::string channelname, pass;

    recvPacket >> channel_id;
    recvPacket >> unknown1 >> unknown2;
    recvPacket >> channelname;
    recvPacket >> pass;

    if (channelname.empty())
        return;

    if (AccountMgr::IsVIPorPlayer(GetSecurity()))
    {
        if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        {
            cMgr->team = _player->GetTeam();
            if (Channel* chn = cMgr->GetJoinChannel(channelname, channel_id))
                chn->Join(_player->GetGUID(), pass.c_str());
        }
    }
    else
    {
        if (ChannelMgr* cMgr = channelMgr(ALLIANCE))
        {
            cMgr->team = ALLIANCE;
            if (Channel* chn = cMgr->GetJoinChannel(channelname, channel_id))
                chn->Join(_player->GetGUID(), pass.c_str());
        }
        if (ChannelMgr* cMgr = channelMgr(HORDE))
        {
            cMgr->team = HORDE;
            if (Channel* chn = cMgr->GetJoinChannel(channelname, channel_id))
                chn->Join(_player->GetGUID(), pass.c_str());
        }
    }
}

void ClientSession::HandleLeaveChannel(WorldPacket& recvPacket)
{
    //sLog->outString("Opcode %u", recvPacket.GetOpcode());

    uint32 unk;
    std::string channelname;
    recvPacket >> unk;                                      // channel id
    recvPacket >> channelname;

    if (channelname.empty())
        return;


    if (AccountMgr::IsVIPorPlayer(GetSecurity()))
    {
        if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        {
            if (Channel* chn = cMgr->GetChannel(channelname, _player))
                chn->Leave(_player->GetGUID(), true);
            cMgr->LeftChannel(channelname);
        }
    }
    else
    {
        if (ChannelMgr* cMgr = channelMgr(ALLIANCE))
        {
            if (Channel* chn = cMgr->GetChannel(channelname, _player))
                chn->Leave(_player->GetGUID(), true);
            cMgr->LeftChannel(channelname);
        }
        if (ChannelMgr* cMgr = channelMgr(HORDE))
        {
            if (Channel* chn = cMgr->GetChannel(channelname, _player))
                chn->Leave(_player->GetGUID(), true);
            cMgr->LeftChannel(channelname);
        }
    }
}

void ClientSession::HandleChannelList(WorldPacket& recvPacket)
{
    //sLog->outString("Opcode %u", recvPacket.GetOpcode());
    std::string channelname;
    recvPacket >> channelname;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->List(_player);
}

void ClientSession::HandleChannelPassword(WorldPacket& recvPacket)
{
    //sLog->outString("Opcode %u", recvPacket.GetOpcode());
    std::string channelname, pass;
    recvPacket >> channelname;
    recvPacket >> pass;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            //prevent players from adding passwords to global world channels
            if (!strcmp(channelname.c_str(), "World") || !strcmp(channelname.c_str(), "world"))
                chn->Password(_player->GetGUID(), pass.c_str());
}

void ClientSession::HandleChannelSetOwner(WorldPacket& recvPacket)
{
    //sLog->outString("Opcode %u", recvPacket.GetOpcode());
    std::string channelname, newp;
    recvPacket >> channelname;
    recvPacket >> newp;

    if (!normalizePlayerName(newp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->SetOwner(_player->GetGUID(), newp.c_str());
}

void ClientSession::HandleChannelOwner(WorldPacket& recvPacket)
{
    //sLog->outString("Opcode %u", recvPacket.GetOpcode());
    std::string channelname;
    recvPacket >> channelname;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->SendWhoOwner(_player->GetGUID());
}

void ClientSession::HandleChannelModerator(WorldPacket& recvPacket)
{
    //sLog->outString("Opcode %u", recvPacket.GetOpcode());
    std::string channelname, otp;
    recvPacket >> channelname;
    recvPacket >> otp;

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->SetModerator(_player->GetGUID(), otp.c_str());
}

void ClientSession::HandleChannelUnmoderator(WorldPacket& recvPacket)
{
    //sLog->outString("Opcode %u", recvPacket.GetOpcode());
    std::string channelname, otp;
    recvPacket >> channelname;

    recvPacket >> otp;
    
    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->UnsetModerator(_player->GetGUID(), otp.c_str());
}

void ClientSession::HandleChannelMute(WorldPacket& recvPacket)
{
    //sLog->outString("Opcode %u", recvPacket.GetOpcode());
    std::string channelname, otp;
    recvPacket >> channelname;

    recvPacket >> otp;

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->SetMute(_player->GetGUID(), otp.c_str());
}

void ClientSession::HandleChannelUnmute(WorldPacket& recvPacket)
{
    //sLog->outString("Opcode %u", recvPacket.GetOpcode());

    std::string channelname, otp;
    recvPacket >> channelname;

    recvPacket >> otp;

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->UnsetMute(_player->GetGUID(), otp.c_str());
}

void ClientSession::HandleChannelInvite(WorldPacket& recvPacket)
{
    //sLog->outString("Opcode %u", recvPacket.GetOpcode());
    std::string channelname, otp;
    recvPacket >> channelname;

    recvPacket >> otp;

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->Invite(_player->GetGUID(), otp.c_str());
}

void ClientSession::HandleChannelKick(WorldPacket& recvPacket)
{
    //sLog->outString("Opcode %u", recvPacket.GetOpcode());
    std::string channelname, otp;
    recvPacket >> channelname;

    recvPacket >> otp;

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->Kick(_player->GetGUID(), otp.c_str());
}

void ClientSession::HandleChannelBan(WorldPacket& recvPacket)
{
    //sLog->outString("Opcode %u", recvPacket.GetOpcode());
    std::string channelname, otp;
    recvPacket >> channelname;

    recvPacket >> otp;
    
    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->Ban(_player->GetGUID(), otp.c_str());
}

void ClientSession::HandleChannelUnban(WorldPacket& recvPacket)
{
    //sLog->outString("Opcode %u", recvPacket.GetOpcode());

    std::string channelname, otp;
    recvPacket >> channelname;

    recvPacket >> otp;

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->UnBan(_player->GetGUID(), otp.c_str());
}

void ClientSession::HandleChannelAnnouncements(WorldPacket& recvPacket)
{
    //sLog->outString("Opcode %u", recvPacket.GetOpcode());
    std::string channelname;
    recvPacket >> channelname;
    
    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->Announce(_player->GetGUID());
}

void ClientSession::HandleChannelDisplayListQuery(WorldPacket &recvPacket)
{
    // this should be OK because the 2 function _were_ the same
    HandleChannelList(recvPacket);
}

void ClientSession::HandleGetChannelMemberCount(WorldPacket &recvPacket)
{
    //sLog->outString("Opcode %u", recvPacket.GetOpcode());
    std::string channelname;
    recvPacket >> channelname;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
    {
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
        {
            WorldPacket data(SMSG_CHANNEL_MEMBER_COUNT, chn->GetName().size()+1+1+4);
            data << chn->GetName();
            data << uint8(chn->GetFlags());
            data << uint32(chn->GetNumPlayers());
            SendPacket(&data);
        }
    }
}

void ClientSession::HandleSetChannelWatch(WorldPacket &recvPacket)
{
    //sLog->outString("Opcode %u", recvPacket.GetOpcode());
    std::string channelname;
    recvPacket >> channelname;
    
    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->JoinNotify(_player->GetGUID());
}

//Temp use
void ClientSession::SMSG_LOGIN_VERIFY_WORLD(WorldPacket& recvPacket)
{
    uint8 unknown = 0;
    WorldPacket data;

    if (m_chlist.empty())
        return;

    /*ChannelList::iterator iter = m_chlist.begin();
    for (; iter != m_chlist.end(); ++iter)  
    {
        data.Initialize(CMSG_JOIN_CHANNEL);
        data << iter->second.Channel_ID;
        data << unknown << unknown;
        data << iter->second.ChannelName.c_str();
        data << iter->second.ChannelPass;
        SendPacketToNode(&data);
    }*/
}
