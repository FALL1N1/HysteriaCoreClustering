#include "PlayerAPI.h"
#include "ArenaTeamMgr.h"
#include "Player.h"
#include "Log.h"

PlayerAPI::PlayerAPI() { }

void PlayerAPI::DeletePlayer(uint64 guid)
{
    return;
    if (!sWorld->getBoolConfig(CONFIG_API_DB_ENABLED))
        return;

    PreparedStatement* stmt = stmt = APIDatabase.GetPreparedStatement(API_DEL_CHAR_STATS);
    stmt->setUInt32(0, guid);

    APIDatabase.Execute(stmt);
    sLog->outAPI("Deleted entry %u from API database, characters table.", guid);
}

void PlayerAPI::UpdateLastLootedItems(Player* player)
{
    return;
    if (!sWorld->getBoolConfig(CONFIG_API_DB_ENABLED))
        return;

    uint8 queryid, index = 0;

    // check how many items should we update?
    if (player->GetAPILastLootedItem(1) != "0" && player->GetAPILastLootedItem(1) != "")
        queryid = 1;
    if (player->GetAPILastLootedItem(2) != "0" && player->GetAPILastLootedItem(2) != "")
        queryid = 2;
    if (player->GetAPILastLootedItem(3) != "0" && player->GetAPILastLootedItem(3) != "")
        queryid = 3;
    if (player->GetAPILastLootedItem(4) != "0" && player->GetAPILastLootedItem(4) != "")
        queryid = 4;
    if (player->GetAPILastLootedItem(5) != "0" && player->GetAPILastLootedItem(5) != "")
        queryid = 5;

    if (queryid == 0)
        return;


    // begin
    switch (queryid)
    {
        case 5:
        {
            PreparedStatement* stmt = APIDatabase.GetPreparedStatement(API_UPD_PLAYER_LOOTED_ITEMS_5);
            stmt->setUInt32(index++, player->GetGUIDLow()); // always first
            stmt->setString(index++, player->GetName()); // always second

            // begin items [5]
            stmt->setString(index++, player->GetAPILastLootedItem(5));
            stmt->setString(index++, player->GetAPILastLootedItem(4));
            stmt->setString(index++, player->GetAPILastLootedItem(3));
            stmt->setString(index++, player->GetAPILastLootedItem(2));
            stmt->setString(index++, player->GetAPILastLootedItem(1));
            APIDatabase.Execute(stmt);
            break;
        }
        case 4:
        {
            PreparedStatement* stmt = APIDatabase.GetPreparedStatement(API_UPD_PLAYER_LOOTED_ITEMS_4);
            stmt->setUInt32(index++, player->GetGUIDLow()); // always first
            stmt->setString(index++, player->GetName()); // always second

            // begin items [4]
            stmt->setString(index++, player->GetAPILastLootedItem(4));
            stmt->setString(index++, player->GetAPILastLootedItem(3));
            stmt->setString(index++, player->GetAPILastLootedItem(2));
            stmt->setString(index++, player->GetAPILastLootedItem(1)); 
            APIDatabase.Execute(stmt);
            break;
        }
        case 3:
        {
            PreparedStatement* stmt = APIDatabase.GetPreparedStatement(API_UPD_PLAYER_LOOTED_ITEMS_3);
            stmt->setUInt32(index++, player->GetGUIDLow()); // always first
            stmt->setString(index++, player->GetName()); // always second

            // begin items [3]
            stmt->setString(index++, player->GetAPILastLootedItem(3));
            stmt->setString(index++, player->GetAPILastLootedItem(2));
            stmt->setString(index++, player->GetAPILastLootedItem(1));
            APIDatabase.Execute(stmt);
            break;
        }
        case 2:
        {
            PreparedStatement* stmt = APIDatabase.GetPreparedStatement(API_UPD_PLAYER_LOOTED_ITEMS_1);
            stmt->setUInt32(index++, player->GetGUIDLow()); // always first
            stmt->setString(index++, player->GetName()); // always second

            // begin items [2]
            stmt->setString(index++, player->GetAPILastLootedItem(2));
            stmt->setString(index++, player->GetAPILastLootedItem(1));
            APIDatabase.Execute(stmt);
            break;
        }
        case 1:
        {
            PreparedStatement* stmt = APIDatabase.GetPreparedStatement(API_UPD_PLAYER_LOOTED_ITEMS_1); 
            stmt->setUInt32(index++, player->GetGUIDLow()); // always first
            stmt->setString(index++, player->GetName()); // always second

            // begin items [1]
            stmt->setString(index++, player->GetAPILastLootedItem(1));
            APIDatabase.Execute(stmt);
            break;
        } 

        default:
            break;
    }
}
 
