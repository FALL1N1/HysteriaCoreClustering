#include "Player.h"
#include "ClientSessionMgr.h"
#include "ObjectMgr.h"

bool normalizePlayerName(std::string& name)
{
    if (name.empty())
        return false;

    wchar_t wstr_buf[MAX_INTERNAL_PLAYER_NAME+1];
    size_t wstr_len = MAX_INTERNAL_PLAYER_NAME;

    if (!Utf8toWStr(name, &wstr_buf[0], wstr_len))
        return false;

    wstr_buf[0] = wcharToUpper(wstr_buf[0]);
    for (size_t i = 1; i < wstr_len; ++i)
        wstr_buf[i] = wcharToLower(wstr_buf[i]);

    if (!WStrToUtf8(wstr_buf, wstr_len, name))
        return false;

    return true;
}

LanguageDesc lang_description[LANGUAGES_COUNT] =
{
    { LANG_ADDON,           0, 0                       },
    { LANG_UNIVERSAL,       0, 0                       },
    { LANG_ORCISH,        669, SKILL_LANG_ORCISH       },
    { LANG_DARNASSIAN,    671, SKILL_LANG_DARNASSIAN   },
    { LANG_TAURAHE,       670, SKILL_LANG_TAURAHE      },
    { LANG_DWARVISH,      672, SKILL_LANG_DWARVEN      },
    { LANG_COMMON,        668, SKILL_LANG_COMMON       },
    { LANG_DEMONIC,       815, SKILL_LANG_DEMON_TONGUE },
    { LANG_TITAN,         816, SKILL_LANG_TITAN        },
    { LANG_THALASSIAN,    813, SKILL_LANG_THALASSIAN   },
    { LANG_DRACONIC,      814, SKILL_LANG_DRACONIC     },
    { LANG_KALIMAG,       817, SKILL_LANG_OLD_TONGUE   },
    { LANG_GNOMISH,      7340, SKILL_LANG_GNOMISH      },
    { LANG_TROLL,        7341, SKILL_LANG_TROLL        },
    { LANG_GUTTERSPEAK, 17737, SKILL_LANG_GUTTERSPEAK  },
    { LANG_DRAENEI,     29932, SKILL_LANG_DRAENEI      },
    { LANG_ZOMBIE,          0, 0                       },
    { LANG_GNOMISH_BINARY,  0, 0                       },
    { LANG_GOBLIN_BINARY,   0, 0                       }
};

LanguageDesc const* GetLanguageDescByID(uint32 lang)
{
    for (uint8 i = 0; i < LANGUAGES_COUNT; ++i)
    {
        if (uint32(lang_description[i].lang_id) == lang)
            return &lang_description[i];
    }

    return NULL;
}

//Player Handlingz
void ObjectMgr::Player_Add(Player* player)
{
    _NewPlayer.add(player);
}

void ObjectMgr::Update()
{
    TRINITY_GUARD(ACE_Thread_Mutex, i_objectLock);
    {
        Player* nPlayer;

        //Add the new Players
        while (!_NewPlayer.empty() && _NewPlayer.next(nPlayer))
        {
            if (nPlayer)
            {
                //Recheck if this fucking player already in list
                PlayerMap::iterator itr = m_PlayerMap.find(nPlayer->GetGUID());
                if (itr != m_PlayerMap.end())
                {
                    itr->second->CleanupChannels();
                    sClientSessionMgr->DecreasePlayerCount(nPlayer->GetTeam());
                    m_PlayerMap.erase(itr);
                }

                m_PlayerMap[nPlayer->GetGUID()] = nPlayer;
                sClientSessionMgr->IncreasePlayerCount(nPlayer->GetTeam());
            }
        }

        //Remove the old one
        while (!_DelPlayer.empty() && _DelPlayer.next(nPlayer))
        {
           if (nPlayer)
           {
                PlayerMap::iterator itr = m_PlayerMap.find(nPlayer->GetGUID());
                if (itr != m_PlayerMap.end())
                {
                    sClientSessionMgr->DecreasePlayerCount(nPlayer->GetTeam());
                    delete nPlayer;
                    m_PlayerMap.erase(itr);
                }
           }
        }
    }
}

void ObjectMgr::Player_Remove(Player* player)
{
    if (player)
    {
        sLog->outDebug(LOG_FILTER_PLAYER_LOADING, "Player %s is loggin out.", player->GetPlayerName().c_str());
        _DelPlayer.add(player);
    }
}

