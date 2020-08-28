#include "LocalesMgr.h"

void LocalesMgr::LoadLocales()
{
    LoadCreatureLocales();
    LoadGameObjectLocales();
    LoadItemLocales();
    LoadQuestLocales();
    sLog->outString();
}

void LocalesMgr::LoadCreatureLocales()
{
    uint32 oldMSTime = getMSTime();

    _creatureLocaleStore.clear();                              // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT entry, name_loc1, subname_loc1, name_loc2, subname_loc2, name_loc3, subname_loc3, name_loc4, subname_loc4, name_loc5, subname_loc5, name_loc6, subname_loc6, name_loc7, subname_loc7, name_loc8, subname_loc8 FROM locales_creature");

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        CreatureLocale& data = _creatureLocaleStore[entry];

        for (uint8 i = 1; i < TOTAL_LOCALES; ++i)
        {
            LocaleConstant locale = (LocaleConstant)i;
            AddLocaleString(fields[1 + 2 * (i - 1)].GetString(), locale, data.Name);
            AddLocaleString(fields[1 + 2 * (i - 1) + 1].GetString(), locale, data.SubName);
        }
    } while (result->NextRow());

    sLog->outString(">> Loaded %lu creature locale strings in %u ms", (unsigned long)_creatureLocaleStore.size(), GetMSTimeDiffToNow(oldMSTime));
    sLog->outString();
}

void LocalesMgr::LoadGameObjectLocales()
{
    uint32 oldMSTime = getMSTime();

    _gameObjectLocaleStore.clear(); // need for reload case
    QueryResult result = WorldDatabase.Query("SELECT entry, "
        "name_loc1, name_loc2, name_loc3, name_loc4, name_loc5, name_loc6, name_loc7, name_loc8, "
        "castbarcaption_loc1, castbarcaption_loc2, castbarcaption_loc3, castbarcaption_loc4, "
        "castbarcaption_loc5, castbarcaption_loc6, castbarcaption_loc7, castbarcaption_loc8 FROM locales_gameobject");

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        GameObjectLocale& data = _gameObjectLocaleStore[entry];

        for (uint8 i = 1; i < TOTAL_LOCALES; ++i)
            AddLocaleString(fields[i].GetString(), LocaleConstant(i), data.Name);

        for (uint8 i = 1; i < TOTAL_LOCALES; ++i)
            AddLocaleString(fields[i + (TOTAL_LOCALES - 1)].GetString(), LocaleConstant(i), data.CastBarCaption);
    } while (result->NextRow());

    sLog->outString(">> Loaded %lu gameobject locale strings in %u ms", (unsigned long)_gameObjectLocaleStore.size(), GetMSTimeDiffToNow(oldMSTime));
    sLog->outString();
}

void LocalesMgr::LoadItemLocales()
{
    uint32 oldMSTime = getMSTime();

    _itemLocaleStore.clear();                                 // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT entry, name_loc1, description_loc1, name_loc2, description_loc2, name_loc3, description_loc3, name_loc4, description_loc4, name_loc5, description_loc5, name_loc6, description_loc6, name_loc7, description_loc7, name_loc8, description_loc8 FROM locales_item");

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        ItemLocale& data = _itemLocaleStore[entry];

        for (uint8 i = 1; i < TOTAL_LOCALES; ++i)
        {
            LocaleConstant locale = (LocaleConstant)i;
            AddLocaleString(fields[1 + 2 * (i - 1)].GetString(), locale, data.Name);
            AddLocaleString(fields[1 + 2 * (i - 1) + 1].GetString(), locale, data.Description);
        }
    } while (result->NextRow());

    sLog->outString(">> Loaded %lu Item locale strings in %u ms", (unsigned long)_itemLocaleStore.size(), GetMSTimeDiffToNow(oldMSTime));
    sLog->outString();
}

