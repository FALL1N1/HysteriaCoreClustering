#ifndef DEF_TRANSMOGRIFICATION_H
#define DEF_TRANSMOGRIFICATION_H

#include "Config.h"

enum TransmogSlot
{
    TRANSMOG_SLOT_HEAD             = PLAYER_VISIBLE_ITEM_1_ENTRYID,
    TRANSMOG_SLOT_SHOULDERS        = PLAYER_VISIBLE_ITEM_3_ENTRYID,
    TRANSMOG_SLOT_CHEST            = PLAYER_VISIBLE_ITEM_5_ENTRYID,
    TRANSMOG_SLOT_HANDS            = PLAYER_VISIBLE_ITEM_10_ENTRYID,
    TRANSMOG_SLOT_LEGS             = PLAYER_VISIBLE_ITEM_7_ENTRYID,
    TRANSMOG_SLOT_WRISTS           = PLAYER_VISIBLE_ITEM_9_ENTRYID,
    TRANSMOG_SLOT_WAIST            = PLAYER_VISIBLE_ITEM_6_ENTRYID,
    TRANSMOG_SLOT_BOOTS            = PLAYER_VISIBLE_ITEM_8_ENTRYID,
    TRANSMOG_SLOT_MAINHAND         = PLAYER_VISIBLE_ITEM_16_ENTRYID,
    TRANSMOG_SLOT_OFFHAND          = PLAYER_VISIBLE_ITEM_17_ENTRYID,
    TRANSMOG_SLOT_RANGED           = PLAYER_VISIBLE_ITEM_18_ENTRYID,
    TRANSMOG_SLOT_BACK             = PLAYER_VISIBLE_ITEM_15_ENTRYID,
    TRANSMOG_SLOT_SHIRT            = PLAYER_VISIBLE_ITEM_4_ENTRYID,
    TRANSMOG_SLOT_TABARD           = PLAYER_VISIBLE_ITEM_19_ENTRYID,
    TRANSMOG_SLOT_MAINHAND_ENCHANT = PLAYER_VISIBLE_ITEM_16_ENCHANTMENT,
    TRANSMOG_SLOT_OFFHAND_ENCHANT  = PLAYER_VISIBLE_ITEM_17_ENCHANTMENT,
    TRANSMOG_SLOT_RANGED_ENCHANT   = PLAYER_VISIBLE_ITEM_18_ENCHANTMENT,
};

enum TransmogOption
{
    TRANSMOG_ACTION_PURCHASE_PRIEST = 2,
    TRANSMOG_ACTION_PURCHASE_MAGE,
    TRANSMOG_ACTION_PURCHASE_WARLOCK,
    TRANSMOG_ACTION_PURCHASE_DRUID,
    TRANSMOG_ACTION_PURCHASE_ROGUE,
    TRANSMOG_ACTION_PURCHASE_PALADIN,
    TRANSMOG_ACTION_PURCHASE_WARRIOR,
    TRANSMOG_ACTION_PURCHASE_DEATHKNIGHT,
    TRANSMOG_ACTION_PURCHASE_SHAMAN,
    TRANSMOG_ACTION_PURCHASE_HUNTER,
    TRANSMOG_ACTION_PURCHASE_NOCLASS,
    TRANSMOG_ACTION_PURCHASE_NOCLASS_CLOTH,
    TRANSMOG_ACTION_PURCHASE_NOCLASS_LEATHER,
    TRANSMOG_ACTION_PURCHASE_NOCLASS_PLATE,
    TRANSMOG_ACTION_PURCHASE_NOCLASS_MAIL,
    TRANSMOG_ACTION_PURSHASE_TRANSMOGS,
    TRANSMOG_ACTION_EXCHANGE_TOKENS,
    TRANSMOG_ACTION_SHOW_ENCHANTS,
    TRANSMOG_ACTION_SHOW_REMOVE_ITEM_TRANSMOG_OPTIONS,
    TRANSMOG_ACTION_SHOW_REMOVE_ENCHANT_TRANSMOG_OPTIONS,
    TRANSMOG_ACTION_SELECT_INDIVIDUAL,
    TRANSMOG_ACTION_SELECT_REMOVE_HEAD,
    TRANSMOG_ACTION_SELECT_REMOVE_SHOULDERS,
    TRANSMOG_ACTION_SELECT_REMOVE_SHIRT,
    TRANSMOG_ACTION_SELECT_REMOVE_CHEST,
    TRANSMOG_ACTION_SELECT_REMOVE_HANDS,
    TRANSMOG_ACTION_SELECT_REMOVE_LEGS,
    TRANSMOG_ACTION_SELECT_REMOVE_WRISTS,
    TRANSMOG_ACTION_SELECT_REMOVE_WAIST,
    TRANSMOG_ACTION_SELECT_REMOVE_FEET,
    TRANSMOG_ACTION_SELECT_REMOVE_MAINHAND,
    TRANSMOG_ACTION_SELECT_REMOVE_OFFHAND,
    TRANSMOG_ACTION_SELECT_REMOVE_RANGED,
    TRANSMOG_ACTION_SELECT_REMOVE_MAINHAND_ENCHANT,
    TRANSMOG_ACTION_SELECT_REMOVE_OFFHAND_ENCHANT,
    TRANSMOG_ACTION_SELECT_REMOVE_ALL_ARMOR,
    TRANSMOG_ACTION_SELECT_REMOVE_ALL_WEAPON,
    TRANSMOG_ACTION_SELECT_REMOVE_ALL_ENCHANT,
    TRANSMOG_ACTION_SELECT_REMOVE_EVERYTHING,
    TRANSMOG_ACTION_LIST_SLOT,
    TRANSMOG_ACTION_SHOW_TRANSMOGRIFIER,
    TRANSMOG_ACTION_REMOVE_TRANSMOG,
};

