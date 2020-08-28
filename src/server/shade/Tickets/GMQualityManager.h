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

#ifndef _GM_QUALITY_MANAGER_H
#define _GM_QUALITY_MANAGER_H

class GmTicket;


#define RETURN_IF_NOT_ENABLED if (!isEnabled()) return;

class GMQualityItem
{
    public:

        GMQualityItem(uint32 ticket, uint32 gmAccountId, uint32 playerAccountId);

        void AddGMText(ChatMsg type, std::string text, std::string receiver);
        void AddPlayerText(ChatMsg type, std::string text, std::string receiver);

        uint32 GetTicketId() const { return ticketId; }
        uint32 GetGMAccountId() const { return gmAccId; }
        uint32 GetPlayerAccountId() const { return playerAccId; }

        void SaveToDB(uint32 ticket, GmTicket* pGmTicket);

    private:

        std::string getAccountNameById(uint32 accId);
        uint8 getAccountSecurityById(uint32 accId);

        uint32 ticketId;
        uint32 gmAccId;
        std::string gmAccName;
        uint8 securityLevel;
        uint32 playerAccId;
        std::string playerAccName;
        std::stringstream gmText;
        std::stringstream playerText;
};


class GMQualityManager
{
    public:

        typedef std::set<GMQualityItem*> GMQualitySet;

        GMQualityManager();
        ~GMQualityManager();

        void Load();

        void OnTicketAssign(GmTicket* pGmTicket);
        void OnTicketUnAssign(GmTicket* pGmTicket);
        void OnTicketClose(GmTicket* pGmTicket);
        void OnTicketDelete(GmTicket* pGmTicket);

        void OnGMChat(ChatMsg type, Player* pGameMaster, std::string text, std::string receiver = "NoTarget");
        void OnPlayerChat(ChatMsg type, Player* pPlayer, std::string text, std::string receiver = "NoTarget");

    private:

        bool isEnabled();

        GMQualitySet::iterator findGMQualityItrByTicketId(uint32 ticketId);
        GMQualitySet::iterator findGMQualityItrByGMAndPlayer(uint32 gmAccId, uint32 playerAccId);
        GMQualitySet::iterator findGMQualityItrByGMOrPlayer(uint32 accId);

        GMQualityItem* findGMQualityItemByTicketId(uint32 ticketId);
        GMQualityItem* findGMQualityItemByGMAndPlayer(uint32 gmAccId, uint32 playerAccId);
        GMQualityItem* findGMQualityItemByGMOrPlayer(uint32 accId);

        void deleteTicketAndSaveToDB(GMQualitySet::iterator itr, GmTicket* pGmTicket);
        void outGMChatLogDB(ChatMsg type, Player* pPlayer, std::string text, std::string receiver = "NoTarget");
        void outGMChatLogDB(ChatMsg type, int32 accId, std::string text, std::string receiver = "NoTarget");

        uint32 lastTicketId;
        GMQualitySet itemStorage;
};

#define sGMQualityManager ACE_Singleton<GMQualityManager, ACE_Null_Mutex>::instance()


#endif // _GM_QUALITY_MANAGER_H