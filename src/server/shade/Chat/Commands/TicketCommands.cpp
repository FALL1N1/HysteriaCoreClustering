/**
 * @file
 * @author TehKevin for Laenalith-WoW
 * @date 01.04.2015
 *
 * Contact kevin.unegg@ainet.at
 * Website: http://www.laenalith-wow.com/
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @copyright Copyright 2015 Laenalith-WoW. All rights reserved.
 */

#include "Chat.h"
#include "AccountMgr.h"
#include "Language.h"
#include "TicketMgr.h"
#include "ClientSessionMgr.h"
#include "ObjectMgr.h"


bool ChatHandler::HandleGMTicketAssignToCommand(char const* args)
{
    if (!*args)
        return false;

    char* ticketIdStr = strtok((char*)args, " ");
    uint32 ticketId = atoi(ticketIdStr);

    char* targetStr = strtok(NULL, " ");
    if (!targetStr)
        return false;

    std::string target(targetStr);
    if (!normalizePlayerName(target))
        return false;

    GmTicket* ticket = sTicketMgr->GetTicket(ticketId);
    if (!ticket || ticket->IsClosed())
    {
        SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
        return true;
    }

    // Get target information
    uint64 targetGuid = sObjectMgr->GetPlayerGUIDByName(target.c_str());
    uint64 targetAccountId = sObjectMgr->GetPlayerAccountIdByGUID(targetGuid);
    uint32 targetGmLevel = AccountMgr::GetSecurity(targetAccountId, realmID);

    // Target must exist and have administrative rights
    if (!targetGuid || AccountMgr::IsPlayerAccount(targetGmLevel))
    {
        SendSysMessage(LANG_COMMAND_TICKETASSIGNERROR_A);
        return true;
    }

    // If already assigned, leave
    if (ticket->IsAssignedTo(targetGuid))
    {
        PSendSysMessage(LANG_COMMAND_TICKETASSIGNERROR_B, ticket->GetId());
        return true;
    }

    // If assigned to different player other than current, leave
    //! Console can override though
    Player* player = GetSession() ? GetSession()->GetPlayer() : NULL;
    if (player && ticket->IsAssignedNotTo(player->GetGUID()))
    {
        PSendSysMessage(LANG_COMMAND_TICKETALREADYASSIGNED, ticket->GetId(), target.c_str());
        return true;
    }

    // Assign ticket
    SQLTransaction trans = SQLTransaction(NULL);
    ticket->SetAssignedTo(targetGuid, AccountMgr::IsAdminAccount(targetGmLevel));
    ticket->SaveToDB(trans);
    sTicketMgr->UpdateLastChange();

    std::string msg = ticket->FormatMessageString(*this, NULL, target.c_str(), NULL, NULL);
    sClientSessionMgr->SendGameMasterMessage(msg.c_str());
    return true;
}

bool ChatHandler::HandleGMTicketCloseByIdCommand(char const* args)
{
    if (!*args)
        return false;

    uint32 ticketId = atoi(args);
    GmTicket* ticket = sTicketMgr->GetTicket(ticketId);
    if (!ticket || ticket->IsClosed() || ticket->IsCompleted())
    {
        SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
        return true;
    }

    // Ticket should be assigned to the player who tries to close it.
    // Console can override though
    Player* player = GetSession() ? GetSession()->GetPlayer() : NULL;
    if (player && ticket->IsAssignedNotTo(player->GetGUID()))
    {
        PSendSysMessage(LANG_COMMAND_TICKETCANNOTCLOSE, ticket->GetId());
        return true;
    }

    sTicketMgr->CloseTicket(ticket->GetId(), player ? player->GetGUID() : -1);
    sTicketMgr->UpdateLastChange();

    std::string msg = ticket->FormatMessageString(*this, player ? player->GetName() : "Console", NULL, NULL, NULL);
    sClientSessionMgr->SendGameMasterMessage(msg.c_str());

    // Inform player, who submitted this ticket, that it is closed
    if (Player* submitter = ticket->GetPlayer())
    {
        if (submitter->GetSession())
        {
            WorldPacket data(SMSG_GMTICKET_DELETETICKET, 4);
            data << uint32(GMTICKET_RESPONSE_TICKET_DELETED);
            submitter->GetSession()->SendPacket(&data);
        }
    }
    return true;
}

