#include "Common.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "DatabaseRoutines.h"
#include "ClientSessionMgr.h"
#include "Proxy.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "ControlSessionMgr.h"
#include "ClusterDefines.h"
#include "ObjectMgr.h"
#include "RoutingHelper.h"
#include "ProgressBar.h"

// NIX ANFASSEN, ES SEI DENN DU WEIßT WAS DU TUST (UND DAS TUST DU NICHT!)

class item_instance : public ACE_Based::Runnable
{
    public:
        void run(void)
        {
            sRebase->ItemInstance();
        }
};

class character_inventory_thread : public ACE_Based::Runnable
{
    public:
        void run(void)
        {
            sRebase->CharacterInventory();
        }
};

void RebaseClass::InitAutomaticRebase()
{
    time_t wstime = time_t(sLogon->getLogonState(LOGON_REBASE_STATE));
    time_t curtime = time(NULL);
    if (wstime < curtime)
    {
        m_NextRebaseTimePlus = curtime + 180;
        m_NextRebaseTime = curtime; 
    }
    else
    {
        m_NextRebaseTimePlus = wstime + 180;
        m_NextRebaseTime = wstime; 
    }

    count_down = 0;
}

void RebaseClass::UpdateTimers(uint32 diff)
{
    //Send an announce and set the timer for 12h
    if (time(NULL) > m_NextRebaseTime - 43200)
    {
        if (sLogon->IsHalting())
            return;
        time_t temp = (m_NextRebaseTime - time(NULL));
        if (temp <= 1)
            sLogon->HaltServ(0, SHUTDOWN_MASK_HALT);
        else
            sLogon->HaltServ((m_NextRebaseTime - time(NULL)), SHUTDOWN_MASK_HALT);
    }

    //begin pre-phase
	if (time(NULL) > m_NextRebaseTime)
	{
        m_NextRebaseTime = m_NextRebaseTime + SEC_IN_DAY * sLogon->getIntConfig(CONFIG_REBASE_INTERVAL);
		sLogon->setLogonState(LOGON_REBASE_STATE, uint64(m_NextRebaseTime));

        //Set nodes offline
        WorldPacket packet(LOGON_SYNC_DATA);
        packet << uint32(CL_DEF_SHUTDOWN);
        packet << uint32(1);
        sControlSessionMgr->SendGlobal(&packet);
	}

    if (time(NULL) > m_NextRebaseTimePlus)
	{
        m_NextRebaseTimePlus = m_NextRebaseTime + 180; //+3 mins
        sRebase->Begin();
	}
}

