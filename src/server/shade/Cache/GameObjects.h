#ifndef TRINITY_GAMEOBJECTS_H
#define TRINITY_GAMEOBJECTS_H

#include "SharedDefines.h"

#define MAX_GAMEOBJECT_QUEST_ITEMS  6
#define MAX_GAMEOBJECT_DATA         24    

struct GameObjectTemplate
{
    uint32      entry;
    uint32      type;
    uint32      displayId;
    std::string Name;
    std::string IconName;
    std::string CastBarCaption;
    std::string unk1;
    float       size;
    uint32  questItems[MAX_GAMEOBJECT_QUEST_ITEMS];

    struct
    {
        uint32 data[MAX_GAMEOBJECT_DATA];
    } raw;
    

};
// Benchmarked: Faster than std::map (insert/find)
typedef std::unordered_map<uint32, GameObjectTemplate> GameObjectTemplateContainer;

class GameObjects
{
    public:
        void WarmingCache();
        GameObjectTemplate const* GetGameObjectTemplate(uint32 entry);
        void CacheGameObjectTemplate(GameObjectTemplate* gob);

    private:
        ACE_RW_Thread_Mutex rwMutex_;
        GameObjectTemplateContainer _gameObjectTemplateStore;

};
#define sGameObjects ACE_Singleton<GameObjects, ACE_Null_Mutex>::instance()
#endif