#include "Creatures.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "ClientSession.h"
#include "LocalesMgr.h"

void Creatures::WarmingCache()
{
    uint32 oldMSTime = getMSTime();

    //                                                 0              1                 2                  3                 4            5           6        7         8
    QueryResult result = WorldDatabase.Query("SELECT entry, difficulty_entry_1, difficulty_entry_2, difficulty_entry_3, KillCredit1, KillCredit2, modelid1, modelid2, modelid3, "
    //                                           9       10      11       12           13           14        15     16      17          18       19         20         21
                                             "modelid4, name, subname, IconName, gossip_menu_id, minlevel, maxlevel, exp, faction, faction, npcflag, speed_walk, speed_run, "
    //                                         22     23     24     25        26          27             28              29                30           31          32
                                             "scale, rank, mindmg, maxdmg, dmgschool, attackpower, dmg_multiplier, baseattacktime, rangeattacktime, unit_class, unit_flags, "
    //                                             33         34         35             36             37             38          39           40              41           42
                                             "dynamicflags, family, trainer_type, trainer_spell, trainer_class, trainer_race, minrangedmg, maxrangedmg, rangedattackpower, type, "
    //                                            43        44          45           46          47          48           49           50           51           52         53
                                             "type_flags, lootid, pickpocketloot, skinloot, resistance1, resistance2, resistance3, resistance4, resistance5, resistance6, spell1, "
    //                                          54      55      56      57      58      59      60          61            62       63       64       65         66
                                             "spell2, spell3, spell4, spell5, spell6, spell7, spell8, PetSpellDataId, VehicleId, mingold, maxgold, AIName, MovementType, "
    //                                             67          68         69         70          71           72          73          74          75          76          77
                                             "InhabitType, HoverHeight, Health_mod, Mana_mod, Armor_mod, RacialLeader, questItem1, questItem2, questItem3, questItem4, questItem5, "
    //                                            78           79           80               81                82           83
                                             " questItem6, movementId, RegenHealth, faction, mechanic_immune_mask, flags_extra, ScriptName "
                                             "FROM creature_template;");

    if (!result)
    {
        sLog->outString(">> Loaded 0 creature template definitions. DB table `creature_template` is empty.");
        sLog->outString();
        return;
    }

    _creatureTemplateStore.rehash(result->GetRowCount());
    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();


        CreatureTemplate& creatureTemplate = _creatureTemplateStore[entry];

        creatureTemplate.entry = entry;

        for (uint8 i = 0; i < MAX_KILL_CREDIT; ++i)
            creatureTemplate.KillCredit[i] = fields[4 + i].GetUInt32();

        creatureTemplate.Modelid1          = fields[6].GetUInt32();
        creatureTemplate.Modelid2          = fields[7].GetUInt32();
        creatureTemplate.Modelid3          = fields[8].GetUInt32();
        creatureTemplate.Modelid4          = fields[9].GetUInt32();
        creatureTemplate.Name              = fields[10].GetString();
        creatureTemplate.SubName           = fields[11].GetString();
        creatureTemplate.IconName          = fields[12].GetString();
        creatureTemplate.rank              = uint32(fields[23].GetUInt8());
        creatureTemplate.family            = uint32(fields[34].GetUInt8());
        creatureTemplate.type              = uint32(fields[42].GetUInt8());
        creatureTemplate.type_flags        = fields[43].GetUInt32();
        creatureTemplate.RacialLeader      = fields[72].GetBool();

        for (uint8 i = 0; i < MAX_CREATURE_QUEST_ITEMS; ++i)
            creatureTemplate.questItems[i] = fields[73 + i].GetUInt32();

        creatureTemplate.movementId         = fields[79].GetUInt32();

        ++count;
    }
    while (result->NextRow());
    sLog->outString(">> Loaded %u creature templates in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

CreatureTemplate const* Creatures::GetCreatureTemplate(uint32 entry)
{
    CreatureTemplateContainer::const_iterator itr = _creatureTemplateStore.find(entry);
    if (itr != _creatureTemplateStore.end())
        return &(itr->second);

    return NULL;
}

void Creatures::CacheCreatureTemplate(CreatureTemplate* creature)
{
    //To prevent a crash we should set an Mutex here
    TRINITY_WRITE_GUARD(ACE_RW_Thread_Mutex, rwMutex_);

    CreatureTemplate& cre = _creatureTemplateStore[creature->entry];
    cre = *creature;
    return;
}

/// Only _static_ data is sent in this packet !!!
void ClientSession::HandleCreatureQueryOpcode(WorldPacket & recv_data)
{
    uint32 entry;
    recv_data >> entry;
    uint64 guid;
    recv_data >> guid;

    CreatureTemplate const* ci = sCreatures->GetCreatureTemplate(entry);
    if (ci)
    {

        std::string Name, SubName;
        Name = ci->Name;
        SubName = ci->SubName;

        int loc_idx = GetSessionDbLocaleIndex();
        if (loc_idx >= 0)
        {
            if (CreatureLocale const* cl = sLocalesMgr->GetCreatureLocale(entry))
            {
                LocalesMgr::GetLocaleString(cl->Name, loc_idx, Name);
                LocalesMgr::GetLocaleString(cl->SubName, loc_idx, SubName);
            }
        }
        sLog->outDetail("WORLD: CMSG_CREATURE_QUERY '%s' - Entry: %u.", ci->Name.c_str(), entry);
                                                            // guess size
        WorldPacket data(SMSG_CREATURE_QUERY_RESPONSE, 100);
        data << uint32(entry);                              // creature entry
        data << Name;
        data << uint8(0) << uint8(0) << uint8(0);           // name2, name3, name4, always empty
        data << SubName;
        data << ci->IconName;                               // "Directions" for guard, string for Icons 2.3.0
        data << uint32(ci->type_flags);                     // flags
        data << uint32(ci->type);                           // CreatureType.dbc
        data << uint32(ci->family);                         // CreatureFamily.dbc
        data << uint32(ci->rank);                           // Creature Rank (elite, boss, etc)
        data << uint32(ci->KillCredit[0]);                  // new in 3.1, kill credit
        data << uint32(ci->KillCredit[1]);                  // new in 3.1, kill credit
        data << uint32(ci->Modelid1);                       // Modelid1
        data << uint32(ci->Modelid2);                       // Modelid2
        data << uint32(ci->Modelid3);                       // Modelid3
        data << uint32(ci->Modelid4);                       // Modelid4
        data << float(ci->ModHealth);                       // dmg/hp modifier
        data << float(ci->ModMana);                         // dmg/mana modifier
        data << uint8(ci->RacialLeader);
        for (uint32 i = 0; i < MAX_CREATURE_QUEST_ITEMS; ++i)
            data << uint32(ci->questItems[i]);              // itemId[6], quest drop
        data << uint32(ci->movementId);                     // CreatureMovementInfo.dbc
        SendPacket(&data);
        sLog->outString("WORLD: Sent SMSG_CREATURE_QUERY_RESPONSE");
    }
    else
        SendPacketToNode(&recv_data);
}

/// Only _static_ data is sent in this packet !!!
void ClientSession::HandleSMSG_CREATURE_QUERY(WorldPacket & recv_data)
{
    uint8 trash;
    uint32 entry;

    recv_data >> entry;

    //Sollte als nicht existent interpretiert werden, denk ich mal
    if (entry >= 0x80000000)
        return;

    CreatureTemplate *creature = new CreatureTemplate;

    creature->entry = entry;                            // creature entry
    recv_data >> creature->Name;
    recv_data >> trash >> trash >> trash;               // name2, name3, name4, always empty
    recv_data >> creature->SubName;
    recv_data >> creature->IconName;                    // "Directions" for guard, string for Icons 2.3.0
    recv_data >> creature->type_flags;                  // flags
    recv_data >> creature->type;                        // CreatureType.dbc
    recv_data >> creature->family;                      // CreatureFamily.dbc
    recv_data >> creature->rank;                        // Creature Rank (elite, boss, etc)
    recv_data >> creature->KillCredit[0];               // new in 3.1, kill credit
    recv_data >> creature->KillCredit[1];               // new in 3.1, kill credit
    recv_data >> creature->Modelid1;                    // Modelid1
    recv_data >> creature->Modelid2;                    // Modelid2
    recv_data >> creature->Modelid3;                    // Modelid3
    recv_data >> creature->Modelid4;                    // Modelid4
    recv_data >> creature->ModHealth;                   // dmg/hp modifier
    recv_data >> creature->ModMana;                     // dmg/mana modifier
    recv_data >> creature->RacialLeader;
    for (uint32 i = 0; i < MAX_CREATURE_QUEST_ITEMS; ++i)
        recv_data >> creature->questItems[i];           // itemId[6], quest drop
    recv_data >> creature->movementId;                  // CreatureMovementInfo.dbc
    
    sCreatures->CacheCreatureTemplate(creature);

}
