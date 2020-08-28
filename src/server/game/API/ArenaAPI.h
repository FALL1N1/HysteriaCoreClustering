class ArenaAPI
{
    friend class ACE_Singleton<ArenaAPI, ACE_Null_Mutex>;
    ArenaAPI();
};

#define sArenaAPI ACE_Singleton<ArenaAPI, ACE_Null_Mutex>::instance()