Player* ObjectMgr::GetPlayer(uint64 GUID)
{ 
    PlayerMap::const_iterator itr = m_PlayerMap.find(GUID);
    if (itr != m_PlayerMap.end())
        return itr->second;                               
    else
        return NULL;
}

Player* ObjectMgr::FindPlayer(uint64 GUID)
{
    PlayerMap::const_iterator itr = m_PlayerMap.find(GUID);
    if (itr != m_PlayerMap.end())
        return itr->second;
    else
        return NULL;
}

// @emo
Player* ObjectMgr::FindPlayerInOrOutOfWorld(uint64 GUID)
{
    PlayerMap::const_iterator itr = m_PlayerMap.find(GUID);
    if (itr != m_PlayerMap.end())
        return itr->second;
    else
        return NULL;
}

Player* ObjectMgr::FindPlayerByName(const char* name)
{
    std::string nameStr = name;
    std::transform(nameStr.begin(), nameStr.end(), nameStr.begin(), ::tolower);

    for (PlayerMap::iterator itr = m_PlayerMap.begin(); itr != m_PlayerMap.end(); itr++)
    {
        if (itr == m_PlayerMap.end())
            return NULL;

        std::string currentName = itr->second->GetPlayerName();
        std::transform(currentName.begin(), currentName.end(), currentName.begin(), ::tolower);
        if (nameStr.compare(currentName) == 0)
            return itr->second;
    }
    return NULL;         
}

Player* ObjectMgr::FindPlayerByName(std::string name)
{
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    for (PlayerMap::iterator itr = m_PlayerMap.begin(); itr != m_PlayerMap.end(); itr++)
    {
        if (itr == m_PlayerMap.end())
            return NULL;

        std::string currentName = itr->second->GetPlayerName();
        std::transform(currentName.begin(), currentName.end(), currentName.begin(), ::tolower);
        if (name.compare(currentName) == 0)
            return itr->second;
    }
    return NULL;         
}

bool ObjectMgr::GetPlayerNameByGUID(uint64 guid, std::string &name) const
{
    // prevent DB access for online player
    if (Player* player = sObjectMgr->GetPlayer(guid))
    {
        name = player->GetPlayerName();
        return true;
    }

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_NAME_BY_GUID);

    stmt->setUInt32(0, GUID_LOPART(guid));

    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (result)
    {
        name = (*result)[0].GetString();
        return true;
    }

    return false;
}

// name must be checked to correctness (if received) before call this function
uint64 ObjectMgr::GetPlayerGUIDByName(std::string name) const
{
    uint64 guid = 0;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GUID_BY_NAME_FILTER);

    stmt->setString(0, name);

    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (result)
        guid = MAKE_NEW_GUID((*result)[0].GetUInt32(), 0, HIGHGUID_PLAYER);

    return guid;
}

uint32 ObjectMgr::GetPlayerAccountIdByGUID(uint64 guid) const
{
    // prevent DB access for online player
    if (Player* player = sObjectMgr->GetPlayer(guid))
    {
        return player->GetSession()->GetAccountId();
    }

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ACCOUNT_BY_GUID);

    stmt->setUInt32(0, GUID_LOPART(guid));

    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (result)
    {
        uint32 acc = (*result)[0].GetUInt32();
        return acc;
    }

    return 0;
}

uint32 ObjectMgr::GetPlayerAccountIdByPlayerName(const std::string& name) const
{
    QueryResult result = CharacterDatabase.PQuery("SELECT account FROM characters WHERE name = '%s'", name.c_str());
    if (result)
    {
        uint32 acc = (*result)[0].GetUInt32();
        return acc;
    }

    return 0;
}

//Locales
void ObjectMgr::AddLocaleString(std::string const& s, LocaleConstant locale, StringVector& data)
{
    if (!s.empty())
    {
        if (data.size() <= size_t(locale))
            data.resize(locale + 1);

        data[locale] = s;
    }
}