void PlayerAPI::UpdatePlayer(Player* player, uint16 achPts)
{ 
    return;
    if (!sWorld->getBoolConfig(CONFIG_API_DB_ENABLED))
        return;

    PreparedStatement* stmt = APIDatabase.GetPreparedStatement(API_INS_CHAR_STATS);
    uint8 index = 0;    
    std::string teamName1, teamName2, teamName3,
                teamRating1, teamRating2, teamRating3,
                personalRating1, personalRating2, personalRating3,
                combined1, combined2, combined3 = "";


    stmt->setUInt32(index++, player->GetGUIDLow());
    stmt->setUInt32(index++, player->GetMaxHealth());

    for (uint8 i = 0; i < MAX_POWERS; ++i)
        stmt->setUInt32(index++, player->GetMaxPower(Powers(i)));

    for (uint8 i = 0; i < MAX_STATS; ++i)
        stmt->setUInt32(index++, player->GetStat(Stats(i)));

    for (int i = 0; i < MAX_SPELL_SCHOOL; ++i)
        stmt->setUInt32(index++, player->GetResistance(SpellSchools(i)));

    // stats
    stmt->setFloat(index++, player->GetFloatValue(PLAYER_BLOCK_PERCENTAGE));
    stmt->setFloat(index++, player->GetFloatValue(PLAYER_DODGE_PERCENTAGE));
    stmt->setFloat(index++, player->GetFloatValue(PLAYER_PARRY_PERCENTAGE));
    stmt->setFloat(index++, player->GetFloatValue(PLAYER_CRIT_PERCENTAGE));
    stmt->setFloat(index++, player->GetFloatValue(PLAYER_RANGED_CRIT_PERCENTAGE));
    stmt->setFloat(index++, player->GetFloatValue(PLAYER_SPELL_CRIT_PERCENTAGE1));
    stmt->setUInt32(index++, player->GetUInt32Value(UNIT_FIELD_ATTACK_POWER));
    stmt->setUInt32(index++, player->GetUInt32Value(UNIT_FIELD_RANGED_ATTACK_POWER));
    stmt->setUInt32(index++, player->GetBaseSpellPowerBonus());
    stmt->setUInt32(index++, player->GetUInt32Value(PLAYER_FIELD_COMBAT_RATING_1 + CR_CRIT_TAKEN_SPELL));

    // ach pts
    stmt->setUInt32(index++, achPts);

    // name, guildname, active spec
    stmt->setString(index++, player->GetName());
    if (player->GetGuild())
    {
        stmt->setString(index++, player->GetGuildName());
        stmt->setUInt32(index++, player->GetGuildId());
    }
    else
    {
        stmt->setString(index++, "");
        stmt->setUInt32(index++, 0);
    }
    stmt->setUInt32(index++, player->GetActiveSpec());

    // location
    stmt->setUInt32(index++, player->GetMapId());
    stmt->setUInt32(index++, player->GetZoneId());
    stmt->setUInt32(index++, player->GetAreaId());
     
    // Save Arena Team Data [2v2, 3v3, 5v5]
    // 2v2
    if (ArenaTeam* arenaTeam = sArenaTeamMgr->GetArenaTeamById(1))
    {
        teamName1 = arenaTeam->GetName();
        teamRating1 = std::to_string(arenaTeam->GetRating());
        personalRating1 = std::to_string(player->GetArenaPersonalRating(1));
        combined1 = teamName1 + ":" + teamRating1 + ":" + personalRating1;
    }

    // 3v3
    if (ArenaTeam* arenaTeam = sArenaTeamMgr->GetArenaTeamById(2))
    {
        teamName2 = arenaTeam->GetName();
        teamRating2 = std::to_string(arenaTeam->GetRating());
        personalRating2 = std::to_string(player->GetArenaPersonalRating(2));
        combined2 = teamName2 + ":" + teamRating2 + ":" + personalRating2;
    }

    // 5v5
    if (ArenaTeam* arenaTeam = sArenaTeamMgr->GetArenaTeamById(3))
    {
        teamName3 = arenaTeam->GetName();
        teamRating3 = std::to_string(arenaTeam->GetRating());
        personalRating3 = std::to_string(player->GetArenaPersonalRating(3));
        combined3 = teamName3 + ":" + teamRating3 + ":" + personalRating3;
    } 

    stmt->setString(index++, combined1);
    stmt->setString(index++, combined2);
    stmt->setString(index++, combined3);

    // race, class
    stmt->setUInt32(index++, player->getRace());
    stmt->setUInt32(index++, player->getClass());
    stmt->setUInt32(index++, player->getGender());

    // 6 entries for glyphs
    for (uint8 i = 0; i < 6; ++i)
    {
                if (uint32 glyphId = player->GetGlyph(i))
                {
                    if (GlyphPropertiesEntry const* glyph = sGlyphPropertiesStore.LookupEntry(glyphId))
                    { 

                        stmt->setUInt32(index++, glyph->SpellId);
                    }
                }
        else stmt->setUInt32(index++, 0);
    }

    // level, hp, power
    stmt->setUInt32(index++, player->getLevel());

    // played time, level played time
    stmt->setUInt32(index++, player->GetTotalPlayedTime());
    stmt->setUInt32(index++, player->GetLevelPlayedTime());

    // money
    stmt->setUInt32(index++, player->GetMoney());


    // REWRITE ASAP
    // @emo
    std::string varstr;
    QueryResult achquery = CharacterDatabase.PQuery("SELECT achievement,date FROM character_achievement WHERE guid = %u ORDER BY date DESC limit 5", player->GetGUIDLow());
    if (achquery)
    {
        do
        {
            Field* fldz = achquery->Fetch();
            if (fldz)
            {
                varstr = std::to_string(fldz[0].GetUInt16()) + ":" + std::to_string(fldz[1].GetUInt32());


                stmt->setString(index++,varstr);
            } else stmt->setString(index++, "NULL");
        } while (achquery->NextRow());
    }   


    // GENERATE PRIMARY PROFESSIONS
    std::string p, p1, p2 = "";
    bool sndP = false;
    uint32 sk1, sk2, sk3, sk4, sk5 = 0;

    if (player->HasSkill(SKILL_ALCHEMY))
    {
        if (!sndP) { p1 = "Alchemy"; sndP = true; sk1 = player->GetSkillValue(SKILL_ALCHEMY); }
        else { p2 = "Alchemy"; sk2 = player->GetSkillValue(SKILL_ALCHEMY); }
    }
    if (player->HasSkill(SKILL_BLACKSMITHING))
    {
        if (!sndP) { p1 = "Blacksmithing"; sndP = true; sk1 = player->GetSkillValue(SKILL_BLACKSMITHING); }
        else { p2 = "Blacksmithing"; sk2 = player->GetSkillValue(SKILL_BLACKSMITHING); }
    }
    if (player->HasSkill(SKILL_ENCHANTING))
    {
        if (!sndP) { p1 = "Enchanting"; sndP = true; sk1 = player->GetSkillValue(SKILL_ENCHANTING); }
        else { p2 = "Enchanting"; sk2 = player->GetSkillValue(SKILL_ENCHANTING); }
    }
    if (player->HasSkill(SKILL_ENGINEERING))
    {
        if (!sndP) { p1 = "Engineering"; sndP = true; sk1 = player->GetSkillValue(SKILL_ENGINEERING); }
        else { p2 = "Engineering"; sk2 = player->GetSkillValue(SKILL_ENGINEERING); }
    }
    if (player->HasSkill(SKILL_HERBALISM))
    {
        if (!sndP) { p1 = "Herbalism"; sndP = true; sk1 = player->GetSkillValue(SKILL_HERBALISM); }
        else { p2 = "Herbalism"; sk2 = player->GetSkillValue(SKILL_HERBALISM); }
    }
    if (player->HasSkill(SKILL_MINING))
    {
        if (!sndP) { p1 = "Mining"; sndP = true; sk1 = player->GetSkillValue(SKILL_MINING); }
        else { p2 = "Mining"; sk2 = player->GetSkillValue(SKILL_MINING); }
    }
    if (player->HasSkill(SKILL_INSCRIPTION))
    {
        if (!sndP) { p1 = "Inscription"; sndP = true; sk1 = player->GetSkillValue(SKILL_INSCRIPTION); }
        else { p2 = "Inscription"; sk2 = player->GetSkillValue(SKILL_INSCRIPTION); }
    }
    if (player->HasSkill(SKILL_JEWELCRAFTING))
    {
        if (!sndP) { p1 = "Jewelcrafting"; sndP = true; sk1 = player->GetSkillValue(SKILL_JEWELCRAFTING); }
        else { p2 = "Jewelcrafting"; sk2 = player->GetSkillValue(SKILL_JEWELCRAFTING); }
    }
    if (player->HasSkill(SKILL_LEATHERWORKING))
    {
        if (!sndP) { p1 = "Leatherworking"; sndP = true; sk1 = player->GetSkillValue(SKILL_LEATHERWORKING); }
        else { p2 = "Leatherworking"; sk2 = player->GetSkillValue(SKILL_LEATHERWORKING); }
    }
    if (player->HasSkill(SKILL_TAILORING))
    {
        if (!sndP) { p1 = "Tailoring"; sndP = true; sk1 = player->GetSkillValue(SKILL_TAILORING); }
        else { p2 = "Tailoring"; sk2 = player->GetSkillValue(SKILL_TAILORING); }
    }

    // GENERATE SECONDARY PROFESSIONS
    if (player->HasSkill(SKILL_COOKING))
        sk3 = player->GetSkillValue(SKILL_COOKING);
    if (player->HasSkill(SKILL_FISHING))
        sk4 = player->GetSkillValue(SKILL_FISHING);
    if (player->HasSkill(SKILL_FIRST_AID))
        sk5 = player->GetSkillValue(SKILL_FIRST_AID);

    p = p1 + ":" + p2; // final

    // append in db
    stmt->setString(index++, p);
    stmt->setUInt32(index++, sk1);
    stmt->setUInt32(index++, sk2);
    stmt->setUInt32(index++, sk3);
    stmt->setUInt32(index++, sk4);
    stmt->setUInt32(index++, sk5);
    // professions end
    ///////////////////////////// 


    APIDatabase.Execute(stmt);
    // after we create/update player entry, update other data 
    UpdateInventory(player);
    UpdateLastLootedItems(player);
    sLog->outAPI("[PlayerAPI] Updated stats for player %s(%u).", player->GetName().c_str(), player->GetGUIDLow());
}  