enum TransmogEnchantId
{
    TRANSMOG_ENCHANT_MONGOOSE       = 2673,
    TRANSMOG_ENCHANT_EXECUTIONER    = 3225,
    TRANSMOG_ENCHANT_SUNFIRE        = 2671,
    TRANSMOG_ENCHANT_SOULFROST      = 2672,
    TRANSMOG_ENCHANT_BLOOD_DRAINING = 3870,
    TRANSMOG_ENCHANT_RAZORICE       = 3370,
    TRANSMOG_ENCHANT_BERSERKING     = 3789,
    TRANSMOG_ENCHANT_SPELLSURGE     = 2674,
    TRANSMOG_ENCHANT_POTENCY        = 2668,
    TRANSMOG_ENCHANT_DEATHFROST     = 3273,
    TRANSMOG_ENCHANT_SPARKLES       = 2669,
    TRANSMOG_ENCHANT_GLOW_RED       = 853,
    TRANSMOG_ENCHANT_GLOW_YELLOW    = 2567,
    TRANSMOG_ENCHANT_GLOW_BLUE      = 1897,
    TRANSMOG_ENCHANT_GLOW_PURPLE    = 1898,
    TRANSMOG_ENCHANT_GLOW_ORANGE    = 803,
    TRANSMOG_ENCHANT_GLOW_GREEN     = 2564,
    TRANSMOG_ENCHANT_GLOW_GREY      = 1900,
    TRANSMOG_ENCHANT_GLOW_WHITE     = 1894,
    TRANSMOG_ENCHANT_NO_VISUAL      = 10000
};

enum TransmogGearSets
{
    // TBC
    TRANSMOG_SET_ABSOLUTION_REGALIA,
    TRANSMOG_SET_ALDOR_REGALIA                 = 5,
    TRANSMOG_SET_AMANI_GARB                    = 10,
    TRANSMOG_SET_AMANI_PLATE                   = 15,
    TRANSMOG_SET_AMANI_RAIMENT                 = 20,
    TRANSMOG_SET_AMANI_REGALIA                 = 25,
    TRANSMOG_SET_AVATAR_RAIMENT                = 30,
    TRANSMOG_SET_BRUTAL_GLAD_AEGIS             = 35,
    TRANSMOG_SET_BRUTAL_GLAD_BATTLEGEAR        = 40,
    TRANSMOG_SET_BRUTAL_GLAD_DREADGEAR         = 45,
    TRANSMOG_SET_BRUTAL_GLAD_EARTHSHAKER       = 50,
    TRANSMOG_SET_BRUTAL_GLAD_INVESTITURE       = 55,
    TRANSMOG_SET_BRUTAL_GLAD_PURSUIT           = 60,
    TRANSMOG_SET_BRUTAL_GLAD_REFUGE            = 65,
    TRANSMOG_SET_BRUTAL_GLAD_REGALIA           = 70,
    TRANSMOG_SET_BRUTAL_GLAD_VESTMENTS         = 75,
    TRANSMOG_SET_CATACLYSM_HARNESS             = 80,
    TRANSMOG_SET_CORRUPTOR_RAIMENT             = 85,
    TRANSMOG_SET_CRYSTALFORGE_ARMOR            = 90,
    TRANSMOG_SET_CYCLONE_HARNESS               = 95,
    TRANSMOG_SET_DEATHMANTLE_ARMOR             = 100,
    TRANSMOG_SET_DEMONSTALKER_ARMOR            = 105,
    TRANSMOG_SET_DESTROYER_ARMOR               = 110,
    TRANSMOG_SET_GLADIATOR_AEGIS               = 115,
    TRANSMOG_SET_GLADIATOR_BATTLEGEAR          = 120,
    TRANSMOG_SET_GLADIATOR_EARTHSHAKER         = 125,
    TRANSMOG_SET_GLADIATOR_FELSHROUD           = 130,
    TRANSMOG_SET_GLADIATOR_INVESTITURE         = 135,
    TRANSMOG_SET_GLADIATOR_PURSUIT             = 140,
    TRANSMOG_SET_GLADIATOR_REFUGE              = 145,
    TRANSMOG_SET_GLADIATOR_REGALIA             = 150,
    TRANSMOG_SET_GLADIATOR_VESTMENTS           = 155,
    TRANSMOG_SET_GRONNSTALKERS_ARMOR           = 160,
    TRANSMOG_SET_INCARNATE_RAIMENT             = 165,
    TRANSMOG_SET_JUSTICAR_ARMOR                = 170,
    TRANSMOG_SET_LIGHTBRINGER_ARMOR            = 175,
    TRANSMOG_SET_MALEFIC_RAIMENT               = 180,
    TRANSMOG_SET_MALORNE_RAIMENT               = 185,
    TRANSMOG_SET_MERCILESS_GLAD_AEGIS          = 190,
    TRANSMOG_SET_MERCILESS_GLAD_BATTLEGEAR     = 195,
    TRANSMOG_SET_MERCILESS_GLAD_DREADGEAR      = 200,
    TRANSMOG_SET_MERCILESS_GLAD_EARTHSHAKER    = 205,
    TRANSMOG_SET_MERCILESS_GLAD_INVESTITURE    = 210,
    TRANSMOG_SET_MERCILESS_GLAD_PURSUIT        = 215,
    TRANSMOG_SET_MERCILESS_GLAD_REFUGE         = 220,
    TRANSMOG_SET_MERCILESS_GLAD_REGALIA        = 225,
    TRANSMOG_SET_MERCILESS_GLAD_VESTMENTS      = 230,
    TRANSMOG_SET_NETHERBLADE_ARMOR             = 235,
    TRANSMOG_SET_NORDRASSIL_HARNESS            = 240,
    TRANSMOG_SET_ONSLAUGHT_ARMOR               = 245,
    TRANSMOG_SET_RIFTSTALKER_ARMOR             = 250,
    TRANSMOG_SET_SKYSHATTER_HARNESS            = 255,
    TRANSMOG_SET_RIGHTEOUS_ARMOR               = 260,
    TRANSMOG_SET_SLAYERS_ARMOR                 = 265,
    TRANSMOG_SET_SUNWELL_DRUID_RAIMENT         = 270,
    TRANSMOG_SET_SUNWELL_HUNTER_ARMOR          = 275,
    TRANSMOG_SET_SUNWELL_MAGE_REGALIA          = 280,
    TRANSMOG_SET_SUNWELL_PALADIN_ARMOR         = 285,
    TRANSMOG_SET_SUNWELL_PRIEST_VESTMENTS      = 290,
    TRANSMOG_SET_SUNWELL_ROGUE_ARMOR           = 295,
    TRANSMOG_SET_SUNWELL_SHAMAN_HARNESS        = 300,
    TRANSMOG_SET_SUNWELL_WARLOCK_RAIMENT       = 305,
    TRANSMOG_SET_SUNWELL_WARRIOR_BATTLEGEAR    = 310,

