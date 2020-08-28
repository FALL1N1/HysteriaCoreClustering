#include "Items.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "ClientSession.h"
#include "LocalesMgr.h"
#include "Log.h"

void Items::WarmingCache()
{
    uint32 oldMSTime = getMSTime();

    //                                                 0      1       2       3     4        5        6       7          8         9        10        11           12
    QueryResult result = WorldDatabase.Query("SELECT entry, class, subclass, unk0, name, displayid, Quality, Flags, FlagsExtra, BuyCount, BuyPrice, SellPrice, InventoryType, "
    //                                              13              14           15          16             17               18                19              20
                                             "AllowableClass, AllowableRace, ItemLevel, RequiredLevel, RequiredSkill, RequiredSkillRank, requiredspell, requiredhonorrank, "
    //                                              21                      22                       23               24        25          26             27           28
                                             "RequiredCityRank, RequiredReputationFaction, RequiredReputationRank, maxcount, stackable, ContainerSlots, StatsCount, stat_type1, "
    //                                            29           30          31           32          33           34          35           36          37           38
                                             "stat_value1, stat_type2, stat_value2, stat_type3, stat_value3, stat_type4, stat_value4, stat_type5, stat_value5, stat_type6, "
    //                                            39           40          41           42           43          44           45           46           47
                                             "stat_value6, stat_type7, stat_value7, stat_type8, stat_value8, stat_type9, stat_value9, stat_type10, stat_value10, "
    //                                                   48                    49           50        51        52         53        54         55      56      57        58
                                             "ScalingStatDistribution, ScalingStatValue, dmg_min1, dmg_max1, dmg_type1, dmg_min2, dmg_max2, dmg_type2, armor, holy_res, fire_res, "
    //                                            59          60         61          62       63       64            65            66          67               68
                                             "nature_res, frost_res, shadow_res, arcane_res, delay, ammo_type, RangedModRange, spellid_1, spelltrigger_1, spellcharges_1, "
    //                                              69              70                71                 72                 73           74               75
                                             "spellppmRate_1, spellcooldown_1, spellcategory_1, spellcategorycooldown_1, spellid_2, spelltrigger_2, spellcharges_2, "
    //                                              76               77              78                  79                 80           81               82
                                             "spellppmRate_2, spellcooldown_2, spellcategory_2, spellcategorycooldown_2, spellid_3, spelltrigger_3, spellcharges_3, "
    //                                              83               84              85                  86                 87           88               89
                                             "spellppmRate_3, spellcooldown_3, spellcategory_3, spellcategorycooldown_3, spellid_4, spelltrigger_4, spellcharges_4, "
    //                                              90               91              92                  93                  94          95               96
                                             "spellppmRate_4, spellcooldown_4, spellcategory_4, spellcategorycooldown_4, spellid_5, spelltrigger_5, spellcharges_5, "
    //                                              97               98              99                  100                 101        102         103       104          105
                                             "spellppmRate_5, spellcooldown_5, spellcategory_5, spellcategorycooldown_5, bonding, description, PageText, LanguageID, PageMaterial, "
    //                                            106       107     108      109          110            111       112     113         114       115   116     117
                                             "startquest, lockid, Material, sheath, RandomProperty, RandomSuffix, block, itemset, MaxDurability, area, Map, BagFamily, "
    //                                            118             119             120             121             122            123              124            125
                                             "TotemCategory, socketColor_1, socketContent_1, socketColor_2, socketContent_2, socketColor_3, socketContent_3, socketBonus, "
    //                                            126                 127                     128            129            130            131         132         133
                                             "GemProperties, RequiredDisenchantSkill, ArmorDamageModifier, Duration, ItemLimitCategory, HolidayId, ScriptName, DisenchantID, "
    //                                           134        135            136
                                             "FoodType, minMoneyLoot, maxMoneyLoot FROM item_template");

    if (!result)
    {
        sLog->outString(">> Loaded 0 item templates. DB table `item_template` is empty.");
        sLog->outString();
        return;
    }

    //_itemTemplateStore.rehash(result->GetRowCount());
    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        ItemTemplate& itemTemplate = _itemTemplateStore[entry];
    
        itemTemplate.ItemId                    = entry;
        itemTemplate.Class                     = uint32(fields[1].GetUInt8());
        itemTemplate.SubClass                  = uint32(fields[2].GetUInt8());
        itemTemplate.Unk0                      = fields[3].GetInt32();
        itemTemplate.Name1                     = fields[4].GetString();
        itemTemplate.DisplayInfoID             = fields[5].GetUInt32();
        itemTemplate.Quality                   = uint32(fields[6].GetUInt8());
        itemTemplate.Flags                     = uint32(fields[7].GetInt64());
        itemTemplate.Flags2                    = fields[8].GetUInt32();
        itemTemplate.BuyPrice                  = int32(fields[10].GetInt64());
        itemTemplate.SellPrice                 = fields[11].GetUInt32();
        itemTemplate.InventoryType             = uint32(fields[12].GetUInt8());
        itemTemplate.AllowableClass            = fields[13].GetInt32();
        itemTemplate.AllowableRace             = fields[14].GetInt32();
        itemTemplate.ItemLevel                 = uint32(fields[15].GetUInt16());
        itemTemplate.RequiredLevel             = uint32(fields[16].GetUInt8());
        itemTemplate.RequiredSkill             = uint32(fields[17].GetUInt16());
        itemTemplate.RequiredSkillRank         = uint32(fields[18].GetUInt16());
        itemTemplate.RequiredSpell             = fields[19].GetUInt32();
        itemTemplate.RequiredHonorRank         = fields[20].GetUInt32();
        itemTemplate.RequiredCityRank          = fields[21].GetUInt32();
        itemTemplate.RequiredReputationFaction = uint32(fields[22].GetUInt16());
        itemTemplate.RequiredReputationRank    = uint32(fields[23].GetUInt16());
        itemTemplate.MaxCount                  = fields[24].GetInt32();
        itemTemplate.Stackable                 = fields[25].GetInt32();
        itemTemplate.ContainerSlots            = uint32(fields[26].GetUInt8());
        itemTemplate.StatsCount                = uint32(fields[27].GetUInt8());

        for (uint8 i = 0; i < itemTemplate.StatsCount; ++i)
        {
            itemTemplate.ItemStat[i].ItemStatType  = uint32(fields[28 + i*2].GetUInt8());
            itemTemplate.ItemStat[i].ItemStatValue = int32(fields[29 + i*2].GetInt16());
        }

        itemTemplate.ScalingStatDistribution = uint32(fields[48].GetUInt16());
        itemTemplate.ScalingStatValue        = fields[49].GetInt32();

        for (uint8 i = 0; i < MAX_ITEM_PROTO_DAMAGES; ++i)
        {
            itemTemplate.Damage[i].DamageMin  = fields[50 + i*3].GetFloat();
            itemTemplate.Damage[i].DamageMax  = fields[51 + i*3].GetFloat();
            itemTemplate.Damage[i].DamageType = uint32(fields[52 + i*3].GetUInt8());
        }

        itemTemplate.Armor          = uint32(fields[56].GetUInt16());
        itemTemplate.HolyRes        = uint32(fields[57].GetUInt8());
        itemTemplate.FireRes        = uint32(fields[58].GetUInt8());
        itemTemplate.NatureRes      = uint32(fields[59].GetUInt8());
        itemTemplate.FrostRes       = uint32(fields[60].GetUInt8());
        itemTemplate.ShadowRes      = uint32(fields[61].GetUInt8());
        itemTemplate.ArcaneRes      = uint32(fields[62].GetUInt8());
        itemTemplate.Delay          = uint32(fields[63].GetUInt16());
        itemTemplate.AmmoType       = uint32(fields[64].GetUInt8());
        itemTemplate.RangedModRange = fields[65].GetFloat();

        /*for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
        {
            itemTemplate.Spells[i].SpellId               = fields[66 + i*7  ].GetInt32();
            itemTemplate.Spells[i].SpellTrigger          = uint32(fields[67 + i*7].GetUInt8());
            itemTemplate.Spells[i].SpellCharges          = int32(fields[68 + i*7].GetInt16());
            itemTemplate.Spells[i].SpellPPMRate          = fields[69 + i*7].GetFloat();
            itemTemplate.Spells[i].SpellCooldown         = fields[70 + i*7].GetInt32();
            itemTemplate.Spells[i].SpellCategory         = uint32(fields[71 + i*7].GetUInt16());
            itemTemplate.Spells[i].SpellCategoryCooldown = fields[72 + i*7].GetInt32();
        }*/

        itemTemplate.Bonding        = uint32(fields[101].GetUInt8());
        itemTemplate.Description    = fields[102].GetString();
        itemTemplate.PageText       = fields[103].GetUInt32();
        itemTemplate.LanguageID     = uint32(fields[104].GetUInt8());
        itemTemplate.PageMaterial   = uint32(fields[105].GetUInt8());
        itemTemplate.StartQuest     = fields[106].GetUInt32();
        itemTemplate.LockID         = fields[107].GetUInt32();
        itemTemplate.Material       = int32(fields[108].GetInt8());
        itemTemplate.Sheath         = uint32(fields[109].GetUInt8());
        itemTemplate.RandomProperty = fields[110].GetUInt32();
        itemTemplate.RandomSuffix   = fields[111].GetInt32();
        itemTemplate.Block          = fields[112].GetUInt32();
        itemTemplate.ItemSet        = fields[113].GetUInt32();
        itemTemplate.MaxDurability  = uint32(fields[114].GetUInt16());
        itemTemplate.Area           = fields[115].GetUInt32();
        itemTemplate.Map            = uint32(fields[116].GetUInt16());
        itemTemplate.BagFamily      = fields[117].GetUInt32();
        itemTemplate.TotemCategory  = fields[118].GetUInt32();

        for (uint8 i = 0; i < MAX_ITEM_PROTO_SOCKETS; ++i)
        {
            itemTemplate.Socket[i].Color   = uint32(fields[119 + i*2].GetUInt8());
            itemTemplate.Socket[i].Content = fields[120 + i*2].GetUInt32();
        }

        itemTemplate.socketBonus             = fields[125].GetUInt32();
        itemTemplate.GemProperties           = fields[126].GetUInt32();
        itemTemplate.RequiredDisenchantSkill = uint32(fields[127].GetInt16());
        itemTemplate.ArmorDamageModifier     = fields[128].GetFloat();
        itemTemplate.Duration                = fields[129].GetInt32();
        itemTemplate.ItemLimitCategory       = uint32(fields[130].GetInt16());
        itemTemplate.HolidayId               = fields[131].GetUInt32();
        itemTemplate.DisenchantID            = fields[133].GetUInt32();
        itemTemplate.FoodType                = uint32(fields[134].GetUInt8());
        itemTemplate.MinMoneyLoot            = fields[135].GetUInt32();
        itemTemplate.MaxMoneyLoot            = fields[136].GetUInt32();
    
        ++count;
    }
    while (result->NextRow());

    sLog->outString(">> Loaded %u item templates in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    sLog->outString();

}