void RebaseClass::Begin()
{
    time_t TimeStamp;
    tm *now;

    TimeStamp = time(0);
    now = localtime(&TimeStamp);
    sLog->outString("Rebase Starts NOW %u:%u:%u", now->tm_hour, now->tm_min, now->tm_sec);

    //Init the pre-sequence
	sLog->outString("Rebase->Pre-Sequence");
    Init();

	sLog->outString("Rebase->Collect_GUIDs");

    QueryResult result = CharacterDatabase.Query("SELECT guid FROM temp_item_instance");
    if (!result)
    {
        sLog->outString(">> Loaded 0 item_instance GUIDs.");
        sLog->outString();
        return;
    }
    barGoLink bar(result->GetRowCount());
    uint32 count = 0;
    do
    {
        bar.step();
        ++count;
        Field* fields = result->Fetch();
        _ItemMap[fields[0].GetUInt32()] = count;
    }while (result->NextRow());
    sLog->outString(">> Loaded %u GUIDs.", count);

    sLog->outString("Rebase->ItemInstance-Thread");
    ACE_Based::Thread t_item_instance(new item_instance);

    sLog->outString("Rebase->CharacterInventory");
	ACE_Based::Thread t_char_inventory(new character_inventory_thread);

    sLog->outString("Rebase->Auctionhouse");
    Auctionhouse();
    sLog->outString("Rebase->CharacterGifts");
    CharacterGifts();
    sLog->outString("Rebase->GuildBankItem");
    GuildBankItem();
    sLog->outString("Rebase->ItemRefundInstance");
    ItemRefundInstance();
	sLog->outString("Rebase->ItemSoulboundTradeData");    
    ItemSoulboundTradeData();
	sLog->outString("Rebase->MailItems");
    MailItems();
    sLog->outString("Rebase->Petition");
    Petition();
    sLog->outString("Rebase->PetitionSign");
    PetitionSign();

    //Wait till items are rebased
    t_item_instance.wait();
    t_char_inventory.wait();
    
    //PastSequence
    Cleanup();   
    
    //Restart the nodes
    WorldPacket packet(LOGON_SYNC_DATA);
    packet << uint32(CL_DEF_SHUTDOWN);
    packet << uint32(1);
    sControlSessionMgr->SendGlobal(&packet);
    
    //Clean the GUIDs
    sLog->outString("ReCalculating highest guids...");
    sRoutingHelper->ResetLastGUID();
    sObjectMgr->SetHighestGuids();

    TimeStamp = time(0);
    now = localtime(&TimeStamp);
	sLog->outString("Rebase Finished %u:%u:%u", now->tm_hour, now->tm_min, now->tm_sec);

    Logon::ContinueNow(); 

    //Set the realm online
    LoginDatabase.DirectPExecute("UPDATE realmlist SET flag = flag & 1, population = 0 WHERE id = '%u'", realmID);
}

void RebaseClass::Init()
{
    CharacterDatabase.DirectPExecute("CREATE TABLE temp_auctionhouse SELECT * FROM auctionhouse");
    CharacterDatabase.DirectPExecute("CREATE TABLE temp_character_gifts SELECT * FROM character_gifts");
    CharacterDatabase.DirectPExecute("CREATE TABLE temp_guild_bank_item SELECT * FROM guild_bank_item");
    CharacterDatabase.DirectPExecute("CREATE TABLE temp_item_refund_instance SELECT * FROM item_refund_instance");
    CharacterDatabase.DirectPExecute("CREATE TABLE temp_item_soulbound_trade_data SELECT * FROM item_soulbound_trade_data");
    CharacterDatabase.DirectPExecute("CREATE TABLE temp_mail_items SELECT * FROM mail_items");
    CharacterDatabase.DirectPExecute("CREATE TABLE temp_petition SELECT * FROM petition");
    CharacterDatabase.DirectPExecute("CREATE TABLE temp_petition_sign SELECT * FROM petition_sign");
    CharacterDatabase.DirectPExecute("CREATE TABLE temp_character_inventory SELECT * FROM character_inventory");   
    CharacterDatabase.DirectPExecute("CREATE TABLE temp_item_instance SELECT * FROM item_instance");

    //Empty Tables
    CharacterDatabase.DirectExecute("TRUNCATE auctionhouse");
    CharacterDatabase.DirectExecute("TRUNCATE character_gifts");
    CharacterDatabase.DirectExecute("TRUNCATE character_inventory");
    CharacterDatabase.DirectExecute("TRUNCATE guild_bank_item");
    CharacterDatabase.DirectExecute("TRUNCATE item_instance");
    CharacterDatabase.DirectExecute("TRUNCATE item_refund_instance");
    CharacterDatabase.DirectExecute("TRUNCATE item_soulbound_trade_data");
    CharacterDatabase.DirectExecute("TRUNCATE mail_items");
    CharacterDatabase.DirectExecute("TRUNCATE petition");
    CharacterDatabase.DirectExecute("TRUNCATE petition_sign");
}