    // WotLK
    TRANSMOG_SET_ABANDONED_FEALTY_BATTLEPLATE  = 315,
    TRANSMOG_SET_AEGIS_BATTLEGEAR              = 320,
    TRANSMOG_SET_AHNKAHAR_HUNTER_BATTLEGEAR    = 325,
    TRANSMOG_SET_BLESSED_BATTLEGEAR_OF_SLAYING = 330,
    TRANSMOG_SET_BLESSED_GARB_OF_SLAYER        = 335,
    TRANSMOG_SET_BLESSED_REGALIA_OF_CLEANSING  = 340,
    TRANSMOG_SET_BLOODMAGE_REGALIA             = 345,
    TRANSMOG_SET_BONESCYTHE_BATTLEGEAR         = 350,
    TRANSMOG_SET_BRAZEN_TRESPASS_ARMOR         = 355,
    TRANSMOG_SET_CHOKING_WINTERS_GARB          = 360,
    TRANSMOG_SET_CONQUERORS_DARKRUNED_GEAR     = 365,
    TRANSMOG_SET_CRIMSON_ACOLYTES_RAIMENT      = 370,
    TRANSMOG_SET_CRYPTSTALKER_BATTLEGEAR       = 375,
    TRANSMOG_SET_DARK_COVENS_REGALIA           = 380,
    TRANSMOG_SET_DARKRUNED_BATTLEGEAR          = 385,
    TRANSMOG_SET_DEADLY_GLAD_BATTLEGEAR        = 390,
    TRANSMOG_SET_DEADLY_GLAD_DESECRATION       = 395,
    TRANSMOG_SET_DEADLY_GLAD_EARTHSHAKER       = 400,
    TRANSMOG_SET_DEADLY_GLAD_FELSHROUD         = 405,
    TRANSMOG_SET_DEADLY_GLAD_INVESTITURE       = 410,
    TRANSMOG_SET_DEADLY_GLAD_PURSUIT           = 415,
    TRANSMOG_SET_DEADLY_GLAD_REDEMPTION        = 420,
    TRANSMOG_SET_DEADLY_GLAD_REFUGE            = 425,
    TRANSMOG_SET_DEADLY_GLAD_REGALIA           = 430,
    TRANSMOG_SET_DEADLY_GLAD_VESTMENTS         = 435,
    TRANSMOG_SET_DEATHBRINGER_GARB             = 440,
    TRANSMOG_SET_DREADNAUGHT_BATTLEGEAR        = 445,
    TRANSMOG_SET_DREAMWALKER_BATTLEGEAR        = 450,
    TRANSMOG_SET_EARTHSHATTER_BATTLEGEAR       = 455,
    TRANSMOG_SET_FROSTWITCH_GARB               = 460,
    TRANSMOG_SET_FROSTFIRE_GARB                = 465,
    TRANSMOG_SET_FROSTWORN_BONE_MAIL           = 470,
    TRANSMOG_SET_FURIOUS_GLAD_BATTLEGEAR       = 475,
    TRANSMOG_SET_FURIOUS_GLAD_CONSECRATION     = 480,
    TRANSMOG_SET_FURIOUS_GLAD_EARTHSHAKER      = 485,
    TRANSMOG_SET_FURIOUS_GLAD_FELSHROUD        = 490,
    TRANSMOG_SET_FURIOUS_GLAD_INVESTITURE      = 495,
    TRANSMOG_SET_FURIOUS_GLAD_PURSUIT          = 500,
    TRANSMOG_SET_FURIOUS_GLAD_REDEMPTION       = 505,
    TRANSMOG_SET_FURIOUS_GLAD_REFUGE           = 510,
    TRANSMOG_SET_FURIOUS_GLAD_REGALIA          = 515,
    TRANSMOG_SET_FURIOUS_GLAD_VESTMENTS        = 520,
    TRANSMOG_SET_GARONAS_BATTLEGEAR            = 525,
    TRANSMOG_SET_GULDANS_REGALIA               = 530,
    TRANSMOG_SET_HATEFUL_GLAD_BATTLEGEAR       = 535,
    TRANSMOG_SET_HATEFUL_GLAD_DESECRATION      = 540,
    TRANSMOG_SET_HATEFUL_GLAD_EARTHSHAKER      = 545,
    TRANSMOG_SET_HATEFUL_GLAD_FELSHROUD        = 550,
    TRANSMOG_SET_HATEFUL_GLAD_INVESTITURE      = 555,
    TRANSMOG_SET_HATEFUL_GLAD_PURSUIT          = 560,
    TRANSMOG_SET_HATEFUL_GLAD_REDEMPTION       = 565,
    TRANSMOG_SET_HATEFUL_GLAD_REFUGE           = 570,
    TRANSMOG_SET_HATEFUL_GLAD_REGALIA          = 575,
    TRANSMOG_SET_HATEFUL_GLAD_VESTMENTS        = 580,
    TRANSMOG_SET_HELLSCREAMS_BATTLEGEAR        = 585,
    TRANSMOG_SET_HEROES_SCOURGEBANE_BATTLEGEAR = 590,
    TRANSMOG_SET_KELTHUZAD_REGALIA             = 595,
    TRANSMOG_SET_KIRINTOR_GARB                 = 600,
    TRANSMOG_SET_KHADGARS_REGALIA              = 605,
    TRANSMOG_SET_KOLTIRAS_BATTLEGEAR           = 610,
    TRANSMOG_SET_LASHERWEAVE_BATTLEGEAR        = 615,
    TRANSMOG_SET_LIADRINS_BATTLEGEAR           = 620,
    TRANSMOG_SET_LIGHTSWORN_GARB               = 625,
    TRANSMOG_SET_MALFURIONS_BATTLEGEAR         = 630,
    TRANSMOG_SET_NIGHTSONG_BATTLEGEAR          = 635,
    TRANSMOG_SET_NOBUNDOS_BATTLEGEAR           = 640,
    TRANSMOG_SET_NORTHREND_FROZEN_MAIL         = 645,
    TRANSMOG_SET_NORTHREND_LEATHER_GARB        = 650,
    TRANSMOG_SET_NORTHREND_SPELLWEAVE_REGALIA  = 655,
    TRANSMOG_SET_PALEBONE_REGALIA              = 660,
    TRANSMOG_SET_PLAGUEHEART_GARB              = 665,
    TRANSMOG_SET_RAVENOUS_FIEND_SET            = 670,
    TRANSMOG_SET_REDEMPTION_BATTLEGEAR         = 675,
    TRANSMOG_SET_REGALIA_OF_FAITH              = 680,
    TRANSMOG_SET_RELENTLESS_GLAD_BATTLEGEAR    = 685,
    TRANSMOG_SET_RELENTLESS_GLAD_DESECRATION   = 690,
    TRANSMOG_SET_RELENTLESS_GLAD_EARTHSHAKER   = 695,
    TRANSMOG_SET_RELENTLESS_GLAD_FELSHROUD     = 700,
    TRANSMOG_SET_RELENTLESS_GLAD_INVESTITURE   = 705,
    TRANSMOG_SET_RELENTLESS_GLAD_PURSUIT       = 710,
    TRANSMOG_SET_RELENTLESS_GLAD_REDEMPTION    = 715,
    TRANSMOG_SET_RELENTLESS_GLAD_REFUGE        = 720,
    TRANSMOG_SET_RELENTLESS_GLAD_REGALIA       = 725,
    TRANSMOG_SET_RELENTLESS_GLAD_VESTMENTS     = 730,
    TRANSMOG_SET_RUNETOTEMS_BATTLEGEAR         = 735,
    TRANSMOG_SET_SANCTIFICATION_REGALIA        = 740,
    TRANSMOG_SET_SCOURGELORDS_BATTLEGEAR       = 745,
    TRANSMOG_SET_SCOURGELORDS_BATTLEPLATE      = 750,
    TRANSMOG_SET_SCOURGESTALKER_BATTLEGEAR     = 755,
    TRANSMOG_SET_SHADOWBLADES_BATTLEGEAR       = 760,
    TRANSMOG_SET_SIEGEBREAKER_BATTLEGEAR       = 765,
    TRANSMOG_SET_SNOWY_BRAMBLE_ARMOR           = 770,
    TRANSMOG_SET_SUNSTRIDERS_REGALIA           = 775,
    TRANSMOG_SET_TERRORBLADE_BATTLEGEAR        = 780,
    TRANSMOG_SET_THESSARIANS_BATTLEGEAR        = 785,
    TRANSMOG_SET_THRALLS_BATTLEGEAR            = 790,
    TRANSMOG_SET_TURALYONS_BATTLEGEAR          = 795,
    TRANSMOG_SET_UNDEAD_SLAYERS_BLESSED_ARMOR  = 800,
    TRANSMOG_SET_VALOROUS_DARKRUNED_BATTLEGEAR = 805,
    TRANSMOG_SET_VALOROUS_SCOURGE_BATTLEPLATE  = 810,
    TRANSMOG_SET_VANCLEEFS_BATTLEGEAR          = 815,
    TRANSMOG_SET_VELENS_RAIMENT                = 820,
    TRANSMOG_SET_WINDRUNNERS_BATTLEGEAR        = 825,
    TRANSMOG_SET_WINDRUNNERS_PURSUIT           = 830,
    TRANSMOG_SET_WORLDBREAKER_BATTLEGEAR       = 835,
    TRANSMOG_SET_WRATHFUL_GLAD_BATTLEGEAR      = 840,
    TRANSMOG_SET_WRATHFUL_GLAD_DESECRATION     = 845,
    TRANSMOG_SET_WRATHFUL_GLAD_EARTHSHAKER     = 850,
    TRANSMOG_SET_WRATHFUL_GLAD_FELSHROUD       = 855,
    TRANSMOG_SET_WRATHFUL_GLAD_INVESTITURE     = 860,
    TRANSMOG_SET_WRATHFUL_GLAD_PURSUIT         = 865,
    TRANSMOG_SET_WRATHFUL_GLAD_REDEMPTION      = 870,
    TRANSMOG_SET_WRATHFUL_GLAD_REFUGE          = 875,
    TRANSMOG_SET_WRATHFUL_GLAD_REGALIA         = 880,
    TRANSMOG_SET_WRATHFUL_GLAD_VESTMENTS       = 885,
    TRANSMOG_SET_WRYNNS_BATTLEGEAR             = 890,
    TRANSMOG_SET_YMIRJARLORDS_PLATE            = 895,
    TRANSMOG_SET_ZABRAS_RAIMENT                = 900,