bool ChatHandler::HandleGMTicketCommentCommand(char const* args)
{
    if (!*args)
        return false;

    char* ticketIdStr = strtok((char*)args, " ");
    uint32 ticketId = atoi(ticketIdStr);

    char* comment = strtok(NULL, "\n");
    if (!comment)
        return false;

    GmTicket* ticket = sTicketMgr->GetTicket(ticketId);
    if (!ticket || ticket->IsClosed())
    {
        PSendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
        return true;
    }

    // Cannot comment ticket assigned to someone else
    //! Console excluded
    Player* player = GetSession() ? GetSession()->GetPlayer() : NULL;
    if (player && ticket->IsAssignedNotTo(player->GetGUID()))
    {
        PSendSysMessage(LANG_COMMAND_TICKETALREADYASSIGNED, ticket->GetId());
        return true;
    }

    SQLTransaction trans = SQLTransaction(NULL);
    ticket->SetComment(comment);
    ticket->SaveToDB(trans);
    sTicketMgr->UpdateLastChange();

    std::string msg = ticket->FormatMessageString(*this, NULL, ticket->GetAssignedToName().c_str(), NULL, NULL);
    msg += PGetParseString(LANG_COMMAND_TICKETLISTADDCOMMENT, player ? player->GetName() : "Console", comment);
    sClientSessionMgr->SendGameMasterMessage(msg.c_str());

    return true;
}

bool ChatHandler::HandleGMTicketListClosedCommand(char const* /*args*/)
{
    sTicketMgr->ShowClosedList(*this);
    return true;
}

bool ChatHandler::HandleGMTicketCompleteCommand(char const* args)
{
    if (!*args)
        return false;

    uint32 ticketId = atoi(args);
    GmTicket* ticket = sTicketMgr->GetTicket(ticketId);
    if (!ticket || ticket->IsClosed() || ticket->IsCompleted())
    {
        SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
        return true;
    }

    if (Player* player = ticket->GetPlayer())
        if (player->GetSession())
            ticket->SendResponse(player->GetSession());

    sTicketMgr->UpdateLastChange();
    return true;
}

bool ChatHandler::HandleGMTicketDeleteByIdCommand(char const* args)
{
    if (!*args)
        return false;

    uint32 ticketId = atoi(args);
    GmTicket* ticket = sTicketMgr->GetTicket(ticketId);
    if (!ticket)
    {
        SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
        return true;
    }

    if (!ticket->IsClosed())
    {
        SendSysMessage(LANG_COMMAND_TICKETCLOSEFIRST);
        return true;
    }

    std::string msg = ticket->FormatMessageString(*this, NULL, NULL, NULL, GetSession() ? GetSession()->GetPlayer()->GetName() : "Console");
    sClientSessionMgr->SendGameMasterMessage(msg.c_str());

    sTicketMgr->RemoveTicket(ticket->GetId());
    sTicketMgr->UpdateLastChange();

    if (Player* player = ticket->GetPlayer())
    {
        if (player->GetSession())
        {
            // Force abandon ticket
            WorldPacket data(SMSG_GMTICKET_DELETETICKET, 4);
            data << uint32(GMTICKET_RESPONSE_TICKET_DELETED);
            player->GetSession()->SendPacket(&data);
        }
    }

    return true;
}

bool ChatHandler::HandleGMTicketEscalateCommand(char const* args)
{
    if (!*args)
        return false;

    uint32 ticketId = atoi(args);
    GmTicket* ticket = sTicketMgr->GetTicket(ticketId);
    if (!ticket || ticket->IsClosed() || ticket->IsCompleted() || ticket->GetEscalatedStatus() != TICKET_UNASSIGNED)
    {
        SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
        return true;
    }

    ticket->SetEscalatedStatus(TICKET_IN_ESCALATION_QUEUE);

    if (Player* player = ticket->GetPlayer())
        if (player->GetSession())
            sTicketMgr->SendTicket(player->GetSession(), ticket);

    sTicketMgr->UpdateLastChange();
    return true;
}

bool ChatHandler::HandleGMTicketListEscalatedCommand(char const* /*args*/)
{
    sTicketMgr->ShowEscalatedList(*this);
    return true;
}

bool ChatHandler::HandleGMTicketListCommand(char const* /*args*/)
{
    sTicketMgr->ShowList(*this, false);
    return true;
}

bool ChatHandler::HandleGMTicketListOnlineCommand(char const* /*args*/)
{
    sTicketMgr->ShowList(*this, true);
    return true;
}

bool ChatHandler::HandleGMTicketResetCommand(char const* /*args*/)
{
    if (sTicketMgr->GetOpenTicketCount())
    {
        SendSysMessage(LANG_COMMAND_TICKETPENDING);
        return true;
    }
    else
    {
        sTicketMgr->ResetTickets();
        SendSysMessage(LANG_COMMAND_TICKETRESET);
    }

    return true;
}

bool ChatHandler::HandleToggleGMTicketSystem(char const* /*args*/)
{
    bool status = !sTicketMgr->GetStatus();
    sTicketMgr->SetStatus(status);
    PSendSysMessage(status ? LANG_ALLOW_TICKETS : LANG_DISALLOW_TICKETS);
    return true;
}

