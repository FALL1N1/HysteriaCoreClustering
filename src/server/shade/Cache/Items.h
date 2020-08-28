#ifndef _ITEMPROTOTYPE_H
#define _ITEMPROTOTYPE_H

#include "Common.h"
#include "SharedDefines.h"

// -1 from client enchantment slot number
enum EnchantmentSlot
{
    PERM_ENCHANTMENT_SLOT           = 0,
    TEMP_ENCHANTMENT_SLOT           = 1,
    SOCK_ENCHANTMENT_SLOT           = 2,
    SOCK_ENCHANTMENT_SLOT_2         = 3,
    SOCK_ENCHANTMENT_SLOT_3         = 4,
    BONUS_ENCHANTMENT_SLOT          = 5,
    PRISMATIC_ENCHANTMENT_SLOT      = 6,                    // added at apply special permanent enchantment
    MAX_INSPECTED_ENCHANTMENT_SLOT  = 7,

    PROP_ENCHANTMENT_SLOT_0         = 7,                    // used with RandomSuffix
    PROP_ENCHANTMENT_SLOT_1         = 8,                    // used with RandomSuffix
    PROP_ENCHANTMENT_SLOT_2         = 9,                    // used with RandomSuffix and RandomProperty
    PROP_ENCHANTMENT_SLOT_3         = 10,                   // used with RandomProperty
    PROP_ENCHANTMENT_SLOT_4         = 11,                   // used with RandomProperty
    MAX_ENCHANTMENT_SLOT            = 12
};

#define MAX_ITEM_PROTO_DAMAGES 2                            // changed in 3.1.0
#define MAX_ITEM_PROTO_SOCKETS 3
#define MAX_ITEM_PROTO_SPELLS  5
#define MAX_ITEM_PROTO_STATS  10

struct _Damage
{
    float   DamageMin;
    float   DamageMax;
    uint32  DamageType;                                     // id from Resistances.dbc
};

struct _ItemStat
{
    uint32  ItemStatType;
    int32   ItemStatValue;
};

struct _Socket
{
    uint32 Color;
    uint32 Content;
};

struct ItemTemplate
{
    uint32      ItemId;
    std::string  Name1;
    uint32      Class;                                           // id from ItemClass.dbc
    uint32      SubClass;                                        // id from ItemSubClass.dbc
    int32       Unk0;
    uint32      DisplayInfoID;                                   // id from ItemDisplayInfo.dbc
    uint32      Quality;
    uint32      Flags;
    uint32      Flags2;
    int32       BuyPrice;
    uint32      SellPrice;
    uint32      InventoryType;
    uint32      AllowableClass;
    uint32      AllowableRace;
    uint32      ItemLevel;
    uint32      RequiredLevel;
    uint32      RequiredSkill;                                   // id from SkillLine.dbc
    uint32      RequiredSkillRank;
    uint32      RequiredSpell;                                   // id from Spell.dbc
    uint32      RequiredHonorRank;
    uint32      RequiredCityRank;
    uint32      RequiredReputationFaction;                       // id from Faction.dbc
    uint32      RequiredReputationRank;
    int32       MaxCount;                                        // <= 0: no limit
    int32       Stackable;                                       // 0: not allowed, -1: put in player coin info tab and don't limit stacking (so 1 slot)
    uint32      ContainerSlots;
    uint32      StatsCount;
    _ItemStat   ItemStat[MAX_ITEM_PROTO_STATS];
    uint32      ScalingStatDistribution;                         // id from ScalingStatDistribution.dbc
    uint32      ScalingStatValue;                                // mask for selecting column in ScalingStatValues.dbc
    _Damage     Damage[MAX_ITEM_PROTO_DAMAGES];
    uint32      Armor;
    uint32      HolyRes;
    uint32      FireRes;
    uint32      NatureRes;
    uint32      FrostRes;
    uint32      ShadowRes;
    uint32      ArcaneRes;
    uint32      Delay;
    uint32      AmmoType;
    float       RangedModRange;
    uint32      SpellData1[MAX_ITEM_PROTO_SPELLS];
    uint32      SpellData2[MAX_ITEM_PROTO_SPELLS];
    uint32      SpellData3[MAX_ITEM_PROTO_SPELLS];
    uint32      SpellData4[MAX_ITEM_PROTO_SPELLS];
    uint32      SpellData5[MAX_ITEM_PROTO_SPELLS];
    uint32      SpellData6[MAX_ITEM_PROTO_SPELLS];
    uint32      Bonding;
    std::string  Description;
    uint32      PageText;
    uint32      LanguageID;
    uint32      PageMaterial;
    uint32      StartQuest;                                      // id from QuestCache.wdb
    uint32      LockID;
    int32       Material;                                        // id from Material.dbc
    uint32      Sheath;
    int32       RandomProperty;                                  // id from ItemRandomProperties.dbc
    int32       RandomSuffix;                                    // id from ItemRandomSuffix.dbc
    uint32      Block;
    uint32      ItemSet;                                         // id from ItemSet.db
    uint32      MaxDurability;
    uint32      Area;                                            // id from AreaTable.dbc
    uint32      Map;                                             // id from Map.dbc
    uint32      BagFamily;                                       // bit mask (1 << id from ItemBagFamily.dbc)
    uint32      TotemCategory;                                   // id from TotemCategory.dbc
    _Socket     Socket[MAX_ITEM_PROTO_SOCKETS];
    uint32      socketBonus;                                     // id from SpellItemEnchantment.dbc
    uint32      GemProperties;                                   // id from GemProperties.dbc
    uint32      RequiredDisenchantSkill;
    float       ArmorDamageModifier;
    int32       Duration;                                        // negative = realtime, positive = ingame time
    uint32      ItemLimitCategory;                               // id from ItemLimitCategory.dbc
    uint32      HolidayId;                                       // id from Holidays.dbc
    uint32      ScriptId;
    uint32      DisenchantID;
    uint32      FoodType;
    uint32      MinMoneyLoot;
    uint32      MaxMoneyLoot;
};





// Benchmarked: Faster than std::map (insert/find)
typedef std::unordered_map<uint32, ItemTemplate> ItemTemplateContainer;

class Items
{
    public:
        void WarmingCache();
        ItemTemplate const* GetItemTemplate(uint32 entry);
        void CacheItemTemplate(ItemTemplate* item);

    private:
        ACE_RW_Thread_Mutex rwMutex_;
        ItemTemplateContainer _itemTemplateStore;

};
#define sItems ACE_Singleton<Items, ACE_Null_Mutex>::instance()
#endif