ItemTemplate const* Items::GetItemTemplate(uint32 entry)
{
    ItemTemplateContainer::const_iterator itr = _itemTemplateStore.find(entry);
    if (itr != _itemTemplateStore.end())
        return &(itr->second);

    return NULL;
}

void Items::CacheItemTemplate(ItemTemplate* item)
{
    //To prevent a crash we should set an Mutex here
    TRINITY_WRITE_GUARD(ACE_RW_Thread_Mutex, rwMutex_);

    ItemTemplate& _item = _itemTemplateStore[item->ItemId];
    _item = *item;
    return;
}

// Only _static_ data send in this packet !!!
void ClientSession::HandleItemQuerySingleOpcode(WorldPacket & recv_data)
{
/*    uint32 item;
    recv_data >> item;

    sLog->outDetail("STORAGE: Item Query = %u", item);

    ItemTemplate const* pProto = sItems->GetItemTemplate(item);
    if (pProto)
    {
        std::string Name        = pProto->Name1;
        std::string Description = pProto->Description;

        int loc_idx = GetSessionDbLocaleIndex();
        if (loc_idx >= 0)
        {
            if (ItemLocale const* il = sLocalesMgr->GetItemLocale(pProto->ItemId))
            {
                LocalesMgr::GetLocaleString(il->Name, loc_idx, Name);
                LocalesMgr::GetLocaleString(il->Description, loc_idx, Description);
            }
        }
                                                            // guess size
        WorldPacket data(SMSG_ITEM_QUERY_SINGLE_RESPONSE, 600);
        data << pProto->ItemId;
        data << pProto->Class;
        data << pProto->SubClass;
        data << int32(pProto->Unk0);                        // new 2.0.3, not exist in wdb cache?
        data << Name;
        data << uint8(0x00);                                //pProto->Name2; // blizz not send name there, just uint8(0x00); <-- \0 = empty string = empty name...
        data << uint8(0x00);                                //pProto->Name3; // blizz not send name there, just uint8(0x00);
        data << uint8(0x00);                                //pProto->Name4; // blizz not send name there, just uint8(0x00);
        data << pProto->DisplayInfoID;
        data << pProto->Quality;
        data << pProto->Flags;
        data << pProto->Flags2;
        data << pProto->BuyPrice;
        data << pProto->SellPrice;
        data << pProto->InventoryType;
        data << pProto->AllowableClass;
        data << pProto->AllowableRace;
        data << pProto->ItemLevel;
        data << pProto->RequiredLevel;
        data << pProto->RequiredSkill;
        data << pProto->RequiredSkillRank;
        data << pProto->RequiredSpell;
        data << pProto->RequiredHonorRank;
        data << pProto->RequiredCityRank;
        data << pProto->RequiredReputationFaction;
        data << pProto->RequiredReputationRank;
        data << int32(pProto->MaxCount);
        data << int32(pProto->Stackable);
        data << pProto->ContainerSlots;
        data << pProto->StatsCount;                         // item stats count
        for (uint32 i = 0; i < pProto->StatsCount; ++i)
        {
            data << pProto->ItemStat[i].ItemStatType;
            data << pProto->ItemStat[i].ItemStatValue;
        }
        data << pProto->ScalingStatDistribution;            // scaling stats distribution
        data << pProto->ScalingStatValue;                   // some kind of flags used to determine stat values column
        for (int i = 0; i < MAX_ITEM_PROTO_DAMAGES; ++i)
        {
            data << pProto->Damage[i].DamageMin;
            data << pProto->Damage[i].DamageMax;
            data << pProto->Damage[i].DamageType;
        }

        // resistances (7)
        data << pProto->Armor;
        data << pProto->HolyRes;
        data << pProto->FireRes;
        data << pProto->NatureRes;
        data << pProto->FrostRes;
        data << pProto->ShadowRes;
        data << pProto->ArcaneRes;

        data << pProto->Delay;
        data << pProto->AmmoType;
        data << pProto->RangedModRange;

        for (int s = 0; s < MAX_ITEM_PROTO_SPELLS; ++s)
        {
                data << pProto->SpellData1[s];
                data << pProto->SpellData2[s];
                data << pProto->SpellData3[s];
                data << pProto->SpellData4[s];
                data << pProto->SpellData5[s];
                data << pProto->SpellData6[s];
        }
        data << pProto->Bonding;
        data << Description;
        data << pProto->PageText;
        data << pProto->LanguageID;
        data << pProto->PageMaterial;
        data << pProto->StartQuest;
        data << pProto->LockID;
        data << int32(pProto->Material);
        data << pProto->Sheath;
        data << pProto->RandomProperty;
        data << pProto->RandomSuffix;
        data << pProto->Block;
        data << pProto->ItemSet;
        data << pProto->MaxDurability;
        data << pProto->Area;
        data << pProto->Map;                                // Added in 1.12.x & 2.0.1 client branch
        data << pProto->BagFamily;
        data << pProto->TotemCategory;
        for (int s = 0; s < MAX_ITEM_PROTO_SOCKETS; ++s)
        {
            data << pProto->Socket[s].Color;
            data << pProto->Socket[s].Content;
        }
        data << pProto->socketBonus;
        data << pProto->GemProperties;
        data << pProto->RequiredDisenchantSkill;
        data << pProto->ArmorDamageModifier;
        data << uint32(abs(pProto->Duration));              // added in 2.4.2.8209, duration (seconds)
        data << pProto->ItemLimitCategory;                  // WotLK, ItemLimitCategory
        data << pProto->HolidayId;                          // Holiday.dbc?
        SendPacket(&data);
    }
    else*/
    {
        SendPacketToNode(&recv_data);
    }
}