void RebaseClass::Cleanup()
{
    CharacterDatabase.PExecute("DROP TABLE IF EXISTS temp_auctionhouse");
    CharacterDatabase.PExecute("DROP TABLE IF EXISTS temp_character_gifts");
    CharacterDatabase.PExecute("DROP TABLE IF EXISTS temp_character_inventory");
    CharacterDatabase.PExecute("DROP TABLE IF EXISTS temp_guild_bank_item");
    CharacterDatabase.PExecute("DROP TABLE IF EXISTS temp_item_instance");
    CharacterDatabase.PExecute("DROP TABLE IF EXISTS temp_item_refund_instance");
    CharacterDatabase.PExecute("DROP TABLE IF EXISTS temp_item_soulbound_trade_data");
    CharacterDatabase.PExecute("DROP TABLE IF EXISTS temp_mail_items");
    CharacterDatabase.PExecute("DROP TABLE IF EXISTS temp_petition");
    CharacterDatabase.PExecute("DROP TABLE IF EXISTS temp_petition_sign");

}

//The main rebase
void RebaseClass::Auctionhouse()
{
    uint64 new_guid;
    uint32 counter = 0;
    std::ostringstream ss;

    QueryResult result = CharacterDatabase.Query("SELECT * FROM temp_auctionhouse");
    if (!result)
    {
        sLog->outString();
        sLog->outString(">> no temp_auctionhouse found!");
        return;
    }
    do
    {
        Field* fields = result->Fetch();
        ItemInstanceMap::const_iterator itr = _ItemMap.find(fields[2].GetUInt32());
        
        if (itr == _ItemMap.end())
            continue;

        new_guid = itr->second;

        if (counter == 0)
            ss << "INSERT IGNORE INTO auctionhouse VALUES" << SingleGUID(2, new_guid, fields, result);
        else
            ss << "," << SingleGUID(2, new_guid, fields, result) << "\n";
            
        counter ++;

        if (counter == WRAPS / 4)
        {
            CharacterDatabase.PExecute(ss.str().c_str());
            counter = 0;
            ss.str("");
        }
    }
    while (result->NextRow());
    CharacterDatabase.PExecute(ss.str().c_str()); 
}

void RebaseClass::CharacterGifts()
{
    uint64 new_guid;
    uint32 counter = 0;
    std::ostringstream ss;

    QueryResult result = CharacterDatabase.Query("SELECT * FROM temp_character_gifts");
    if (!result)
    {
        sLog->outString();
        sLog->outString(">> no temp_character_gifts found!");
        return;
    }
    do
    {
        Field* fields = result->Fetch();
        ItemInstanceMap::const_iterator itr = _ItemMap.find(fields[1].GetUInt32());
        
        if (itr == _ItemMap.end())
            continue;

        new_guid = itr->second;

        if (counter == 0)
            ss << "INSERT IGNORE INTO character_gifts VALUES" << SingleGUID(1, new_guid, fields, result);
        else
            ss << "," << SingleGUID(1, new_guid, fields, result) << "\n";
            
        counter ++;

        if (counter == WRAPS / 2)
        {
            CharacterDatabase.PExecute(ss.str().c_str());
            counter = 0;
            ss.str("");
        }
    }
    while (result->NextRow());
    CharacterDatabase.PExecute(ss.str().c_str()); 
}

