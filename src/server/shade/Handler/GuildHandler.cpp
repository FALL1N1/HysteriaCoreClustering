#include "Common.h"
#include "WorldPacket.h"
#include "ClientSession.h"
#include "Proxy.h"
#include "ObjectMgr.h"
#include "GuildsMgr.h"
#include "Log.h"
#include "Opcodes.h"
#include "Guilds.h" 
#include "SocialMgr.h"

void ClientSession::HandleGuildQueryOpcode(WorldPacket& recvPacket)
{
    uint32 guildId;
    recvPacket >> guildId;

   sLog->outString("CMSG_GUILD_QUERY [%s]: Guild: %u", GetPlayer()->GetName(), guildId);
    if (!guildId)
        return;

    if (Guild* guild = sGuildMgr->GetGuildById(guildId))
        guild->HandleQuery(this);
}

void ClientSession::HandleGuildCreateOpcode(WorldPacket& recvPacket)
{ 
}

void ClientSession::HandleGuildInviteOpcode(WorldPacket& recvPacket)
{
    std::string invitedName;
    recvPacket >> invitedName;

    ;//sLog->outDebug(LOG_FILTER_GUILD, "CMSG_GUILD_INVITE [%s]: Invited: %s", GetPlayerInfo().c_str(), invitedName.c_str());
    if (normalizePlayerName(invitedName))
        if (Guild* guild = GetPlayer()->GetGuild())
            guild->HandleInviteMember(this, invitedName);
}

void ClientSession::HandleGuildRemoveOpcode(WorldPacket& recvPacket)
{
    std::string playerName;
    recvPacket >> playerName;

    ;//sLog->outDebug(LOG_FILTER_GUILD, "CMSG_GUILD_REMOVE [%s]: Target: %s", GetPlayerInfo().c_str(), playerName.c_str());

    if (normalizePlayerName(playerName))
        if (Guild* guild = GetPlayer()->GetGuild())
            guild->HandleRemoveMember(this, playerName);
}

void ClientSession::HandleGuildAcceptOpcode(WorldPacket& /*recvPacket*/)
{
    sLog->outString("CMSG_GUILD_ACCEPT [%s]", GetPlayer()->GetName());

    if (!GetPlayer()->GetGuildId())
    {
        sLog->outString("aha");
        if (Guild* guild = sGuildMgr->GetGuildById(GetPlayer()->GetGuildIdInvited()))
        {
            sLog->outString("mhm");
            guild->HandleAcceptMember(this);
        }
    }
}

void ClientSession::HandleGuildDeclineOpcode(WorldPacket& /*recvPacket*/)
{
    ;//sLog->outDebug(LOG_FILTER_GUILD, "CMSG_GUILD_DECLINE [%s]", GetPlayerInfo().c_str());

    GetPlayer()->SetGuildIdInvited(0);
    GetPlayer()->SetInGuild(0);
}

void ClientSession::HandleGuildInfoOpcode(WorldPacket& /*recvPacket*/)
{
    ;//sLog->outDebug(LOG_FILTER_GUILD, "CMSG_GUILD_INFO [%s]", GetPlayerInfo().c_str());

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->SendInfo(this);
}

void ClientSession::HandleGuildRosterOpcode(WorldPacket& /*recvPacket*/)
{
    ;//sLog->outDebug(LOG_FILTER_GUILD, "CMSG_GUILD_ROSTER [%s]", GetPlayerInfo().c_str());

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->HandleRoster(this);
    else
        Guild::SendCommandResult(this, GUILD_COMMAND_ROSTER, ERR_GUILD_PLAYER_NOT_IN_GUILD);
}

void ClientSession::HandleGuildPromoteOpcode(WorldPacket& recvPacket)
{
    std::string playerName;
    recvPacket >> playerName;

    ;//sLog->outDebug(LOG_FILTER_GUILD, "CMSG_GUILD_PROMOTE [%s]: Target: %s", GetPlayerInfo().c_str(), playerName.c_str());

    if (normalizePlayerName(playerName))
        if (Guild* guild = GetPlayer()->GetGuild())
            guild->HandleUpdateMemberRank(this, playerName, false);
}

void ClientSession::HandleGuildDemoteOpcode(WorldPacket& recvPacket)
{
    std::string playerName;
    recvPacket >> playerName;

    ;//sLog->outDebug(LOG_FILTER_GUILD, "CMSG_GUILD_DEMOTE [%s]: Target: %s", GetPlayerInfo().c_str(), playerName.c_str());

    if (normalizePlayerName(playerName))
        if (Guild* guild = GetPlayer()->GetGuild())
            guild->HandleUpdateMemberRank(this, playerName, true);
}

void ClientSession::HandleGuildLeaveOpcode(WorldPacket& /*recvPacket*/)
{
    ;//sLog->outDebug(LOG_FILTER_GUILD, "CMSG_GUILD_LEAVE [%s]", GetPlayerInfo().c_str());

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->HandleLeaveMember(this);
}