bool ObjectMgr::LoadTrinityStrings(const char* table, int32 min_value, int32 max_value)
{
    uint32 oldMSTime = getMSTime();

    int32 start_value = min_value;
    int32 end_value   = max_value;
    // some string can have negative indexes range
    if (start_value < 0)
    {
        if (end_value >= start_value)
        {
            sLog->outErrorDb("Table '%s' attempt loaded with invalid range (%d - %d), strings not loaded.", table, min_value, max_value);
            return false;
        }

        // real range (max+1, min+1) exaple: (-10, -1000) -> -999...-10+1
        std::swap(start_value, end_value);
        ++start_value;
        ++end_value;
    }
    else
    {
        if (start_value >= end_value)
        {
            sLog->outErrorDb("Table '%s' attempt loaded with invalid range (%d - %d), strings not loaded.", table, min_value, max_value);
            return false;
        }
    }

    // cleanup affected map part for reloading case
    for (TrinityStringLocaleContainer::iterator itr = _trinityStringLocaleStore.begin(); itr != _trinityStringLocaleStore.end();)
    {
        if (itr->first >= start_value && itr->first < end_value)
            _trinityStringLocaleStore.erase(itr++);
        else
            ++itr;
    }

    QueryResult result = WorldDatabase.PQuery("SELECT entry, content_default, content_loc1, content_loc2, content_loc3, content_loc4, content_loc5, content_loc6, content_loc7, content_loc8 FROM %s", table);

    if (!result)
    {
        if (min_value == MIN_TRINITY_STRING_ID)              // error only in case internal strings
            sLog->outErrorDb(">> Loaded 0 trinity strings. DB table `%s` is empty. Cannot continue.", table);
        else
            sLog->outString(">> Loaded 0 string templates. DB table `%s` is empty.", table);
        sLog->outString();
        return false;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        int32 entry = fields[0].GetInt32();

        if (entry == 0)
        {
            sLog->outErrorDb("Table `%s` contain reserved entry 0, ignored.", table);
            continue;
        }
        else if (entry < start_value || entry >= end_value)
        {
            sLog->outErrorDb("Table `%s` contain entry %i out of allowed range (%d - %d), ignored.", table, entry, min_value, max_value);
            continue;
        }

        TrinityStringLocale& data = _trinityStringLocaleStore[entry];

        if (!data.Content.empty())
        {
            sLog->outErrorDb("Table `%s` contain data for already loaded entry  %i (from another table?), ignored.", table, entry);
            continue;
        }

        data.Content.resize(1);
        ++count;

        for (uint8 i = 0; i < TOTAL_LOCALES; ++i)
            AddLocaleString(fields[i + 1].GetString(), LocaleConstant(i), data.Content);
    } while (result->NextRow());

    if (min_value == MIN_TRINITY_STRING_ID)
        sLog->outString(">> Loaded %u Trinity strings from table %s in %u ms", count, table, GetMSTimeDiffToNow(oldMSTime));
    else
        sLog->outString(">> Loaded %u string templates from %s in %u ms", count, table, GetMSTimeDiffToNow(oldMSTime));

    sLog->outString();
    return true;
}

const char *ObjectMgr::GetTrinityString(int32 entry, LocaleConstant locale_idx) const
{
    if (TrinityStringLocale const* msl = GetTrinityStringLocale(entry))
    {
        if (msl->Content.size() > size_t(locale_idx) && !msl->Content[locale_idx].empty())
            return msl->Content[locale_idx].c_str();

        return msl->Content[DEFAULT_LOCALE].c_str();
    }

    if (entry > 0)
        sLog->outErrorDb("Entry %i not found in `trinity_string` table.", entry);
    else
        sLog->outErrorDb("Trinity string entry %i not found in DB.", entry);
    return "<error>";
}