void PlayerAPI::UpdateInventory(Player* player)
{
    return;
    if (!sWorld->getBoolConfig(CONFIG_API_DB_ENABLED))
        return;

    uint32 
    item1_entry, item1_enchant, item1_gem_1, item1_gem_2, item1_gem_3,
    item2_entry, item2_enchant, item2_gem_1, item2_gem_2, item2_gem_3,
    item3_entry, item3_enchant, item3_gem_1, item3_gem_2, item3_gem_3,
    item4_entry, item4_enchant, item4_gem_1, item4_gem_2, item4_gem_3,
    item5_entry, item5_enchant, item5_gem_1, item5_gem_2, item5_gem_3,
    item6_entry, item6_enchant, item6_gem_1, item6_gem_2, item6_gem_3,
    item7_entry, item7_enchant, item7_gem_1, item7_gem_2, item7_gem_3,
    item8_entry, item8_enchant, item8_gem_1, item8_gem_2, item8_gem_3,
    item9_entry, item9_enchant, item9_gem_1, item9_gem_2, item9_gem_3,
    item10_entry, item10_enchant, item10_gem_1, item10_gem_2, item10_gem_3,
    item11_entry, item11_enchant, item11_gem_1, item11_gem_2, item11_gem_3,
    item12_entry, item12_enchant, item12_gem_1, item12_gem_2, item12_gem_3,
    item13_entry, item13_enchant, item13_gem_1, item13_gem_2, item13_gem_3,
    item14_entry, item14_enchant, item14_gem_1, item14_gem_2, item14_gem_3,
    item15_entry, item15_enchant, item15_gem_1, item15_gem_2, item15_gem_3,
    item16_entry, item16_enchant, item16_gem_1, item16_gem_2, item16_gem_3,
    item17_entry, item17_enchant, item17_gem_1, item17_gem_2, item17_gem_3,
    item18_entry, item18_enchant, item18_gem_1, item18_gem_2, item18_gem_3,
    item19_entry, item19_enchant, item19_gem_1, item19_gem_2, item19_gem_3 = 0;


    for (uint8 s = EQUIPMENT_SLOT_START; s < EQUIPMENT_SLOT_END;)
    {
        if (Item* i = player->GetItemByPos(INVENTORY_SLOT_BAG_0, s))
        { 
            switch(s)
            {
                case EQUIPMENT_SLOT_HEAD:
                    item1_entry = i->GetEntry();
                    item1_enchant = i->GetEnchantmentId(PERM_ENCHANTMENT_SLOT);
                    item1_gem_1 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT);
                    item1_gem_2 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2);
                    item1_gem_3 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3); 
                    break;
                case EQUIPMENT_SLOT_NECK:
                    item2_entry = i->GetEntry();
                    item2_enchant = i->GetEnchantmentId(PERM_ENCHANTMENT_SLOT);
                    item2_gem_1 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT);
                    item2_gem_2 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2);
                    item2_gem_3 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3); 
                    break;
                case EQUIPMENT_SLOT_SHOULDERS:
                    item3_entry = i->GetEntry();
                    item3_enchant = i->GetEnchantmentId(PERM_ENCHANTMENT_SLOT);
                    item3_gem_1 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT);
                    item3_gem_2 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2);
                    item3_gem_3 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3); 
                    break;
                case EQUIPMENT_SLOT_BODY:
                    item4_entry = i->GetEntry();
                    item4_enchant = i->GetEnchantmentId(PERM_ENCHANTMENT_SLOT);
                    item4_gem_1 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT);
                    item4_gem_2 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2);
                    item4_gem_3 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3); 
                    break;
                case EQUIPMENT_SLOT_CHEST:
                    item5_entry = i->GetEntry();
                    item5_enchant = i->GetEnchantmentId(PERM_ENCHANTMENT_SLOT);
                    item5_gem_1 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT);
                    item5_gem_2 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2);
                    item5_gem_3 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3); 
                    break;
                case EQUIPMENT_SLOT_WAIST:
                    item6_entry = i->GetEntry();
                    item6_enchant = i->GetEnchantmentId(PERM_ENCHANTMENT_SLOT);
                    item6_gem_1 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT);
                    item6_gem_2 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2);
                    item6_gem_3 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3); 
                    break;
                case EQUIPMENT_SLOT_LEGS:
                    item7_entry = i->GetEntry();
                    item7_enchant = i->GetEnchantmentId(PERM_ENCHANTMENT_SLOT);
                    item7_gem_1 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT);
                    item7_gem_2 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2);
                    item7_gem_3 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3); 
                    break;
                case EQUIPMENT_SLOT_FEET:
                    item8_entry = i->GetEntry();
                    item8_enchant = i->GetEnchantmentId(PERM_ENCHANTMENT_SLOT);
                    item8_gem_1 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT);
                    item8_gem_2 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2);
                    item8_gem_3 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3); 
                    break;
                case EQUIPMENT_SLOT_WRISTS:
                    item9_entry = i->GetEntry();
                    item9_enchant = i->GetEnchantmentId(PERM_ENCHANTMENT_SLOT);
                    item9_gem_1 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT);
                    item9_gem_2 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2);
                    item9_gem_3 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3); 
                    break;
                case EQUIPMENT_SLOT_HANDS:
                    item10_entry = i->GetEntry();
                    item10_enchant = i->GetEnchantmentId(PERM_ENCHANTMENT_SLOT);
                    item10_gem_1 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT);
                    item10_gem_2 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2);
                    item10_gem_3 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3); 
                    break;
                case EQUIPMENT_SLOT_FINGER1:
                    item11_entry = i->GetEntry();
                    item11_enchant = i->GetEnchantmentId(PERM_ENCHANTMENT_SLOT);
                    item11_gem_1 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT);
                    item11_gem_2 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2);
                    item11_gem_3 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3); 
                    break;
                case EQUIPMENT_SLOT_FINGER2:
                    item12_entry = i->GetEntry();
                    item12_enchant = i->GetEnchantmentId(PERM_ENCHANTMENT_SLOT);
                    item12_gem_1 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT);
                    item12_gem_2 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2);
                    item12_gem_3 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3); 
                    break;
                case EQUIPMENT_SLOT_TRINKET1:
                    item13_entry = i->GetEntry();
                    item13_enchant = i->GetEnchantmentId(PERM_ENCHANTMENT_SLOT);
                    item13_gem_1 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT);
                    item13_gem_2 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2);
                    item13_gem_3 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3); 
                    break;
                case EQUIPMENT_SLOT_TRINKET2:
                    item14_entry = i->GetEntry();
                    item14_enchant = i->GetEnchantmentId(PERM_ENCHANTMENT_SLOT);
                    item14_gem_1 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT);
                    item14_gem_2 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2);
                    item14_gem_3 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3); 
                    break;
                case EQUIPMENT_SLOT_BACK:
                    item15_entry = i->GetEntry();
                    item15_enchant = i->GetEnchantmentId(PERM_ENCHANTMENT_SLOT);
                    item15_gem_1 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT);
                    item15_gem_2 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2);
                    item15_gem_3 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3); 
                    break;
                case EQUIPMENT_SLOT_MAINHAND:
                    item16_entry = i->GetEntry();
                    item16_enchant = i->GetEnchantmentId(PERM_ENCHANTMENT_SLOT);
                    item16_gem_1 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT);
                    item16_gem_2 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2);
                    item16_gem_3 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3); 
                    break;
                case EQUIPMENT_SLOT_OFFHAND:
                    item17_entry = i->GetEntry();
                    item17_enchant = i->GetEnchantmentId(PERM_ENCHANTMENT_SLOT);
                    item17_gem_1 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT);
                    item17_gem_2 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2);
                    item17_gem_3 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3); 
                    break;
                case EQUIPMENT_SLOT_RANGED:
                    item18_entry = i->GetEntry();
                    item18_enchant = i->GetEnchantmentId(PERM_ENCHANTMENT_SLOT);
                    item18_gem_1 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT);
                    item18_gem_2 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2);
                    item18_gem_3 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3); 
                    break;
                case EQUIPMENT_SLOT_TABARD:
                    item19_entry = i->GetEntry();
                    item19_enchant = i->GetEnchantmentId(PERM_ENCHANTMENT_SLOT);
                    item19_gem_1 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT);
                    item19_gem_2 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2);
                    item19_gem_3 = i->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3);
                    break;
                default:
                    sLog->outAPI("ERROR WHILE FETCHING ITEM(%u WITH SLOT %u), DEFAULT CASE HIT!", i->GetEntry(), s);
                    break;
            }

        }
        s++;
    }


    PreparedStatement* stmt = APIDatabase.GetPreparedStatement(API_UPD_PLAYER_ITEMS);
    uint8 index = 0;
    // plr guid
    stmt->setUInt32(index++, player->GetGUIDLow());
    stmt->setUInt32(index++, item1_entry);
    stmt->setUInt32(index++, item1_gem_1);
    stmt->setUInt32(index++, item1_gem_2);
    stmt->setUInt32(index++, item1_gem_3);
    stmt->setUInt32(index++, item1_enchant);
    stmt->setUInt32(index++, item2_entry);
    stmt->setUInt32(index++, item2_gem_1);
    stmt->setUInt32(index++, item2_gem_2);
    stmt->setUInt32(index++, item2_gem_3);
    stmt->setUInt32(index++, item2_enchant);
    stmt->setUInt32(index++, item3_entry);
    stmt->setUInt32(index++, item3_gem_1);
    stmt->setUInt32(index++, item3_gem_2);
    stmt->setUInt32(index++, item3_gem_3);
    stmt->setUInt32(index++, item3_enchant);
    stmt->setUInt32(index++, item4_entry);
    stmt->setUInt32(index++, item4_gem_1);
    stmt->setUInt32(index++, item4_gem_2);
    stmt->setUInt32(index++, item4_gem_3);
    stmt->setUInt32(index++, item4_enchant);
    stmt->setUInt32(index++, item5_entry);
    stmt->setUInt32(index++, item5_gem_1);
    stmt->setUInt32(index++, item5_gem_2);
    stmt->setUInt32(index++, item5_gem_3);
    stmt->setUInt32(index++, item5_enchant);
    stmt->setUInt32(index++, item6_entry);
    stmt->setUInt32(index++, item6_gem_1);
    stmt->setUInt32(index++, item6_gem_2);
    stmt->setUInt32(index++, item6_gem_3);
    stmt->setUInt32(index++, item6_enchant);
    stmt->setUInt32(index++, item7_entry);
    stmt->setUInt32(index++, item7_gem_1);
    stmt->setUInt32(index++, item7_gem_2);
    stmt->setUInt32(index++, item7_gem_3);
    stmt->setUInt32(index++, item7_enchant);
    stmt->setUInt32(index++, item8_entry);
    stmt->setUInt32(index++, item8_gem_1);
    stmt->setUInt32(index++, item8_gem_2);
    stmt->setUInt32(index++, item8_gem_3);
    stmt->setUInt32(index++, item8_enchant);
    stmt->setUInt32(index++, item9_entry);
    stmt->setUInt32(index++, item9_gem_1);
    stmt->setUInt32(index++, item9_gem_2);
    stmt->setUInt32(index++, item9_gem_3);
    stmt->setUInt32(index++, item9_enchant);
    stmt->setUInt32(index++, item10_entry);
    stmt->setUInt32(index++, item10_gem_1);
    stmt->setUInt32(index++, item10_gem_2);
    stmt->setUInt32(index++, item10_gem_3);
    stmt->setUInt32(index++, item10_enchant);
    stmt->setUInt32(index++, item11_entry);
    stmt->setUInt32(index++, item11_gem_1);
    stmt->setUInt32(index++, item11_gem_2);
    stmt->setUInt32(index++, item11_gem_3);
    stmt->setUInt32(index++, item11_enchant);
    stmt->setUInt32(index++, item12_entry);
    stmt->setUInt32(index++, item12_gem_1);
    stmt->setUInt32(index++, item12_gem_2);
    stmt->setUInt32(index++, item12_gem_3);
    stmt->setUInt32(index++, item12_enchant);
    stmt->setUInt32(index++, item13_entry);
    stmt->setUInt32(index++, item13_gem_1);
    stmt->setUInt32(index++, item13_gem_2);
    stmt->setUInt32(index++, item13_gem_3);
    stmt->setUInt32(index++, item13_enchant);
    stmt->setUInt32(index++, item14_entry);
    stmt->setUInt32(index++, item14_gem_1);
    stmt->setUInt32(index++, item14_gem_2);
    stmt->setUInt32(index++, item14_gem_3);
    stmt->setUInt32(index++, item14_enchant);
    stmt->setUInt32(index++, item15_entry);
    stmt->setUInt32(index++, item15_gem_1);
    stmt->setUInt32(index++, item15_gem_2);
    stmt->setUInt32(index++, item15_gem_3);
    stmt->setUInt32(index++, item15_enchant);
    stmt->setUInt32(index++, item16_entry);
    stmt->setUInt32(index++, item16_gem_1);
    stmt->setUInt32(index++, item16_gem_2);
    stmt->setUInt32(index++, item16_gem_3);
    stmt->setUInt32(index++, item16_enchant);
    stmt->setUInt32(index++, item17_entry);
    stmt->setUInt32(index++, item17_gem_1);
    stmt->setUInt32(index++, item17_gem_2);
    stmt->setUInt32(index++, item17_gem_3);
    stmt->setUInt32(index++, item17_enchant);
    stmt->setUInt32(index++, item18_entry);
    stmt->setUInt32(index++, item18_gem_1);
    stmt->setUInt32(index++, item18_gem_2);
    stmt->setUInt32(index++, item18_gem_3);
    stmt->setUInt32(index++, item18_enchant);
    stmt->setUInt32(index++, item19_entry);
    stmt->setUInt32(index++, item19_gem_1);
    stmt->setUInt32(index++, item19_gem_2);
    stmt->setUInt32(index++, item19_gem_3);
    stmt->setUInt32(index++, item19_enchant);
    APIDatabase.Execute(stmt);

    sLog->outAPI("[PlayerAPI] Updated inventory for player %s(%u).", player->GetName().c_str(), player->GetGUIDLow());
}

