#include "RaidAPI.h"

RaidAPI::RaidAPI() { }

void RaidAPI::StartLogging(InstanceMap* map, Creature* boss)
{ 
    if (!sWorld->getBoolConfig(CONFIG_API_DB_ENABLED))
        return;

    if (!map)
        return;

    startLoggingTime = getMSTime();
    Map::PlayerList const& pl = map->GetPlayers();
    for (Map::PlayerList::const_iterator itr = pl.begin(); itr != pl.end(); ++itr)
        if (Player* player = itr->GetSource())
        {
            player->StartLogPlayer = true;
        }
}

void RaidAPI::ResetLogging(InstanceMap* map, Creature* boss)
{
    if (!sWorld->getBoolConfig(CONFIG_API_DB_ENABLED))
        return;

    startLoggingTime = 0;
    finishLoggingTime = 0;

    Map::PlayerList const& pl = map->GetPlayers();
    for (Map::PlayerList::const_iterator itr = pl.begin(); itr != pl.end(); ++itr)
        if (Player* player = itr->GetSource())
        {
            player->ResetAPIVars();
        }
}
 
void RaidAPI::FinishLogging(InstanceMap* map, Creature* boss)
{  
    if (!sWorld->getBoolConfig(CONFIG_API_DB_ENABLED))
        return;

    /* generate unique raid id */
    QueryResult result = APIDatabase.Query("select max(fightEntry) from pve_fight");
    if (!result)
        return;
    Field* fields = result->Fetch();
    uint32 fightEntry = fields[0].GetUInt32() + 1;

    /* ------------------------*/

    finishLoggingTime = getMSTime();
    SQLTransaction rapitrans = APIDatabase.BeginTransaction();
    PreparedStatement* rapistmt = APIDatabase.GetPreparedStatement(API_INS_PVE_KILL);

    Map::PlayerList const& pl = map->GetPlayers();
    for (Map::PlayerList::const_iterator itr = pl.begin(); itr != pl.end(); ++itr)
        if (Player* player = itr->GetSource())
        {
            player->StartLogPlayer = false;

            uint8 index = 0;
            rapistmt->setUInt32(index, startLoggingTime);
            rapistmt->setUInt32(++index, player->GetGUID());
            rapistmt->setString(++index, player->GetName());
            rapistmt->setUInt8(++index, player->IsHealerTalentSpec() ? 1 : 0);
            rapistmt->setUInt32(++index, player->GetAPIHealing());
            rapistmt->setUInt32(++index, player->GetAPIDamage());
            rapistmt->setUInt32(++index, boss->GetEntry());
            rapistmt->setString(++index, boss->GetName());
            rapistmt->setUInt32(++index, boss->GetMaxHealth());
            rapistmt->setUInt32(++index, finishLoggingTime);
            rapistmt->setUInt32(++index, fightEntry);

            rapitrans->Append(rapistmt);

            player->ResetAPIVars();
        }
    APIDatabase.CommitTransaction(rapitrans);

    startLoggingTime = 0;
    finishLoggingTime = 0;
    sLog->outAPI("Players killed %s(%u) maxhp: %u, logged successfully!", boss->GetName(), boss->GetEntry(), boss->GetMaxHealth());
} 