    //Vanilla
    TRANSMOG_SET_ARCANIST_REGALIA              = 905,
    TRANSMOG_SET_AVENGERS_BATTLEGEAR           = 910,
    TRANSMOG_SET_BATTLEGEAR_OF_HEROISM         = 915,
    TRANSMOG_SET_BATTLEGEAR_OF_MIGHT           = 920,
    TRANSMOG_SET_BATTLEGEAR_OF_WRATH           = 925,
    TRANSMOG_SET_BEASTMASTER_ARMOR             = 930,
    TRANSMOG_SET_BLOODFANG_ARMOR               = 935,
    TRANSMOG_SET_BONESCYTHE_ARMOR              = 940,
    TRANSMOG_SET_CENARION_RAIMENT              = 945,
    TRANSMOG_SET_CONQUERORS_BATTLEGEAR         = 950,
    TRANSMOG_SET_CRYPTSTALKER_ARMOR            = 955,
    TRANSMOG_SET_DARKIRON_PLATE                = 960,
    TRANSMOG_SET_DARKMANTLE_ARMOR              = 965,
    TRANSMOG_SET_DEATHDEALERS_EMBRACE          = 970,
    TRANSMOG_SET_DOOMCALLERS_ATTIRE            = 975,
    TRANSMOG_SET_DRAGONSTALKER_ARMOR           = 980,
    TRANSMOG_SET_DREADNAUGHTS_BATTLEGEAR       = 985,
    TRANSMOG_SET_DREAMWALKER_RAIMENT           = 990,
    TRANSMOG_SET_ENIGMA_VESTMENTS              = 995,
    TRANSMOG_SET_FELHEART_RAIMENT              = 1000,
    TRANSMOG_SET_FERALHEART_RAIMENT            = 1005,
    TRANSMOG_SET_FIELD_MARSHALS_BATTLEGEAR     = 1010,
    TRANSMOG_SET_FIELD_MARSHALS_PURSUIT        = 1015,
    TRANSMOG_SET_FIELD_MARSHALS_RAIMENT        = 1020,
    TRANSMOG_SET_FIELD_MARSHALS_REGALIA        = 1025,
    TRANSMOG_SET_FIELD_MARSHALS_SANCTUARY      = 1030,
    TRANSMOG_SET_FIELD_MARSHALS_THREADS        = 1035,
    TRANSMOG_SET_FIELD_MARSHALS_VESTMENTS      = 1040,
    TRANSMOG_SET_FROSTFIRE_REGALIA             = 1045,
    TRANSMOG_SET_GARMENTS_OF_THE_ORACLE        = 1050,
    TRANSMOG_SET_GENESIS_RAIMENT               = 1055,
    TRANSMOG_SET_GIANTSTALKER_ARMOR            = 1060,
    TRANSMOG_SET_INFERNO_FORGED_MAIL           = 1065,
    TRANSMOG_SET_INFERNO_HARDENED_GARB         = 1070,
    TRANSMOG_SET_INFERNOWEAVE_REGALIA          = 1075,
    TRANSMOG_SET_JUDGEMENT_ARMOR               = 1080,
    TRANSMOG_SET_NAXXRAMAS_ICEBANE_PLATE       = 1085,
    TRANSMOG_SET_NAXXRAMAS_ICE_SCALE_MAIL      = 1090,
    TRANSMOG_SET_NAXXRAMAS_POLAR_ARMOR         = 1095,
    TRANSMOG_SET_NEMESIS_RAIMENT               = 1100,
    TRANSMOG_SET_NETHERWIND_REGALIA            = 1105,
    TRANSMOG_SET_NIGHTSLAYER_ARMOR             = 1110,
    TRANSMOG_SET_PLAGUEHEART_RAIMENT           = 1115,
    TRANSMOG_SET_REDEMPTION_ARMOR              = 1120,
    TRANSMOG_SET_SORCERERS_REGALIA             = 1125,
    TRANSMOG_SET_SOULFORGE_ARMOR               = 1130,
    TRANSMOG_SET_STORMCALLERS_GARB             = 1135,
    TRANSMOG_SET_STORMRAGE_RAIMENT             = 1140,
    TRANSMOG_SET_STRIKERS_GARB                 = 1145,
    TRANSMOG_SET_THE_EARTHFURY                 = 1150,
    TRANSMOG_SET_THE_EARTHSHATTERER            = 1155,
    TRANSMOG_SET_THE_FIVE_THUNDERS             = 1160,
    TRANSMOG_SET_THE_TEN_STORMS                = 1165,
    TRANSMOG_SET_VESTMENTS_OF_FAITH            = 1170,
    TRANSMOG_SET_VESTMENTS_OF_PROPHECY         = 1175,
    TRANSMOG_SET_VESTMENTS_OF_THE_VIRTUOUS     = 1180,
    TRANSMOG_SET_VESTMENTS_OF_TRANSCENDENCE    = 1185,
    TRANSMOG_SET_WARLORDS_BATTLEGEAR           = 1190,
    TRANSMOG_SET_WARLORDS_PURSUIT              = 1195,
    TRANSMOG_SET_WARLORDS_RAIMENT              = 1200,
    TRANSMOG_SET_WARLORDS_REGALIA              = 1205,
    TRANSMOG_SET_WARLORDS_THREADS              = 1210,
    TRANSMOG_SET_WARLORDS_VESTMENTS            = 1215,
};

