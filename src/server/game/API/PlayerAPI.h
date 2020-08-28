class PlayerAPI
{
    friend class ACE_Singleton<PlayerAPI, ACE_Null_Mutex>;
    PlayerAPI();

    public:

        void UpdatePlayer(Player* player, uint16 achPts);
        void UpdateLastLootedItems(Player* player);
        void DeletePlayer(uint64 guid);
        void UpdateInventory(Player* player); 

};

#define sPlayerAPI ACE_Singleton<PlayerAPI, ACE_Null_Mutex>::instance()