void ClientSession::HandleGuildDisbandOpcode(WorldPacket& /*recvPacket*/)
{
    ;//sLog->outDebug(LOG_FILTER_GUILD, "CMSG_GUILD_DISBAND [%s]", GetPlayerInfo().c_str());

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->HandleDisband(this);
}

void ClientSession::HandleGuildLeaderOpcode(WorldPacket& recvPacket)
{
    std::string name;
    recvPacket >> name;

    ;//sLog->outDebug(LOG_FILTER_GUILD, "CMSG_GUILD_LEADER [%s]: Target: %s", GetPlayerInfo().c_str(), name.c_str());

    if (normalizePlayerName(name))
        if (Guild* guild = GetPlayer()->GetGuild())
            guild->HandleSetLeader(this, name);
}

void ClientSession::HandleGuildMOTDOpcode(WorldPacket& recvPacket)
{
    std::string motd;
    recvPacket >> motd;

    ;//sLog->outDebug(LOG_FILTER_GUILD, "CMSG_GUILD_MOTD [%s]: MOTD: %s", GetPlayerInfo().c_str(), motd.c_str());

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->HandleSetMOTD(this, motd);
}

void ClientSession::HandleGuildSetPublicNoteOpcode(WorldPacket& recvPacket)
{
    std::string playerName;
    std::string note;
    recvPacket >> playerName >> note;

    ;//sLog->outDebug(LOG_FILTER_GUILD, "CMSG_GUILD_SET_PUBLIC_NOTE [%s]: Target: %s, Note: %s",
     //    GetPlayerInfo().c_str(), playerName.c_str(), note.c_str());

    if (normalizePlayerName(playerName))
        if (Guild* guild = GetPlayer()->GetGuild())
            guild->HandleSetMemberNote(this, playerName, note, true);
}

void ClientSession::HandleGuildSetOfficerNoteOpcode(WorldPacket& recvPacket)
{
    std::string playerName;
    std::string note;
    recvPacket >> playerName >> note;

    ;//sLog->outDebug(LOG_FILTER_GUILD, "CMSG_GUILD_SET_OFFICER_NOTE [%s]: Target: %s, Note: %s",
    //     GetPlayerInfo().c_str(), playerName.c_str(), note.c_str());

    if (normalizePlayerName(playerName))
        if (Guild* guild = GetPlayer()->GetGuild())
            guild->HandleSetMemberNote(this, playerName, note, false);
}

void ClientSession::HandleGuildRankOpcode(WorldPacket& recvPacket)
{
    uint32 rankId;
    recvPacket >> rankId;

    uint32 rights;
    recvPacket >> rights;

    std::string rankName;
    recvPacket >> rankName;

    uint32 money;
    recvPacket >> money;

    ;//sLog->outDebug(LOG_FILTER_GUILD, "CMSG_GUILD_RANK [%s]: Rank: %s (%u)", GetPlayerInfo().c_str(), rankName.c_str(), rankId);

    Guild* guild = GetPlayer()->GetGuild();
    if (!guild)
    {
        recvPacket.rpos(recvPacket.wpos());
        return;
    }

    GuildBankRightsAndSlotsVec rightsAndSlots(GUILD_BANK_MAX_TABS);

    for (uint8 tabId = 0; tabId < GUILD_BANK_MAX_TABS; ++tabId)
    {
        uint32 bankRights;
        uint32 slots;

        recvPacket >> bankRights;
        recvPacket >> slots;

        rightsAndSlots[tabId] = GuildBankRightsAndSlots(tabId, bankRights, slots);
    }

    guild->HandleSetRankInfo(this, rankId, rankName, rights, money, rightsAndSlots);
}

void ClientSession::HandleGuildAddRankOpcode(WorldPacket& recvPacket)
{
    std::string rankName;
    recvPacket >> rankName;

    ;//sLog->outDebug(LOG_FILTER_GUILD, "CMSG_GUILD_ADD_RANK [%s]: Rank: %s", GetPlayerInfo().c_str(), rankName.c_str());

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->HandleAddNewRank(this, rankName);
}

void ClientSession::HandleGuildDelRankOpcode(WorldPacket& /*recvPacket*/)
{
    ;//sLog->outDebug(LOG_FILTER_GUILD, "CMSG_GUILD_DEL_RANK [%s]", GetPlayerInfo().c_str());

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->HandleRemoveLowestRank(this);
}

void ClientSession::HandleGuildChangeInfoTextOpcode(WorldPacket& recvPacket)
{
    std::string info;
    recvPacket >> info;

    ;//sLog->outDebug(LOG_FILTER_GUILD, "CMSG_GUILD_INFO_TEXT [%s]: %s", GetPlayerInfo().c_str(), info.c_str());

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->HandleSetInfo(this, info);
} 

void ClientSession::HandleGuildEventLogQueryOpcode(WorldPacket& /* recvPacket */)
{
    ;//sLog->outDebug(LOG_FILTER_GUILD, "MSG_GUILD_EVENT_LOG_QUERY [%s]", GetPlayerInfo().c_str());

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->SendEventLog(this);
}