class Transmogrification
{
public:
    Transmogrification() { };
    ~Transmogrification() { };

    uint32 GetRequireGold() { return RequireGold; }
    float GetGoldModifier() { return GoldModifier; }
    uint32 GetGoldCost() { return GoldCost; }

    bool GetRequireToken() { return RequireToken; }
    uint32 GetTokenEntry(uint32 inventoryType) 
    {  
        uint32 entry = entries[inventoryType];
        return entry ? entry : TokenEntry; 
    }
    uint32 GetTokenAmount(uint32 inventoryType) 
    { 
        uint32 amount = amounts[inventoryType];
        return amount ? amount : TokenAmount; 
    }
    uint32 GetQualityMultiplier(uint32 quality)
    {
        uint32 m = quality_multiplier[quality];
        return m ? m : 1;
    }

    std::string GetItemIcon(uint32 entry, uint32 width, uint32 height, int x, int y)
    {
        switch (entry)
        {
            case 230017:
                return GetSlotIcon(EQUIPMENT_SLOT_BODY, width, height, x, y);
            case 230018:
                return GetSlotIcon(EQUIPMENT_SLOT_TABARD, width, height, x, y);
            default:
                break;
        }

        std::ostringstream ss;
        ss << "|TInterface";
        const ItemTemplate* temp = sObjectMgr->GetItemTemplate(entry);
        const ItemDisplayInfoEntry* dispInfo = NULL;
        if (temp)
        {
            dispInfo = sItemDisplayInfoStore.LookupEntry(temp->DisplayInfoID);
            if (dispInfo)
                ss << "/ICONS/" << dispInfo->inventoryIcon;
        }
        if (!temp && !dispInfo)
            ss << "/InventoryItems/WoWUnknownItem01";
        ss << ":" << width << ":" << height << ":" << x << ":" << y << "|t";
        return ss.str();
    }