void RebaseClass::CharacterInventory()
{
    uint32 max_loops = 0;
    uint32 min_loops = 0;
    uint32 add = 500000;
    uint32 counter = 0;
    std::ostringstream ss;
    bool bag;

    //Count the Rows
    QueryResult result = CharacterDatabase.Query("SELECT COUNT(*) FROM temp_character_inventory");
    if (!result)
        return;
    
    Field* fields = result->Fetch();
    max_loops = fields[0].GetUInt32();  

    for (min_loops = 0; min_loops < max_loops; min_loops += add)
    {
        QueryResult result;
        if (min_loops + add < max_loops)
            result = CharacterDatabase.PQuery("SELECT * FROM temp_character_inventory LIMIT %u, 500000", min_loops);
        else
            result = CharacterDatabase.PQuery("SELECT * FROM temp_character_inventory LIMIT %u, 18446744073709551615", min_loops);

        if (!result)         
            continue;

        do
        {
            Field* fields = result->Fetch();

            //Field 1 !=0 we have a bag
            if (fields[1].GetUInt32() == 0)
                bag = false;
            else 
                bag = true;

            if (bag)
            {
                ItemInstanceMap::const_iterator itr = _ItemMap.find(fields[3].GetUInt32());
                ItemInstanceMap::const_iterator iter_two = _ItemMap.find(fields[1].GetUInt32());
                if (itr == _ItemMap.end() || iter_two == _ItemMap.end())
                    continue;

                if (counter == 0)
                    ss << "INSERT IGNORE INTO character_inventory VALUES" << DoubleGUID(1, iter_two->second, 3, itr->second, fields, result);
                else
                    ss << "," << DoubleGUID(1, iter_two->second, 3, itr->second, fields, result);
            }
            else
            {
                ItemInstanceMap::const_iterator itr = _ItemMap.find(fields[3].GetUInt32());
                if (itr == _ItemMap.end())
                    continue;

                if (counter == 0)
                    ss << "INSERT IGNORE INTO character_inventory VALUES" << SingleGUID(3, itr->second, fields, result);
                else
                    ss << "," << SingleGUID(3, itr->second, fields, result);
            }        
            counter ++;

            if (counter == 1000)
            {
                CharacterDatabase.DirectExecute(ss.str().c_str());
                counter = 0;
                ss.str("");
            }


        }while (result->NextRow());

        if (counter != 0)
        {
            CharacterDatabase.DirectExecute(ss.str().c_str());
            counter = 0;
            ss.str("");
        }
    }   
}

void RebaseClass::GuildBankItem()
{
    uint64 new_guid;
    uint32 counter = 0;
    std::ostringstream ss;

    QueryResult result = CharacterDatabase.Query("SELECT * FROM temp_guild_bank_item");
    if (!result)
    {
        sLog->outString();
        sLog->outString(">> no temp_guild_bank_item found!");
        return;
    }
    do
    {
        Field* fields = result->Fetch();
        ItemInstanceMap::const_iterator itr = _ItemMap.find(fields[3].GetUInt32());
        
        if (itr == _ItemMap.end())
            continue;

        new_guid = itr->second;

        if (counter == 0)
            ss << "INSERT IGNORE INTO guild_bank_item VALUES" << SingleGUID(3, new_guid, fields, result);
        else
            ss << "," << SingleGUID(3, new_guid, fields, result) << "\n";
            
        counter ++;

        if (counter == WRAPS / 2)
        {
            CharacterDatabase.PExecute(ss.str().c_str());
            counter = 0;
            ss.str("");
        }
    }
    while (result->NextRow());
    CharacterDatabase.PExecute(ss.str().c_str()); 
}

void RebaseClass::ItemRefundInstance()
{
    uint64 new_guid;
    uint32 counter = 0;
    std::ostringstream ss;

    QueryResult result = CharacterDatabase.Query("SELECT * FROM temp_item_refund_instance");
    if (!result)
    {
        sLog->outString();
        sLog->outString(">> no temp_item_refund_instance found!");
        return;
    }
    do
    {
        Field* fields = result->Fetch();
        ItemInstanceMap::const_iterator itr = _ItemMap.find(fields[0].GetUInt32());
        
        if (itr == _ItemMap.end())
            continue;

        new_guid = itr->second;

        if (counter == 0)
            ss << "INSERT IGNORE INTO item_refund_instance VALUES" << SingleGUID(0, new_guid, fields, result);
        else
            ss << "," << SingleGUID(0, new_guid, fields, result) << "\n";
            
        counter ++;

        if (counter == WRAPS / 10)
        {
            CharacterDatabase.PExecute(ss.str().c_str());
            counter = 0;
            ss.str("");
        }
    }
    while (result->NextRow());
    CharacterDatabase.PExecute(ss.str().c_str()); 
}

