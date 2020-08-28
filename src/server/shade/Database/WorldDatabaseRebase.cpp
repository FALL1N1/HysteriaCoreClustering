#include "Common.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "ClientSessionMgr.h"
#include "Proxy.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "ControlSessionMgr.h"
#include "ClusterDefines.h"
#include "ObjectMgr.h"
#include "RoutingHelper.h"
#include "ProgressBar.h"
#include "WorldDatabaseRebase.h"
#include "DatabaseRoutines.h"


void WorldDatabaseRebase::Begin()
{

    PreProcess();

    sLog->outString("Collecting Creature-GUIDs");
    QueryResult result = WorldDatabase.Query("SELECT guid FROM creature");
    if (!result)
    {
        sLog->outString(">> Loaded 0 creature GUIDs.");
        sLog->outString();
        return;
    }
    barGoLink bar(result->GetRowCount());
    uint32 count = 0;
    do
    {
        bar.step();
        ++count;
        Field* fields = result->Fetch();
        _CreatureGUIDs[fields[0].GetUInt32()] = count;
    }while (result->NextRow());
    sLog->outString(">> Loaded %u GUIDs.", count);

    //Rebase Creature depending things
    sLog->outString("Creature");
    RebaseCreatures();
    //sLog->outString("CreatureAddon");
    //RebaseCreatureAddons();
    //sLog->outString("CreatureFormations");
    //RebaseCreatureFormations();
    sLog->outString("LinkedRespawn");
    RebaseLinkedRespawn();
}

void WorldDatabaseRebase::PreProcess()
{
    WorldDatabase.DirectPExecute("CREATE TABLE temp_creature SELECT * FROM creature");
    //WorldDatabase.DirectPExecute("CREATE TABLE temp_creature_addon SELECT * FROM creature_addon");
    //WorldDatabase.DirectPExecute("CREATE TABLE temp_creature_formations SELECT * FROM creature_formations");
    WorldDatabase.DirectPExecute("CREATE TABLE temp_linked_respawn SELECT * FROM linked_respawn");

    WorldDatabase.DirectExecute("TRUNCATE temp_creature");
    //WorldDatabase.DirectExecute("TRUNCATE creature_addon");
    //WorldDatabase.DirectExecute("TRUNCATE creature_formations");
    WorldDatabase.DirectExecute("TRUNCATE temp_linked_respawn");
}

void WorldDatabaseRebase::RebaseCreatures()
{
    uint64 new_guid;
    uint32 counter = 0;
    std::ostringstream ss;

    QueryResult result = WorldDatabase.Query("SELECT * FROM creature");
    if (!result)
    {
        sLog->outString();
        sLog->outString(">> no temp_creature found!");
        return;
    }

    barGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();
        CreatureMap::const_iterator itr = _CreatureGUIDs.find(fields[0].GetUInt32());

        if (itr == _CreatureGUIDs.end())
            continue;

        bar.step();
        new_guid = itr->second;

        if (counter == 0)
            ss << "INSERT IGNORE INTO temp_creature VALUES" << sRebase->SingleGUID(0, new_guid, fields, result) << "\n";
        else
            ss << "," << sRebase->SingleGUID(0, new_guid, fields, result) << "\n";

        counter ++;

        if (counter == WRAPS / 10)
        {
            WorldDatabase.PExecute(ss.str().c_str());
            counter = 0;
            ss.str("");
        }
    }
    while (result->NextRow());
    WorldDatabase.PExecute(ss.str().c_str());

}

void WorldDatabaseRebase::RebaseCreatureAddons()
{
    uint64 new_guid;
    uint32 counter = 0;
    std::ostringstream ss;

    QueryResult result = WorldDatabase.Query("SELECT * FROM temp_creature_addon");
    if (!result)
    {
        sLog->outString();
        sLog->outString(">> no temp_creature_addon found!");
        return;
    }

    barGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();
        CreatureMap::const_iterator itr = _CreatureGUIDs.find(fields[0].GetUInt32());

        if (itr == _CreatureGUIDs.end())
            continue;

        bar.step();
        new_guid = itr->second;

        if (counter == 0)
            ss << "INSERT IGNORE INTO creature_addon VALUES" << sRebase->SingleGUID(0, new_guid, fields, result) << "\n";
        else
            ss << "," << sRebase->SingleGUID(0, new_guid, fields, result) << "\n";

        counter ++;

        if (counter == WRAPS / 10)
        {
            WorldDatabase.PExecute(ss.str().c_str());
            counter = 0;
            ss.str("");
        }
    }
    while (result->NextRow());
    WorldDatabase.PExecute(ss.str().c_str());
}

void WorldDatabaseRebase::RebaseCreatureFormations()
{
    uint32 counter = 0;
    std::ostringstream ss;

    QueryResult result = WorldDatabase.Query("SELECT * FROM temp_creature_formations");
    if (!result)
    {
        sLog->outString();
        sLog->outString(">> no temp_creature_formations found!");
        return;
    }

    barGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();

        bar.step();

        CreatureMap::const_iterator itr = _CreatureGUIDs.find(fields[0].GetUInt32());
        CreatureMap::const_iterator iter_two = _CreatureGUIDs.find(fields[1].GetUInt32());
        if (itr == _CreatureGUIDs.end() || iter_two == _CreatureGUIDs.end())
            continue;

        if (counter == 0)
            ss << "INSERT IGNORE INTO creature_formations VALUES" << sRebase->DoubleGUID(0, itr->second, 1, iter_two->second, fields, result) << "\n";
        else
            ss << "," << sRebase->DoubleGUID(0, itr->second, 1, iter_two->second, fields, result) << "\n";

        counter ++;

        if (counter == WRAPS / 10)
        {
            WorldDatabase.PExecute(ss.str().c_str());
            counter = 0;
            ss.str("");
        }
    }
    while (result->NextRow());
    WorldDatabase.PExecute(ss.str().c_str());

}

void WorldDatabaseRebase::RebaseLinkedRespawn()
{
    uint64 new_guid;
    uint32 counter = 0;
    std::ostringstream ss;

    QueryResult result = WorldDatabase.Query("SELECT * FROM linked_respawn");
    if (!result)
    {
        sLog->outString();
        sLog->outString(">> no temp_linked_respawn found!");
        return;
    }

    barGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();
        CreatureMap::const_iterator itr = _CreatureGUIDs.find(fields[0].GetUInt32());

        if (itr == _CreatureGUIDs.end())
            continue;

        bar.step();
        new_guid = itr->second;

        if (counter == 0)
            ss << "INSERT IGNORE INTO temp_linked_respawn VALUES" << sRebase->SingleGUID(0, new_guid, fields, result) << "\n";
        else
            ss << "," << sRebase->SingleGUID(0, new_guid, fields, result) << "\n";

        counter ++;

        if (counter == WRAPS / 10)
        {
            WorldDatabase.PExecute(ss.str().c_str());
            counter = 0;
            ss.str("");
        }
    }
    while (result->NextRow());
    WorldDatabase.PExecute(ss.str().c_str());
}