void ObjectMgr::SetHighestGuids()
{
    QueryResult result = CharacterDatabase.Query("SELECT MAX(guid) FROM characters");
    if (result)
        m_hiCharGuid = (*result)[0].GetUInt32()+1;

    result = WorldDatabase.Query("SELECT MAX(guid) FROM creature");
    if (result)
        m_hiCreatureGuid = (*result)[0].GetUInt32()+1;

    result = CharacterDatabase.Query("SELECT MAX(guid) FROM item_instance");
    if (result)
        m_hiItemGuid = (*result)[0].GetUInt32()+1;

    // Cleanup other tables from not existed guids ( >= m_hiItemGuid)
    CharacterDatabase.PExecute("DELETE FROM character_inventory WHERE item >= '%u'", m_hiItemGuid);
    CharacterDatabase.PExecute("DELETE FROM mail_items WHERE item_guid >= '%u'", m_hiItemGuid);
    CharacterDatabase.PExecute("DELETE FROM auctionhouse WHERE itemguid >= '%u'", m_hiItemGuid);
    CharacterDatabase.PExecute("DELETE FROM guild_bank_item WHERE item_guid >= '%u'", m_hiItemGuid);

    result = WorldDatabase.Query("SELECT MAX(guid) FROM gameobject");
    if (result)
        m_hiGoGuid = (*result)[0].GetUInt32()+1;

    result = WorldDatabase.Query("SELECT MAX(guid) FROM transports");
    if (result)
        m_hiMoTransGuid = (*result)[0].GetUInt32()+1;

    result = CharacterDatabase.Query("SELECT MAX(id) FROM auctionhouse");
    if (result)
        m_auctionid = (*result)[0].GetUInt32()+1;

    result = CharacterDatabase.Query("SELECT MAX(id) FROM mail");
    if (result)
        m_mailid = (*result)[0].GetUInt32()+1;

    result = CharacterDatabase.Query("SELECT MAX(corpseGuid) FROM corpse");
    if (result)
        m_hiCorpseGuid = (*result)[0].GetUInt32()+1;

    //result = CharacterDatabase.Query("SELECT MAX(arenateamid) FROM arena_team");
    //if (result)
    //    sArenaTeamMgr->SetNextArenaTeamId((*result)[0].GetUInt32()+1);

    result = CharacterDatabase.Query("SELECT MAX(setguid) FROM character_equipmentsets");
    if (result)
        m_equipmentSetGuid = (*result)[0].GetUInt64()+1;

    //result = CharacterDatabase.Query("SELECT MAX(guildId) FROM guild");
    //if (result)
    //    sGuildMgr->SetNextGuildId((*result)[0].GetUInt32()+1);

    result = CharacterDatabase.Query("SELECT MAX(guid) FROM groups");
    if (result)
        m_hiGroupGuid = ((*result)[0].GetUInt32()+1);
    else
        m_hiGroupGuid = 1;
}

//Groups
uint32 ObjectMgr::IncreaseGroupId()
{
    if (m_hiGroupGuid >= 0xFFFFFFFE)
    {
        sLog->outError("Group guid overflow!! Can't continue, shutting down server. ");
        Logon::StopNow(ERROR_EXIT_CODE);
    }
    return m_hiGroupGuid++;
}

void ObjectMgr::LoadScriptNames()
{
    uint32 oldMSTime = getMSTime();

    _scriptNamesStore.push_back("");
    QueryResult result = WorldDatabase.Query(
      "SELECT DISTINCT(ScriptName) FROM achievement_criteria_data WHERE ScriptName <> '' AND type = 11 "
      "UNION "
      "SELECT DISTINCT(ScriptName) FROM battleground_template WHERE ScriptName <> '' "
      "UNION "
      "SELECT DISTINCT(ScriptName) FROM creature_template WHERE ScriptName <> '' "
      "UNION "
      "SELECT DISTINCT(ScriptName) FROM gameobject_template WHERE ScriptName <> '' "
      "UNION "
      "SELECT DISTINCT(ScriptName) FROM item_template WHERE ScriptName <> '' "
      "UNION "
      "SELECT DISTINCT(ScriptName) FROM areatrigger_scripts WHERE ScriptName <> '' "
      "UNION "
      "SELECT DISTINCT(ScriptName) FROM spell_script_names WHERE ScriptName <> '' "
      "UNION "
      "SELECT DISTINCT(ScriptName) FROM transports WHERE ScriptName <> '' "
      "UNION "
      "SELECT DISTINCT(ScriptName) FROM game_weather WHERE ScriptName <> '' "
      "UNION "
      "SELECT DISTINCT(ScriptName) FROM conditions WHERE ScriptName <> '' "
      "UNION "
      "SELECT DISTINCT(ScriptName) FROM outdoorpvp_template WHERE ScriptName <> '' "
      "UNION "
      "SELECT DISTINCT(script) FROM instance_template WHERE script <> ''");

    if (!result)
    {
        sLog->outString();
        sLog->outErrorDb(">> Loaded empty set of Script Names!");
        return;
    }

    uint32 count = 1;

    do
    {
        _scriptNamesStore.push_back((*result)[0].GetString());
        ++count;
    }
    while (result->NextRow());

    std::sort(_scriptNamesStore.begin(), _scriptNamesStore.end());
    sLog->outString(">> Loaded %d Script Names in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    sLog->outString();
}

uint32 ObjectMgr::GetScriptId(const char *name)
{
    // use binary search to find the script name in the sorted vector
    // assume "" is the first element
    if (!name)
        return 0;

    ScriptNameContainer::const_iterator itr = std::lower_bound(_scriptNamesStore.begin(), _scriptNamesStore.end(), name);
    if (itr == _scriptNamesStore.end() || *itr != name)
        return 0;

    return uint32(itr - _scriptNamesStore.begin());
}

void ObjectMgr::CheckScripts(ScriptsType type, std::set<int32>& ids)
{
    return;
}