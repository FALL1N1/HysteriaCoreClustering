class DungeonAPI
{
    friend class ACE_Singleton<DungeonAPI, ACE_Null_Mutex>;
    DungeonAPI();

    public:

        void StartLogging();
        void StopLogging();
};

#define sDungeonAPI ACE_Singleton<DungeonAPI, ACE_Null_Mutex>::instance()