void LocalesMgr::LoadQuestLocales()
{
    uint32 oldMSTime = getMSTime();

    _questLocaleStore.clear();                                // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT Id, "
        "Title_loc1, Details_loc1, Objectives_loc1, OfferRewardText_loc1, RequestItemsText_loc1, EndText_loc1, CompletedText_loc1, ObjectiveText1_loc1, ObjectiveText2_loc1, ObjectiveText3_loc1, ObjectiveText4_loc1, "
        "Title_loc2, Details_loc2, Objectives_loc2, OfferRewardText_loc2, RequestItemsText_loc2, EndText_loc2, CompletedText_loc2, ObjectiveText1_loc2, ObjectiveText2_loc2, ObjectiveText3_loc2, ObjectiveText4_loc2, "
        "Title_loc3, Details_loc3, Objectives_loc3, OfferRewardText_loc3, RequestItemsText_loc3, EndText_loc3, CompletedText_loc3, ObjectiveText1_loc3, ObjectiveText2_loc3, ObjectiveText3_loc3, ObjectiveText4_loc3, "
        "Title_loc4, Details_loc4, Objectives_loc4, OfferRewardText_loc4, RequestItemsText_loc4, EndText_loc4, CompletedText_loc4, ObjectiveText1_loc4, ObjectiveText2_loc4, ObjectiveText3_loc4, ObjectiveText4_loc4, "
        "Title_loc5, Details_loc5, Objectives_loc5, OfferRewardText_loc5, RequestItemsText_loc5, EndText_loc5, CompletedText_loc5, ObjectiveText1_loc5, ObjectiveText2_loc5, ObjectiveText3_loc5, ObjectiveText4_loc5, "
        "Title_loc6, Details_loc6, Objectives_loc6, OfferRewardText_loc6, RequestItemsText_loc6, EndText_loc6, CompletedText_loc6, ObjectiveText1_loc6, ObjectiveText2_loc6, ObjectiveText3_loc6, ObjectiveText4_loc6, "
        "Title_loc7, Details_loc7, Objectives_loc7, OfferRewardText_loc7, RequestItemsText_loc7, EndText_loc7, CompletedText_loc7, ObjectiveText1_loc7, ObjectiveText2_loc7, ObjectiveText3_loc7, ObjectiveText4_loc7, "
        "Title_loc8, Details_loc8, Objectives_loc8, OfferRewardText_loc8, RequestItemsText_loc8, EndText_loc8, CompletedText_loc8, ObjectiveText1_loc8, ObjectiveText2_loc8, ObjectiveText3_loc8, ObjectiveText4_loc8"
        " FROM locales_quest");

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        QuestLocale& data = _questLocaleStore[entry];

        for (uint8 i = 1; i < TOTAL_LOCALES; ++i)
        {
            LocaleConstant locale = (LocaleConstant)i;

            AddLocaleString(fields[1 + 11 * (i - 1)].GetString(), locale, data.Title);
            AddLocaleString(fields[1 + 11 * (i - 1) + 1].GetString(), locale, data.Details);
            AddLocaleString(fields[1 + 11 * (i - 1) + 2].GetString(), locale, data.Objectives);
            AddLocaleString(fields[1 + 11 * (i - 1) + 3].GetString(), locale, data.OfferRewardText);
            AddLocaleString(fields[1 + 11 * (i - 1) + 4].GetString(), locale, data.RequestItemsText);
            AddLocaleString(fields[1 + 11 * (i - 1) + 5].GetString(), locale, data.EndText);
            AddLocaleString(fields[1 + 11 * (i - 1) + 6].GetString(), locale, data.CompletedText);

            for (uint8 k = 0; k < 4; ++k)
                AddLocaleString(fields[1 + 11 * (i - 1) + 7 + k].GetString(), locale, data.ObjectiveText[k]);
        }
    } while (result->NextRow());

    sLog->outString(">> Loaded %lu Quest locale strings in %u ms", (unsigned long)_questLocaleStore.size(), GetMSTimeDiffToNow(oldMSTime));
    sLog->outString();
}

void LocalesMgr::AddLocaleString(std::string const& s, LocaleConstant locale, StringVector& data)
{
    if (!s.empty())
    {
        if (data.size() <= size_t(locale))
            data.resize(locale + 1);

        data[locale] = s;
    }
}