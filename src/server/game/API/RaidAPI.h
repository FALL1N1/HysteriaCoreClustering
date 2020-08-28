#include "Map.h"
#include "Player.h"
#include "Log.h"

class RaidAPI
{
    friend class ACE_Singleton<RaidAPI, ACE_Null_Mutex>;
    RaidAPI();

    public:

        void StartLogging(InstanceMap* map, Creature* boss);
        void ResetLogging(InstanceMap* map, Creature* boss);
        void FinishLogging(InstanceMap* map, Creature* boss);
         

    private:
        uint32 startLoggingTime; // getMSTime();
        uint32 finishLoggingTime; // getMSTime();
};

#define sRaidAPI ACE_Singleton<RaidAPI, ACE_Null_Mutex>::instance()