void RebaseClass::ItemSoulboundTradeData()
{
    uint64 new_guid;
    uint32 counter = 0;
    std::ostringstream ss;

    QueryResult result = CharacterDatabase.Query("SELECT * FROM temp_item_soulbound_trade_data");
    if (!result)
    {
        sLog->outString();
        sLog->outString(">> no temp_item_soulbound_trade_data found!");
        return;
    }
    do
    {
        Field* fields = result->Fetch();
        ItemInstanceMap::const_iterator itr = _ItemMap.find(fields[0].GetUInt32());
        
        if (itr == _ItemMap.end())
            continue;

        new_guid = itr->second;

        if (counter == 0)
            ss << "INSERT IGNORE INTO item_soulbound_trade_data VALUES" << SingleGUID(0, new_guid, fields, result);
        else
            ss << "," << SingleGUID(0, new_guid, fields, result) << "\n";
            
        counter ++;

        if (counter == WRAPS / 10)
        {
            CharacterDatabase.PExecute(ss.str().c_str());
            counter = 0;
            ss.str("");
        }
    }
    while (result->NextRow());
    CharacterDatabase.PExecute(ss.str().c_str()); 
}

void RebaseClass::MailItems()
{
    uint64 new_guid;
    uint32 counter = 0;
    std::ostringstream ss;

    QueryResult result = CharacterDatabase.Query("SELECT * FROM temp_mail_items");
    if (!result)
    {
        sLog->outString();
        sLog->outString(">> no temp_mail_items found!");
        return;
    }
    do
    {
        Field* fields = result->Fetch();
        ItemInstanceMap::const_iterator itr = _ItemMap.find(fields[1].GetUInt32());
        
        if (itr == _ItemMap.end())
            continue;

        new_guid = itr->second;

        if (counter == 0)
            ss << "INSERT IGNORE INTO mail_items VALUES" << SingleGUID(1, new_guid, fields, result);
        else
            ss << "," << SingleGUID(1, new_guid, fields, result) << "\n";
            
        counter ++;

        if (counter == WRAPS / 2)
        {
            CharacterDatabase.PExecute(ss.str().c_str());
            counter = 0;
            ss.str("");
        }
    }
    while (result->NextRow());
    CharacterDatabase.PExecute(ss.str().c_str()); 
}

void RebaseClass::ItemInstance()
{
    uint32 max_loops = 0;
    uint32 min_loops = 0;
    uint32 add = 500000;
    uint32 counter = 0;
    std::ostringstream ss;

    //Count the Rows
    QueryResult result = CharacterDatabase.Query("SELECT COUNT(*) FROM temp_item_instance");
    if (!result)
        return;
    
    Field* fields = result->Fetch();
    max_loops = fields[0].GetUInt32();  

    for (min_loops = 0; min_loops < max_loops; min_loops += add)
    {
        QueryResult result;
        if (min_loops + add < max_loops)
            result = CharacterDatabase.PQuery("SELECT * FROM temp_item_instance LIMIT %u, 500000", min_loops);
        else
            result = CharacterDatabase.PQuery("SELECT * FROM temp_item_instance LIMIT %u, 18446744073709551615", min_loops);

        if (!result)         
            continue;

        do
        {
            Field* fields = result->Fetch();
            ItemInstanceMap::const_iterator itr = _ItemMap.find(fields[0].GetUInt32());
            
            if (itr == _ItemMap.end())
                continue;

            uint64 new_guid = itr->second;
            if (counter == 0)
                ss << "INSERT IGNORE INTO item_instance VALUES" << SingleGUID(0, new_guid, fields, result);
            else
                ss << "," << SingleGUID(0, new_guid, fields, result) << "\n";
            
            counter ++;

            if (counter == 5000)
            {
                CharacterDatabase.DirectExecute(ss.str().c_str());
                counter = 0;
                ss.str("");
            }

        }while (result->NextRow());

        if (counter != 0)
        {
            CharacterDatabase.DirectExecute(ss.str().c_str());
            counter = 0;
            ss.str("");
        }
    }
}