// Only _static_ data send in this packet !!!
void ClientSession::HandleSMSG_ITEM_QUERY_SINGLE(WorldPacket & recv_data)
{
    uint32 entry;
    uint8 trash;
    ItemTemplate* item = new ItemTemplate;

    recv_data >> entry;

    if (entry >= 0x80000000)
        return;

    item->ItemId = entry;
    recv_data >> item->Class;
    recv_data >> item->SubClass;
    recv_data >> item->Unk0;                        // new 2.0.3, not exist in wdb cache?
    recv_data >> item->Name1;
    recv_data >> trash;                                //pProto->Name2; // blizz not send name there, just uint8(0x00); <-- \0 = empty string = empty name...
    recv_data >> trash;                                //pProto->Name3; // blizz not send name there, just uint8(0x00);
    recv_data >> trash;                                //pProto->Name4; // blizz not send name there, just uint8(0x00);
    recv_data >> item->DisplayInfoID;
    recv_data >> item->Quality;
    recv_data >> item->Flags;
    recv_data >> item->Flags2;
    recv_data >> item->BuyPrice;
    recv_data >> item->SellPrice;
    recv_data >> item->InventoryType;
    recv_data >> item->AllowableClass;
    recv_data >> item->AllowableRace;
    recv_data >> item->ItemLevel;
    recv_data >> item->RequiredLevel;
    recv_data >> item->RequiredSkill;
    recv_data >> item->RequiredSkillRank;
    recv_data >> item->RequiredSpell;
    recv_data >> item->RequiredHonorRank;
    recv_data >> item->RequiredCityRank;
    recv_data >> item->RequiredReputationFaction;
    recv_data >> item->RequiredReputationRank;
    recv_data >> item->MaxCount;
    recv_data >> item->Stackable;
    recv_data >> item->ContainerSlots;
    recv_data >> item->StatsCount;                         // item stats count
    
    for (uint32 i = 0; i < item->StatsCount; ++i)
    {
        recv_data >> item->ItemStat[i].ItemStatType;
        recv_data >> item->ItemStat[i].ItemStatValue;
    }
    
    recv_data >> item->ScalingStatDistribution;            // scaling stats distribution
    recv_data >> item->ScalingStatValue;                   // some kind of flags used to determine stat values column

    for (int i = 0; i < MAX_ITEM_PROTO_DAMAGES; ++i)
    {
        recv_data >> item->Damage[i].DamageMin;
        recv_data >> item->Damage[i].DamageMax;
        recv_data >> item->Damage[i].DamageType;
    }

    // resistances (7)
    recv_data >> item->Armor;
    recv_data >> item->HolyRes;
    recv_data >> item->FireRes;
    recv_data >> item->NatureRes;
    recv_data >> item->FrostRes;
    recv_data >> item->ShadowRes;
    recv_data >> item->ArcaneRes;

    recv_data >> item->Delay;
    recv_data >> item->AmmoType;
    recv_data >> item->RangedModRange;

    for (int s = 0; s < MAX_ITEM_PROTO_SPELLS; ++s)
    {
        recv_data >> item->SpellData1[s];
        recv_data >> item->SpellData2[s];
        recv_data >> item->SpellData3[s];
        recv_data >> item->SpellData4[s];
        recv_data >> item->SpellData5[s];
        recv_data >> item->SpellData6[s];
    }

    recv_data >> item->Bonding;
    recv_data >> item->Description;
    recv_data >> item->PageText;
    recv_data >> item->LanguageID;
    recv_data >> item->PageMaterial;
    recv_data >> item->StartQuest;
    recv_data >> item->LockID;
    recv_data >> item->Material;
    recv_data >> item->Sheath;
    recv_data >> item->RandomProperty;
    recv_data >> item->RandomSuffix;
    recv_data >> item->Block;
    recv_data >> item->ItemSet;
    recv_data >> item->MaxDurability;
    recv_data >> item->Area;
    recv_data >> item->Map;                                // Added in 1.12.x & 2.0.1 client branch
    recv_data >> item->BagFamily;
    recv_data >> item->TotemCategory;
    
    for (int s = 0; s < MAX_ITEM_PROTO_SOCKETS; ++s)
    {
        recv_data >> item->Socket[s].Color;
        recv_data >> item->Socket[s].Content;
    }

    recv_data >> item->socketBonus;
    recv_data >> item->GemProperties;
    recv_data >> item->RequiredDisenchantSkill;
    recv_data >> item->ArmorDamageModifier;
    recv_data >> item->Duration;              // added in 2.4.2.8209, duration (seconds)
    recv_data >> item->ItemLimitCategory;                  // WotLK, ItemLimitCategory
    recv_data >> item->HolidayId;                          // Holiday.dbc?

    sItems->CacheItemTemplate(item);
}