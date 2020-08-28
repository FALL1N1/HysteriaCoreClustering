#include "APIDatabase.h"

void APIDatabaseConnection::DoPrepareStatements()
{
    if (!m_reconnecting)
        m_stmts.resize(MAX_APIDATABASE_STATEMENTS);

    // PlayerAPI
    PrepareStatement(API_DEL_CHAR_STATS, "DELETE FROM characters WHERE guid = ?", CONNECTION_ASYNC);
    PrepareStatement(API_INS_CHAR_STATS, "REPLACE INTO characters ("
        "guid, maxhealth, maxpower1, maxpower2, maxpower3, maxpower4, maxpower5, maxpower6, maxpower7, strength, "
        "agility, stamina, intellect, spirit, armor, resHoly, resFire, resNature, resFrost, resShadow, "
        "resArcane, blockPct, dodgePct, parryPct, critPct, rangedCritPct, spellCritPct, attackPower, rangedAttackPower, spellPower, "
        "resilience, achievementPoints, name, guildname, guildid, activespec, map, zone, area, lastseen, "
        "2v2, 3v3, 5v5, race, class, gender, glyph1, glyph2, glyph3, glyph4, "
        "glyph5, glyph6, level, totaltime, leveltime, money, ach1, ach2, ach3, ach4, ach5, proff, proff1skill, proff2skill, proff3skill, proff4skill, proff5skill)"
        " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, NOW(), "
        "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", CONNECTION_ASYNC);

    PrepareStatement(API_INS_PLAYER_TALENT, "INSERT INTO characters_talents (guid, spell, specMask) VALUES (?, ?, ?)", CONNECTION_ASYNC);
    PrepareStatement(API_DEL_PLAYER_TALENT_BY_SPELL, "DELETE FROM characters_talents WHERE guid = ? and spell = ?", CONNECTION_ASYNC);

    PrepareStatement(API_UPD_PLAYER_ITEMS, "REPLACE INTO characters_inventories(characterguid, "
        " item1_id, item1_gem_1, item1_gem_2, item1_gem_3, item1_enchant, "
        " item2_id, item2_gem_1, item2_gem_2, item2_gem_3, item2_enchant, "
        " item3_id, item3_gem_1, item3_gem_2, item3_gem_3, item3_enchant, "
        " item4_id, item4_gem_1, item4_gem_2, item4_gem_3, item4_enchant, "
        " item5_id, item5_gem_1, item5_gem_2, item5_gem_3, item5_enchant, "
        " item6_id, item6_gem_1, item6_gem_2, item6_gem_3, item6_enchant, "
        " item7_id, item7_gem_1, item7_gem_2, item7_gem_3, item7_enchant, "
        " item8_id, item8_gem_1, item8_gem_2, item8_gem_3, item8_enchant, "
        " item9_id, item9_gem_1, item9_gem_2, item9_gem_3, item9_enchant, "
        " item10_id, item10_gem_1, item10_gem_2, item10_gem_3, item10_enchant, "
        " item11_id, item11_gem_1, item11_gem_2, item11_gem_3, item11_enchant, "
        " item12_id, item12_gem_1, item12_gem_2, item12_gem_3, item12_enchant, "
        " item13_id, item13_gem_1, item13_gem_2, item13_gem_3, item13_enchant, "
        " item14_id, item14_gem_1, item14_gem_2, item14_gem_3, item14_enchant, "
        " item15_id, item15_gem_1, item15_gem_2, item15_gem_3, item15_enchant, "
        " item16_id, item16_gem_1, item16_gem_2, item16_gem_3, item16_enchant, "
        " item17_id, item17_gem_1, item17_gem_2, item17_gem_3, item17_enchant, "
        " item18_id, item18_gem_1, item18_gem_2, item18_gem_3, item18_enchant, "
        " item19_id, item19_gem_1, item19_gem_2, item19_gem_3, item19_enchant, "
        " udate) VALUES(? , "
        " ? , ? , ? , ? , ? , "
        " ? , ? , ? , ? , ? , "
        " ? , ? , ? , ? , ? , "
        " ? , ? , ? , ? , ? , "
        " ? , ? , ? , ? , ? , "
        " ? , ? , ? , ? , ? , "
        " ? , ? , ? , ? , ? , "
        " ? , ? , ? , ? , ? , "
        " ? , ? , ? , ? , ? , "
        " ? , ? , ? , ? , ? , "
        " ? , ? , ? , ? , ? , "
        " ? , ? , ? , ? , ? , "
        " ? , ? , ? , ? , ? , "
        " ? , ? , ? , ? , ? , "
        " ? , ? , ? , ? , ? , "
        " ? , ? , ? , ? , ? , "
        " ? , ? , ? , ? , ? , "
        " ? , ? , ? , ? , ? , "
        " ? , ? , ? , ? , ? , "
               " now())", CONNECTION_ASYNC);

    // ArenaAPI

    PrepareStatement(API_INS_ARENA_GAME, "INSERT INTO arena_fights VALUES (?, NOW(), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", CONNECTION_ASYNC);

    PrepareStatement(API_INS_ARENA_MEMBERSTATS, "REPLACE INTO arena_memberstats  (fight_id, member_id, name, guid, team, account, ip, damage, heal, kblows) VALUES("
                                         "?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", CONNECTION_ASYNC);

    // RaidAPI
    PrepareStatement(API_INS_PVE_KILL, "INSERT INTO pve_fight (start, plrguid, plrname, healer, healingdone, damagedone, boss, bossname, bosshp, finish, fightEntry) VALUES (? , ? , ? , ? , ? , ? , ? , ? , ? , ? , ? )", CONNECTION_ASYNC);








    // Player last looted items

    // one item
    PrepareStatement(API_UPD_PLAYER_LOOTED_ITEMS_1, "INSERT INTO characters_items_history (guid, name, item1, item2, item3, item4, item5) VALUES(?, ?, 0, 0, 0, 0, 0) ON DUPLICATE KEY UPDATE item5 = item4, item4 = item3, item3 = item2, item2 = item1, item1 = ?", CONNECTION_ASYNC);
    // two items
    PrepareStatement(API_UPD_PLAYER_LOOTED_ITEMS_2, "INSERT INTO characters_items_history (guid, name, item1, item2, item3, item4, item5) VALUES(?, ?, 0, 0, 0, 0, 0) ON DUPLICATE KEY UPDATE item5 = item4, item4 = item3, item3 = item2, item2 = ?, item1 = ?", CONNECTION_ASYNC);
    // three items
    PrepareStatement(API_UPD_PLAYER_LOOTED_ITEMS_3, "INSERT INTO characters_items_history (guid, name, item1, item2, item3, item4, item5) VALUES(?, ?, 0, 0, 0, 0, 0) ON DUPLICATE KEY UPDATE item5 = item4, item4 = item3, item3 = ?, item2 = ?, item1 = ?", CONNECTION_ASYNC);
    // four items
    PrepareStatement(API_UPD_PLAYER_LOOTED_ITEMS_4, "INSERT INTO characters_items_history (guid, name, item1, item2, item3, item4, item5) VALUES(?, ?, 0, 0, 0, 0, 0) ON DUPLICATE KEY UPDATE item5 = item4, item4 = ?, item3 = ?, item2 = ?, item1 = ?", CONNECTION_ASYNC);
    // five items
    PrepareStatement(API_UPD_PLAYER_LOOTED_ITEMS_5, "INSERT INTO characters_items_history (guid, name, item1, item2, item3, item4, item5) VALUES(?, ?, 0, 0, 0, 0, 0) ON DUPLICATE KEY UPDATE item5 = ?, item4 = ?, item3 = ?, item2 = ?, item1 = ?", CONNECTION_ASYNC);

}