void RebaseClass::Petition()
{
    uint64 new_guid;
    uint32 counter = 0;
    std::ostringstream ss;

    QueryResult result = CharacterDatabase.Query("SELECT * FROM temp_petition");
    if (!result)
    {
        sLog->outString();
        sLog->outString(">> no temp_petition found!");
        return;
    }
    do
    {
        Field* fields = result->Fetch();
        ItemInstanceMap::const_iterator itr = _ItemMap.find(fields[1].GetUInt32());
        
        if (itr == _ItemMap.end())
            continue;

        new_guid = itr->second;

        if (counter == 0)
            ss << "INSERT IGNORE INTO petition VALUES" << SingleGUID(1, new_guid, fields, result);
        else
            ss << "," << SingleGUID(1, new_guid, fields, result) << "\n";
            
        counter ++;

        if (counter == WRAPS)
        {
            CharacterDatabase.PExecute(ss.str().c_str());
            counter = 0;
            ss.str("");
        }
    }
    while (result->NextRow());
    CharacterDatabase.PExecute(ss.str().c_str()); 
}

void RebaseClass::PetitionSign()
{
    uint64 new_guid;
    uint32 counter = 0;
    std::ostringstream ss;

    QueryResult result = CharacterDatabase.Query("SELECT * FROM temp_petition_sign");
    if (!result)
    {
        sLog->outString();
        sLog->outString(">> no temp_petition_sign found!");
        return;
    }
    do
    {
        Field* fields = result->Fetch();
        ItemInstanceMap::const_iterator itr = _ItemMap.find(fields[1].GetUInt32());
        
        if (itr == _ItemMap.end())
            continue;

        new_guid = itr->second;

        if (counter == 0)
            ss << "INSERT IGNORE INTO petition_sign VALUES" << SingleGUID(1, new_guid, fields, result);
        else
            ss << "," << SingleGUID(1, new_guid, fields, result) << "\n";
            
        counter ++;

        if (counter == WRAPS)
        {
            CharacterDatabase.PExecute(ss.str().c_str());
            counter = 0;
            ss.str("");
        }
    }
    while (result->NextRow());
    CharacterDatabase.PExecute(ss.str().c_str()); 
}

//Dirty little helpers
std::string RebaseClass::SingleGUID(uint64 offset, uint64 value, Field* fields, QueryResult result)
{
    std::ostringstream ss;
    ss << " (";
    for (uint32 i = 0; i < result->GetFieldCount(); ++i)
    {
        std::string s = fields[i].GetString();
        CharacterDatabase.EscapeString(s);

        if (i == offset)
            ss << "\'" << value << "\'";
        else if (s == "")
        {
            if (fields[i]._IsNumeric())
                ss << "null";
            else
                ss << "\'\'";
        }
        else
            ss << "\'" << s << "\'";

        if (i < result->GetFieldCount()-1)
            ss << ", ";
    }
    ss << ")";

    return ss.str();
}

std::string RebaseClass::DoubleGUID(uint64 offset, uint64 value, uint64 offsetb, uint64 valueb, Field* fields, QueryResult result)
{
    std::ostringstream ss;
    ss << " (";
    for (uint32 i = 0; i < result->GetFieldCount(); ++i)
    {
        std::string s = fields[i].GetString();
        CharacterDatabase.EscapeString(s);

        if (i == offset)
            ss << "\'" << value << "\'";
        else if (i == offsetb)
            ss << "\'" << valueb << "\'";
        else if (s == "")
        {
            if (fields[i]._IsNumeric())
                ss << "null";
            else
                ss << "\'\'";
        }
        else
           ss << "\'" << s << "\'";

        if (i < result->GetFieldCount()-1)
            ss << ", ";
    }
    ss << ")";

    return ss.str();
}