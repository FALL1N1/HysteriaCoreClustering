#ifndef TRINITY_QUEST_H
#define TRINITY_QUEST_H

#include "SharedDefines.h"
#include "QuestDef.h"

typedef std::unordered_map<uint32, QuestTemplate*> QuestTemplateContainer;

class Quest
{
    public:
        void WarmingCache();
        QuestTemplate const* GetQuestTemplate(uint32 entry);
        void CacheQuestTemplate(QuestTemplate* creature);

    private:
        ACE_RW_Thread_Mutex rwMutex_;
        QuestTemplateContainer _questTemplateStore;

};
#define sQuest ACE_Singleton<Quest, ACE_Null_Mutex>::instance()

#endif
