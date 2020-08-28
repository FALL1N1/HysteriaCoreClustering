#ifndef TRINITY_LOCALES_MGR_H
#define TRINITY_LOCALES_MGR_H

#include "QuestDef.h"

struct CreatureLocale
{
    StringVector Name;
    StringVector SubName;
};

struct GameObjectLocale
{
    StringVector Name;
    StringVector CastBarCaption;
};

struct ItemLocale
{
    StringVector Name;
    StringVector Description;
};

typedef std::unordered_map<uint32, CreatureLocale>   CreatureLocaleContainer;
typedef std::unordered_map<uint32, GameObjectLocale> GameObjectLocaleContainer;
typedef std::unordered_map<uint32, ItemLocale> ItemLocaleContainer;
typedef std::unordered_map<uint32, QuestLocale> QuestLocaleContainer;

class LocalesMgr
{
    public:
        //Load Loc
        void LoadLocales();
        void LoadCreatureLocales();
        void LoadGameObjectLocales();
        void LoadItemLocales();
        void LoadQuestLocales();

        CreatureLocale const* GetCreatureLocale(uint32 entry) const
        {
            CreatureLocaleContainer::const_iterator itr = _creatureLocaleStore.find(entry);
            if (itr == _creatureLocaleStore.end()) return NULL;
            return &itr->second;
        }

        GameObjectLocale const* GetGameObjectLocale(uint32 entry) const
        {
            GameObjectLocaleContainer::const_iterator itr = _gameObjectLocaleStore.find(entry);
            if (itr == _gameObjectLocaleStore.end()) return NULL;
            return &itr->second;
        }

        ItemLocale const* GetItemLocale(uint32 entry) const
        {
            ItemLocaleContainer::const_iterator itr = _itemLocaleStore.find(entry);
            if (itr == _itemLocaleStore.end()) return NULL;
            return &itr->second;
        }
        QuestLocale const* GetQuestLocale(uint32 entry) const
        {
            QuestLocaleContainer::const_iterator itr = _questLocaleStore.find(entry);
            if (itr == _questLocaleStore.end()) return NULL;
            return &itr->second;
        }
        static inline void GetLocaleString(const StringVector& data, int loc_idx, std::string& value)
        {
            if (data.size() > size_t(loc_idx) && !data[loc_idx].empty())
                value = data[loc_idx];
        }

        static void AddLocaleString(const std::string& s, LocaleConstant locale, StringVector& data);
    private:
        CreatureLocaleContainer     _creatureLocaleStore;
        GameObjectLocaleContainer   _gameObjectLocaleStore;
        ItemLocaleContainer         _itemLocaleStore;     
        QuestLocaleContainer        _questLocaleStore;
};
#define sLocalesMgr ACE_Singleton<LocalesMgr, ACE_Null_Mutex>::instance()
#endif