    std::string GetSlotIcon(uint8 slot, uint32 width, uint32 height, int x, int y)
    {
        std::ostringstream ss;
        ss << "|TInterface/PaperDoll/";
        switch (slot)
        {
            case EQUIPMENT_SLOT_HEAD      : ss << "UI-PaperDoll-Slot-Head"; break;
            case EQUIPMENT_SLOT_SHOULDERS : ss << "UI-PaperDoll-Slot-Shoulder"; break;
            case EQUIPMENT_SLOT_BODY      : ss << "UI-PaperDoll-Slot-Shirt"; break;
            case EQUIPMENT_SLOT_CHEST     : ss << "UI-PaperDoll-Slot-Chest"; break;
            case EQUIPMENT_SLOT_WAIST     : ss << "UI-PaperDoll-Slot-Waist"; break;
            case EQUIPMENT_SLOT_LEGS      : ss << "UI-PaperDoll-Slot-Legs"; break;
            case EQUIPMENT_SLOT_FEET      : ss << "UI-PaperDoll-Slot-Feet"; break;
            case EQUIPMENT_SLOT_WRISTS    : ss << "UI-PaperDoll-Slot-Wrists"; break;
            case EQUIPMENT_SLOT_HANDS     : ss << "UI-PaperDoll-Slot-Hands"; break;
            case EQUIPMENT_SLOT_BACK      : ss << "UI-PaperDoll-Slot-Chest"; break;
            case EQUIPMENT_SLOT_MAINHAND  : ss << "UI-PaperDoll-Slot-MainHand"; break;
            case EQUIPMENT_SLOT_OFFHAND   : ss << "UI-PaperDoll-Slot-SecondaryHand"; break;
            case EQUIPMENT_SLOT_RANGED    : ss << "UI-PaperDoll-Slot-Ranged"; break;
            case EQUIPMENT_SLOT_TABARD    : ss << "UI-PaperDoll-Slot-Tabard"; break;
            default: ss << "UI-Backpack-EmptySlot";
        }
        ss << ":" << width << ":" << height << ":" << x << ":" << y << "|t";
        return ss.str();
    }

