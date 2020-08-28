#ifndef _WORLDDATABASEROUTINES_H_
#define _WORLDDATABASEROUTINES_H_

#include <string>
#include <map>
#include <limits>
#include <functional>





typedef std::unordered_map <uint32, uint32> CreatureMap;

class WorldDatabaseRebase
{
    public:

        CreatureMap _CreatureGUIDs;

        void Begin();
        void PreProcess();

        void RebaseCreatures();
        void RebaseCreatureAddons();
        void RebaseCreatureFormations();
        void RebaseLinkedRespawn();

    private:
};
#define sWorldRebase ACE_Singleton<WorldDatabaseRebase, ACE_Null_Mutex>::instance()

#endif
