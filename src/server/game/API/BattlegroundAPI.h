class BattlegroundAPI
{
    friend class ACE_Singleton<BattlegroundAPI, ACE_Null_Mutex>;
    BattlegroundAPI();

    public:

        void StartLogging();
        void StopLogging();
};

#define sBattlegroundAPI ACE_Singleton<BattlegroundAPI, ACE_Null_Mutex>::instance()