#ifndef TRINITY_CREATURES_H
#define TRINITY_CREATURES_H

#include "SharedDefines.h"

#define MAX_KILL_CREDIT 2
#define MAX_CREATURE_QUEST_ITEMS 6

struct CreatureTemplate
{
    uint32      entry;
    std::string Name;
    std::string SubName;
    std::string IconName;
    uint32      type_flags;
    uint32      type;
    uint32      family;
    uint32      rank;
    uint32      KillCredit[MAX_KILL_CREDIT];
    uint32      Modelid1;
    uint32      Modelid2;
    uint32      Modelid3;
    uint32      Modelid4;
    float       ModHealth;
    float       ModMana;
    bool        RacialLeader;
    uint32      questItems[MAX_CREATURE_QUEST_ITEMS];
    uint32      movementId;
};
// Benchmarked: Faster than std::map (insert/find)
typedef std::unordered_map<uint32, CreatureTemplate> CreatureTemplateContainer;

class Creatures
{
    public:
        void WarmingCache();
        CreatureTemplate const* GetCreatureTemplate(uint32 entry);
        void CacheCreatureTemplate(CreatureTemplate* creature);

    private:
        ACE_RW_Thread_Mutex rwMutex_;
        CreatureTemplateContainer _creatureTemplateStore;

};
#define sCreatures ACE_Singleton<Creatures, ACE_Null_Mutex>::instance()

#endif