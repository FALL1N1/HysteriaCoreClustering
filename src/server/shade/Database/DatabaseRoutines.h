#ifndef _DATABASEROUTINES_H_
#define _DATABASEROUTINES_H_

#include <string>
#include <map>
#include <limits>
#include <functional>

//RebaseState
#define LOGON_REBASE_STATE 1
#define SEC_IN_DAY 86400
#define WRAPS 1000

typedef std::unordered_map <uint32, uint32> ItemInstanceMap;

class RebaseClass
{
    friend class item_instance;
    friend class character_inventory_thread;

    public:
        void Begin();

		//Timer deps
		void InitAutomaticRebase();
		void UpdateTimers(uint32 diff);
        time_t GetTimer() { return m_NextRebaseTime; }

        std::string SingleGUID(uint64 offset, uint64 value, Field* fields, QueryResult result);
        std::string DoubleGUID(uint64 offset, uint64 value, uint64 offsetb, uint64 valueb, Field* fields, QueryResult result);

    private:
        ItemInstanceMap _ItemMap;

        void Init();
        void Cleanup();

        void ItemInstance();
        void Auctionhouse();
        void CharacterGifts();
        void CharacterInventory();
        void GuildBankItem();
        void ItemRefundInstance();
        void ItemSoulboundTradeData();
        void MailItems();
        void Petition();
        void PetitionSign();


		time_t m_NextRebaseTime;
        time_t m_NextRebaseTimePlus;
        uint32 count_down;

};
#define sRebase ACE_Singleton<RebaseClass, ACE_Null_Mutex>::instance()


#endif
