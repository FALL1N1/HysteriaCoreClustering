/**
 * @file
 * @author TehKevin for Laenalith-WoW
 * @date 08.03.2015
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

#include "GMQualityManager.h"
#include "TicketMgr.h"
#include "AccountMgr.h"

typedef GMQualityManager GMQ;


GMQualityItem::GMQualityItem(uint32 ticket, uint32 gmAccountId, uint32 playerAccountId)
    : ticketId(ticket), gmAccId(gmAccountId), playerAccId(playerAccountId)
{
    gmAccName = getAccountNameById(gmAccountId);
    securityLevel = getAccountSecurityById(gmAccountId);
    playerAccName = getAccountNameById(playerAccountId);
}

void GMQualityItem::AddGMText(ChatMsg type, std::string text, std::string receiver)
{
    gmText << "[" << type << "][" << receiver << "]" << text.c_str() << ";";
    sLog->outError(gmText.str().c_str());
}

void GMQualityItem::AddPlayerText(ChatMsg type, std::string text, std::string receiver)
{
    playerText << "[" << type << "][" << receiver << "]" << text.c_str() << ";";
    sLog->outError(playerText.str().c_str());
}

void GMQualityItem::SaveToDB(uint32 ticket, GmTicket* pGmTicket)
{
 


}

std::string GMQualityItem::getAccountNameById(uint32 accId)
{
    std::string name;
    if (AccountMgr::GetName(accId, name))
        return name;
    return "Unknown";
}

uint8 GMQualityItem::getAccountSecurityById(uint32 accId)
{
    return AccountMgr::GetSecurity(accId);
}


GMQ::GMQualityManager()
    : lastTicketId(0)
{
}

GMQ::~GMQualityManager()
{
    if (!itemStorage.empty())
        for (GMQualitySet::iterator itr = itemStorage.begin(); itr != itemStorage.end(); ++itr)
            if (GMQualityItem* pItem = (*itr))
                delete pItem;
}

void GMQ::Load()
{


}

void GMQ::OnTicketAssign(GmTicket* pGmTicket)
{
    RETURN_IF_NOT_ENABLED

    if (!pGmTicket)
        return;

    GMQualityItem* newItem = new GMQualityItem(pGmTicket->GetId(), sObjectMgr->GetPlayerAccountIdByGUID(pGmTicket->GetAssignedToGUID()),
                                               sObjectMgr->GetPlayerAccountIdByGUID(pGmTicket->GetPlayerGUID()));

    if (newItem)
        itemStorage.insert(newItem);
    else
        delete newItem;
}

void GMQ::OnTicketUnAssign(GmTicket* pGmTicket)
{
    RETURN_IF_NOT_ENABLED

    if (!pGmTicket)
        return;

    GMQualitySet::iterator itr = findGMQualityItrByTicketId(pGmTicket->GetId());
    if (itr != itemStorage.end())
        deleteTicketAndSaveToDB(itr, pGmTicket);
}

void GMQ::OnTicketClose(GmTicket* pGmTicket)
{
    RETURN_IF_NOT_ENABLED

    if (!pGmTicket)
        return;

    GMQualitySet::iterator itr = findGMQualityItrByTicketId(pGmTicket->GetId());
        if (itr != itemStorage.end())
            deleteTicketAndSaveToDB(itr, pGmTicket);
}

void GMQ::OnTicketDelete(GmTicket* pGmTicket)
{
    RETURN_IF_NOT_ENABLED

    if (!pGmTicket)
        return;
}

void GMQ::OnGMChat(ChatMsg type, Player* pGameMaster, std::string text, std::string receiver)
{
    RETURN_IF_NOT_ENABLED;

    bool outGMChat = false;

    if (receiver.compare("NoTarget") != 0)
    {
        if (Player* pPlayer = sObjectMgr->FindPlayerByName(receiver.c_str()))
        {
            outGMChatLogDB(type, pGameMaster, text, pPlayer->GetName());
            outGMChat = true;

            if (GMQualityItem* pItem = findGMQualityItemByGMAndPlayer(pGameMaster->GetSession()->GetAccountId(), pPlayer->GetSession()->GetAccountId()))
            {
                pItem->AddGMText(type, text, receiver);
                return;
            }
        }
    }

    if (GMQualityItem* pItem = findGMQualityItemByGMOrPlayer(pGameMaster->GetSession()->GetAccountId()))
        pItem->AddGMText(type, text, receiver);

    if (!outGMChat)
        outGMChatLogDB(type, pGameMaster, text, receiver);
}

void GMQ::OnPlayerChat(ChatMsg type, Player* pPlayer, std::string text, std::string receiver)
{
    RETURN_IF_NOT_ENABLED

    if (receiver.compare("NoTarget") != 0)
    {
        if (Player* pGameMaster = sObjectMgr->FindPlayerByName(receiver.c_str()))
        {
            if (AccountMgr::IsVIPorPlayer(pGameMaster->GetSession()->GetSecurity()))
                return;

            // Player chat has negative gm_account_guid
            outGMChatLogDB(type, (0 - pGameMaster->GetSession()->GetAccountId()), text, pPlayer->GetName());

            if (GMQualityItem* pItem = findGMQualityItemByGMAndPlayer(pGameMaster->GetSession()->GetAccountId(), pPlayer->GetSession()->GetAccountId()))
            {
                pItem->AddPlayerText(type, text, receiver);
                return;
            }
        }
    }

    if (GMQualityItem* pItem = findGMQualityItemByGMOrPlayer(pPlayer->GetSession()->GetAccountId()))
        pItem->AddPlayerText(type, text, receiver);
}

bool GMQ::isEnabled()
{
    return sLogon->getBoolConfig(CONFIG_CHATLOG_GM_ACTIVITY);
}

GMQ::GMQualitySet::iterator GMQ::findGMQualityItrByTicketId(uint32 ticketId)
{
    if (!itemStorage.empty())
        for (GMQualitySet::iterator itr = itemStorage.begin(); itr != itemStorage.end(); ++itr)
            if ((*itr)->GetTicketId() == ticketId)
                return itr;
    return itemStorage.end();
}

GMQ::GMQualitySet::iterator GMQ::findGMQualityItrByGMAndPlayer(uint32 gmAccId, uint32 playerAccId)
{
    if (!itemStorage.empty())
        for (GMQualitySet::iterator itr = itemStorage.begin(); itr != itemStorage.end(); ++itr)
            if (GMQualityItem* item = (*itr))
                if (item->GetGMAccountId() == gmAccId && item->GetPlayerAccountId() == playerAccId)
                    return itr;
    return itemStorage.end();
}

GMQ::GMQualitySet::iterator GMQ::findGMQualityItrByGMOrPlayer(uint32 accId)
{
    if (!itemStorage.empty())
        for (GMQualitySet::iterator itr = itemStorage.begin(); itr != itemStorage.end(); ++itr)
            if (GMQualityItem* item = (*itr))
                if (item->GetGMAccountId() == accId || item->GetPlayerAccountId() == accId)
                    return itr;
    return itemStorage.end();
}

GMQualityItem* GMQ::findGMQualityItemByTicketId(uint32 ticketId)
{
    GMQualitySet::iterator itr = findGMQualityItrByTicketId(ticketId);
    if (itr != itemStorage.end())
        return (*itr);
    return NULL;
}

GMQualityItem* GMQ::findGMQualityItemByGMAndPlayer(uint32 gmAccId, uint32 playerAccId)
{
    GMQualitySet::iterator itr = findGMQualityItrByGMAndPlayer(gmAccId, playerAccId);
    if (itr != itemStorage.end())
        return (*itr);
    return NULL;
}

GMQualityItem* GMQ::findGMQualityItemByGMOrPlayer(uint32 accId)
{
    GMQualitySet::iterator itr = findGMQualityItrByGMOrPlayer(accId);
    if (itr != itemStorage.end())
        return (*itr);
    return NULL;
}

void GMQ::deleteTicketAndSaveToDB(GMQualitySet::iterator itr, GmTicket* pGmTicket)
{
    if (GMQualityItem* pItem = (*itr))
    {
        pItem->SaveToDB(++lastTicketId, pGmTicket);

        delete pItem;
        itemStorage.erase(itr);
    }
}

void GMQ::outGMChatLogDB(ChatMsg type, Player* pPlayer, std::string text, std::string receiver)
{
    outGMChatLogDB(type, pPlayer->GetSession()->GetAccountId(), text, receiver);
}

void GMQ::outGMChatLogDB(ChatMsg type, int32 accId, std::string text, std::string receiver)
{


}