    std::string GetItemLink(Item* item, WorldSession* session)
    {
        LocaleConstant loc_idx = session->GetSessionDbLocaleIndex();
        const ItemTemplate* temp = item->GetTemplate();
        std::string name = temp->Name1;
        if (ItemLocale const* il = sObjectMgr->GetItemLocale(temp->ItemId))
            ObjectMgr::GetLocaleString(il->Name, loc_idx, name);

        if (int32 itemRandPropId = item->GetItemRandomPropertyId())
        {
            char* const* suffix = NULL;
            if (itemRandPropId < 0)
            {
                const ItemRandomSuffixEntry* itemRandEntry = sItemRandomSuffixStore.LookupEntry(-item->GetItemRandomPropertyId());
                if (itemRandEntry)
                    suffix = itemRandEntry->nameSuffix;
            }
            else
            {
                const ItemRandomPropertiesEntry* itemRandEntry = sItemRandomPropertiesStore.LookupEntry(item->GetItemRandomPropertyId());
                if (itemRandEntry)
                    suffix = itemRandEntry->nameSuffix;
            }
            if (suffix)
            {
                std::string test(suffix[(name != temp->Name1) ? loc_idx : DEFAULT_LOCALE]);
                if (!test.empty())
                {
                    name += ' ';
                    name += test;
                }
            }
        }

        std::ostringstream oss;
        oss << "|c" << std::hex << ItemQualityColors[temp->Quality] << std::dec <<
            "|Hitem:" << temp->ItemId <<":" <<
            item->GetEnchantmentId(PERM_ENCHANTMENT_SLOT) << ":" <<
            item->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT) << ":" <<
            item->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2) << ":" <<
            item->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3) << ":" <<
            item->GetEnchantmentId(BONUS_ENCHANTMENT_SLOT) << ":" <<
            item->GetItemRandomPropertyId() << ":" << item->GetItemSuffixFactor() << ":" <<
            (uint32)item->GetOwner()->getLevel() << "|h[" << name << "]|h|r";

        return oss.str();
   }

    uint8 GetItemSlotByAction(uint16 action)
    {
        switch (action)
        {
            case TRANSMOG_ACTION_SELECT_REMOVE_HEAD:
                return EQUIPMENT_SLOT_HEAD;
            case TRANSMOG_ACTION_SELECT_REMOVE_SHOULDERS:
                return EQUIPMENT_SLOT_SHOULDERS;
            case TRANSMOG_ACTION_SELECT_REMOVE_SHIRT:
                return EQUIPMENT_SLOT_BODY;
            case TRANSMOG_ACTION_SELECT_REMOVE_CHEST:
                return EQUIPMENT_SLOT_CHEST;
            case TRANSMOG_ACTION_SELECT_REMOVE_HANDS:
                return EQUIPMENT_SLOT_HANDS;
            case TRANSMOG_ACTION_SELECT_REMOVE_LEGS:
                return EQUIPMENT_SLOT_LEGS;
            case TRANSMOG_ACTION_SELECT_REMOVE_WRISTS:
                return EQUIPMENT_SLOT_WRISTS;
            case TRANSMOG_ACTION_SELECT_REMOVE_WAIST:
                return EQUIPMENT_SLOT_WAIST;
            case TRANSMOG_ACTION_SELECT_REMOVE_FEET:
                return EQUIPMENT_SLOT_FEET;
            case TRANSMOG_ACTION_SELECT_REMOVE_MAINHAND:
            case TRANSMOG_ACTION_SELECT_REMOVE_MAINHAND_ENCHANT:
                return EQUIPMENT_SLOT_MAINHAND;
            case TRANSMOG_ACTION_SELECT_REMOVE_OFFHAND:
            case TRANSMOG_ACTION_SELECT_REMOVE_OFFHAND_ENCHANT:
                return EQUIPMENT_SLOT_OFFHAND;
            case TRANSMOG_ACTION_SELECT_REMOVE_RANGED:
                return EQUIPMENT_SLOT_RANGED;
            default:
                return 0;
        }
    }

    bool AllowedQuality(uint32 quality, uint32 InventoryType) // Only thing used elsewhere (Player.cpp)
    {
        // @todo: implement this better
        if (InventoryType == INVTYPE_TABARD || InventoryType == INVTYPE_BODY)
            return AllowRare;

        switch(quality)
        {
            case ITEM_QUALITY_POOR: return AllowPoor;
            case ITEM_QUALITY_NORMAL: return AllowCommon;
            case ITEM_QUALITY_UNCOMMON: return AllowUncommon;
            case ITEM_QUALITY_RARE: return AllowRare;
            case ITEM_QUALITY_EPIC: return AllowEpic;
            case ITEM_QUALITY_LEGENDARY: return AllowLegendary;
            case ITEM_QUALITY_ARTIFACT: return AllowArtifact;
            case ITEM_QUALITY_HEIRLOOM: return AllowHeirloom;
            default: return false;
        }
    }

    void LoadConfig()
    {
        RequireGold = (uint32)sConfigMgr->GetIntDefault("Transmogrification.RequireGold", 1);
        GoldModifier = sConfigMgr->GetFloatDefault("Transmogrification.GoldModifier", 1.0f);
        GoldCost = (uint32)sConfigMgr->GetIntDefault("Transmogrification.GoldCost", 100000);

        RequireToken = sConfigMgr->GetBoolDefault("Transmogrification.RequireToken", true);
        TokenEntry = (uint32)sConfigMgr->GetIntDefault("Transmogrification.TokenEntry", 49426);
        TokenAmount = (uint32)sConfigMgr->GetIntDefault("Transmogrification.TokenAmount", 1);

        AllowPoor = sConfigMgr->GetBoolDefault("Transmogrification.AllowPoor", false);
        AllowCommon = sConfigMgr->GetBoolDefault("Transmogrification.AllowCommon", false);
        AllowUncommon = sConfigMgr->GetBoolDefault("Transmogrification.AllowUncommon", true);
        AllowRare = sConfigMgr->GetBoolDefault("Transmogrification.AllowRare", true);
        AllowEpic = sConfigMgr->GetBoolDefault("Transmogrification.AllowEpic", true);
        AllowLegendary = sConfigMgr->GetBoolDefault("Transmogrification.AllowLegendary", false);
        AllowArtifact = sConfigMgr->GetBoolDefault("Transmogrification.AllowArtifact", false);
        AllowHeirloom = sConfigMgr->GetBoolDefault("Transmogrification.AllowHeirloom", true);

        for(uint32 i = 0; i < 30; ++i)
        {
            entries[i] = TokenEntry;
            amounts[i] = TokenAmount;
        }

        entries[INVTYPE_HEAD] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.HeadTokenEntry", 49426);
        amounts[INVTYPE_HEAD] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.HeadTokenAmount", 1);

        entries[INVTYPE_SHOULDERS] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.ShouldersTokenEntry", 49426);
        amounts[INVTYPE_SHOULDERS] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.ShouldersTokenAmount", 1);

        entries[INVTYPE_BODY] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.ShirtTokenEntry", 49426);
        amounts[INVTYPE_BODY] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.ShirtTokenAmount", 1);

        entries[INVTYPE_CHEST] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.ChestTokenEntry", 49426);
        amounts[INVTYPE_CHEST] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.ChestTokenAmount", 1);

        entries[INVTYPE_ROBE] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.RobeTokenEntry", 49426);
        amounts[INVTYPE_ROBE] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.RobeTokenAmount", 1);

        entries[INVTYPE_WAIST] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.WaistTokenEntry", 49426);
        amounts[INVTYPE_WAIST] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.WaistTokenAmount", 1);

        entries[INVTYPE_LEGS] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.LegsTokenEntry", 49426);
        amounts[INVTYPE_LEGS] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.LegsTokenAmount", 1);

        entries[INVTYPE_FEET] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.FeetTokenEntry", 49426);
        amounts[INVTYPE_FEET] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.FeetTokenAmount", 1);

        entries[INVTYPE_WRISTS] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.WristsTokenEntry", 49426);
        amounts[INVTYPE_WRISTS] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.WristsTokenAmount", 1);

        entries[INVTYPE_HANDS] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.HandsTokenEntry", 49426);
        amounts[INVTYPE_HANDS] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.HandsTokenAmount", 1);

        entries[INVTYPE_CLOAK] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.BackTokenEntry", 49426);
        amounts[INVTYPE_CLOAK] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.BackTokenAmount", 1);

        entries[INVTYPE_2HWEAPON] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.TwoHandedTokenEntry", 49426);
        amounts[INVTYPE_2HWEAPON] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.TwoHandedTokenAmount", 2);

        entries[INVTYPE_WEAPON] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.WeaponTokenEntry", 49426);
        amounts[INVTYPE_WEAPON] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.WeaponTokenAmount", 2);

        entries[INVTYPE_SHIELD] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.ShieldTokenEntry", 49426);
        amounts[INVTYPE_SHIELD] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.ShieldTokenAmount", 2);

        entries[INVTYPE_CLOAK] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.CloakTokenEntry", 49426);
        amounts[INVTYPE_CLOAK] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.CloakTokenAmount", 2);

        entries[INVTYPE_WEAPONMAINHAND] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.MainHandTokenEntry", 49426);
        amounts[INVTYPE_WEAPONMAINHAND] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.MainHandTokenAmount", 2);

        entries[INVTYPE_HOLDABLE] = entries[INVTYPE_WEAPONOFFHAND] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.OffHandTokenEntry", 49426);
        amounts[INVTYPE_HOLDABLE] = amounts[INVTYPE_WEAPONOFFHAND] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.OffHandTokenAmount", 2);

        entries[INVTYPE_THROWN] = entries[INVTYPE_RANGED] = entries[INVTYPE_RANGEDRIGHT] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.RangedTokenEntry", 49426);
        amounts[INVTYPE_THROWN] = amounts[INVTYPE_RANGED] = amounts[INVTYPE_RANGEDRIGHT] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.RangedTokenAmount", 1);

        entries[INVTYPE_TABARD] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.TabardTokenEntry", 49426);
        amounts[INVTYPE_TABARD] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.TabardTokenAmount", 1);

        quality_multiplier[ITEM_QUALITY_POOR] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.QualityPoorMultiplier", 1);
        quality_multiplier[ITEM_QUALITY_NORMAL] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.QualityNormalMultiplier", 1);
        quality_multiplier[ITEM_QUALITY_UNCOMMON] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.QualityUncommonMultiplier", 1);
        quality_multiplier[ITEM_QUALITY_RARE] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.QualityRareMultiplier", 1);
        quality_multiplier[ITEM_QUALITY_EPIC] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.QualityEpicMultiplier", 2);
        quality_multiplier[ITEM_QUALITY_LEGENDARY] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.QualityLegendaryMultiplier", 1);
        quality_multiplier[ITEM_QUALITY_ARTIFACT] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.QualityArtifactMultiplier", 1);
        quality_multiplier[ITEM_QUALITY_HEIRLOOM] = (uint32)sConfigMgr->GetIntDefault("Transmogrification.QualityHeirloomMultiplier", 1);  
    }

private:

    uint32 RequireGold;
    float GoldModifier;
    uint32 GoldCost;

    bool RequireToken;
    uint32 TokenEntry;
    uint32 TokenAmount;

    uint32 entries[30];
    uint32 amounts[30];
    uint32 quality_multiplier[8];

    bool AllowPoor;
    bool AllowCommon;
    bool AllowUncommon;
    bool AllowRare;
    bool AllowEpic;
    bool AllowLegendary;
    bool AllowArtifact;
    bool AllowHeirloom;
};
#define sTransmogrification ACE_Singleton<Transmogrification, ACE_Null_Mutex>::instance()
#endif