bool ChatHandler::HandleGMTicketUnAssignCommand(char const* args)
{
    if (!*args)
        return false;

    uint32 ticketId = atoi(args);
    GmTicket* ticket = sTicketMgr->GetTicket(ticketId);
    if (!ticket || ticket->IsClosed())
    {
        SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
        return true;
    }
    // Ticket must be assigned
    if (!ticket->IsAssigned())
    {
        PSendSysMessage(LANG_COMMAND_TICKETNOTASSIGNED, ticket->GetId());
        return true;
    }

    // Get security level of player, whom this ticket is assigned to
    uint32 security = SEC_PLAYER;
    Player* assignedPlayer = ticket->GetAssignedPlayer();
    if (assignedPlayer && assignedPlayer->GetSession())
        security = assignedPlayer->GetSession()->GetSecurity();
    else
    {
        uint64 guid = ticket->GetAssignedToGUID();
        uint32 accountId = sObjectMgr->GetPlayerAccountIdByGUID(guid);
        security = AccountMgr::GetSecurity(accountId, realmID);
    }

    // Check security
    //! If no m_session present it means we're issuing this command from the console
    uint32 mySecurity = GetSession() ? GetSession()->GetSecurity() : SEC_CONSOLE;
    if (security > mySecurity)
    {
        SendSysMessage(LANG_COMMAND_TICKETUNASSIGNSECURITY);
        return true;
    }

    std::string assignedTo = ticket->GetAssignedToName(); // copy assignedto name because we need it after the ticket has been unnassigned
    SQLTransaction trans = SQLTransaction(NULL);
    ticket->SetUnassigned();
    ticket->SaveToDB(trans);
    sTicketMgr->UpdateLastChange();

    std::string msg = ticket->FormatMessageString(*this, NULL, assignedTo.c_str(),
                                                  GetSession() ? GetSession()->GetPlayer()->GetName() : "Console", NULL);
    sClientSessionMgr->SendGameMasterMessage(msg.c_str());

    return true;
}

bool ChatHandler::HandleGMTicketGetByIdCommand(char const* args)
{
    if (!*args)
        return false;

    uint32 ticketId = atoi(args);
    GmTicket* ticket = sTicketMgr->GetTicket(ticketId);
    if (!ticket || ticket->IsClosed() || ticket->IsCompleted())
    {
        SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
        return true;
    }

    SQLTransaction trans = SQLTransaction(NULL);
    ticket->SetViewed();
    ticket->SaveToDB(trans);

    SendSysMessage(ticket->FormatMessageString(*this, true).c_str());
    return true;
}

bool ChatHandler::HandleGMTicketGetByNameCommand(char const* args)
{
    if (!*args)
        return false;

    std::string name(args);
    if (!normalizePlayerName(name))
        return false;

    // Detect target's GUID
    uint64 guid = 0;
    if (Player* player = sObjectMgr->FindPlayerByName(name.c_str()))
        guid = player->GetGUID();
    else
        guid = sObjectMgr->GetPlayerGUIDByName(name);

    // Target must exist
    if (!guid)
    {
        SendSysMessage(LANG_NO_PLAYERS_FOUND);
        return true;
    }

    // Ticket must exist
    GmTicket* ticket = sTicketMgr->GetTicketByPlayer(guid);
    if (!ticket)
    {
        SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
        return true;
    }

    SQLTransaction trans = SQLTransaction(NULL);
    ticket->SetViewed();
    ticket->SaveToDB(trans);

    SendSysMessage(ticket->FormatMessageString(*this, true).c_str());
    return true;
}

bool _HandleGMTicketResponseAppendCommand(char const* args, bool newLine, ChatHandler* handler)
{
    if (!*args)
        return false;

    char* ticketIdStr = strtok((char*)args, " ");
    uint32 ticketId = atoi(ticketIdStr);

    char* response = strtok(NULL, "\n");
    if (!response)
        return false;

    GmTicket* ticket = sTicketMgr->GetTicket(ticketId);
    if (!ticket || ticket->IsClosed())
    {
        handler->PSendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
        return true;
    }

    // Cannot add response to ticket, assigned to someone else
    //! Console excluded
    Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : NULL;
    if (player && ticket->IsAssignedNotTo(player->GetGUID()))
    {
        handler->PSendSysMessage(LANG_COMMAND_TICKETALREADYASSIGNED, ticket->GetId());
        return true;
    }

    SQLTransaction trans = SQLTransaction(NULL);
    ticket->AppendResponse(response);
    if (newLine)
        ticket->AppendResponse("\n");
    ticket->SaveToDB(trans);

    return true;
}

bool ChatHandler::HandleGMTicketResponseAppendCommand(char const* args)
{
    return _HandleGMTicketResponseAppendCommand(args, false, this);
}

bool ChatHandler::HandleGMTicketResponseAppendLnCommand(char const* args)
{
    return _HandleGMTicketResponseAppendCommand(args, true, this);
}