#include "ScriptPCH.h"
#include "Transmogrification.h"
#include "Language.h"

class npc_transmogrify : public CreatureScript
{
public:
    npc_transmogrify() : CreatureScript("npc_transmogrify") {}

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (!sWorld->getBoolConfig(CUSTOM_SCRIPTS_ENABLED_OR_NOT))
            return false;

        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\Inv_helmet_94:35:35:-18:0|tTransmogrify Items", GOSSIP_SENDER_MAIN, TRANSMOG_ACTION_SHOW_TRANSMOGRIFIER);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\Inv_weapon_shortblade_54:35:35:-18:0|tTransmogrify Enchants", GOSSIP_SENDER_MAIN, TRANSMOG_ACTION_SHOW_ENCHANTS);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\ability_DualWieldSpecialization:35:35:-18:0|tRemove Enchant Transmogrifications", GOSSIP_SENDER_MAIN, TRANSMOG_ACTION_SHOW_REMOVE_ENCHANT_TRANSMOG_OPTIONS);
        player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\spell_Shadow_SacrificialShield:35:35:-18:0|tRemove All Transmogrifications", GOSSIP_SENDER_MAIN, TRANSMOG_ACTION_SELECT_REMOVE_EVERYTHING, "Are you sure you wish to remove all transmogrifications?", 0, false);
        player->SEND_GOSSIP_MENU(1, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();
        WorldSession* session = player->GetSession();
        switch (sender)
        {
            case GOSSIP_SENDER_MAIN:
            {
                switch (action)
                {
                    case TRANSMOG_ACTION_SHOW_TRANSMOGRIFIER:                  ShowTransmogOptions(player, creature);                  break;
                    case TRANSMOG_ACTION_PURSHASE_TRANSMOGS:                   ShowMaterialTransmogOptions(player, creature);          break;
                    case TRANSMOG_ACTION_SHOW_ENCHANTS:                        ShowTransmogEnchants(player, creature);                 break;
                    case TRANSMOG_ACTION_SHOW_REMOVE_ENCHANT_TRANSMOG_OPTIONS: ShowRemoveTransmogEnchantOptions(player, creature);     break;
                    case TRANSMOG_ACTION_SELECT_REMOVE_EVERYTHING:             RemoveAllTransmog(player);                              break;
                    case GOSSIP_SENDER_MAIN:                                   OnGossipHello(player, creature);                        break;
                }
                break;
            }
            case TRANSMOG_ACTION_PURCHASE_NOCLASS_CLOTH:
            case TRANSMOG_ACTION_PURCHASE_NOCLASS_LEATHER:
            case TRANSMOG_ACTION_PURCHASE_NOCLASS_MAIL:
            case TRANSMOG_ACTION_PURCHASE_NOCLASS_PLATE:
            case TRANSMOG_ACTION_PURCHASE_PRIEST:
            case TRANSMOG_ACTION_PURCHASE_MAGE:
            case TRANSMOG_ACTION_PURCHASE_WARLOCK:
            case TRANSMOG_ACTION_PURCHASE_DRUID:
            case TRANSMOG_ACTION_PURCHASE_ROGUE:
            case TRANSMOG_ACTION_PURCHASE_PALADIN:
            case TRANSMOG_ACTION_PURCHASE_WARRIOR:
            case TRANSMOG_ACTION_PURCHASE_DEATHKNIGHT:
            case TRANSMOG_ACTION_PURCHASE_SHAMAN:
            case TRANSMOG_ACTION_PURCHASE_HUNTER:
                player->GetSession()->SendListInventory(creature->GetGUID()); break;
            case TRANSMOG_ACTION_SHOW_ENCHANTS:                     TransmogrifyEnchant(player, creature, action);                  break;
            case TRANSMOG_ACTION_SHOW_REMOVE_ITEM_TRANSMOG_OPTIONS:
            {
                switch (action)
                {
                    case TRANSMOG_ACTION_SELECT_REMOVE_ALL_ARMOR:   RemoveAllArmorTransmog(player);                     break;
                    case TRANSMOG_ACTION_SELECT_REMOVE_ALL_WEAPON:  RemoveAllWeaponTransmog(player);                    break;
                    default:                                        RemoveItemTransmog(player, action);                 break;
                }
                break;
            }
            case TRANSMOG_ACTION_SHOW_REMOVE_ENCHANT_TRANSMOG_OPTIONS:
            {
                switch (action)
                {
                    case TRANSMOG_ACTION_SELECT_REMOVE_ALL_ENCHANT: RemoveAllEnchantTransmog(player);      break;
                    default:                                        RemoveEnchantTransmog(player, action); break;
                }
                break;
            }
            case TRANSMOG_ACTION_LIST_SLOT: // Show items you can use
            {
                if (Item* oldItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, action))
                {
                    uint32 lowGUID = player->GetGUIDLow();
                    _items[lowGUID].clear();
                    uint32 limit = 0;
                    uint32 price = 0;
                    switch (sTransmogrification->GetRequireGold())
                    {
                        case 1: { price = (unsigned int)(GetFakePrice(oldItem)*sTransmogrification->GetGoldModifier()); } break;
                        case 2: { price = (unsigned int)sTransmogrification->GetGoldCost(); } break;
                    }

                    std::string item_name = "";

                    const ItemTemplate *oldItemTemplate = oldItem->GetTemplate();

                    for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; i++)
                    {
                        if (limit >= 28)
                            break;

                        if (Item* newItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                        {
                            uint32 display = newItem->GetTemplate()->DisplayInfoID;
                            if (player->CanTransmogItem(oldItem->GetTemplate(), newItem->GetTemplate(), false))
                            {
                                if (_items[lowGUID].find(display) == _items[lowGUID].end())
                                {
                                    limit++;
                                    _items[lowGUID][display] = newItem;
                                    player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_MONEY_BAG, sTransmogrification->GetItemIcon(newItem->GetEntry(), 30, 30, -18, 0)+sTransmogrification->GetItemLink(newItem, session), action * 100, display, "Using this item for transmogrify will bind it to you and make it non-refundable and non-tradeable.\r\nDo you wish to continue?\r\n\r\n", price, false);
                                }
                            }
                        }
                    }

                    for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)
                    {
                        if (Bag* bag = player->GetBagByPos(i))
                        {
                            for (uint32 j = 0; j < bag->GetBagSize(); j++)
                            {
                                if (limit >= 28)
                                    break;

                                if (Item* newItem = player->GetItemByPos(i, j))
                                {
                                    uint32 display = newItem->GetTemplate()->DisplayInfoID;
                                    if (player->CanTransmogItem(oldItem->GetTemplate(), newItem->GetTemplate(), false))
                                    {
                                        if (_items[lowGUID].find(display) == _items[lowGUID].end())
                                        {
                                            limit++;
                                            _items[lowGUID][display] = newItem;
                                            player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_MONEY_BAG,  sTransmogrification->GetItemIcon(newItem->GetEntry(), 30, 30, -18, 0)+sTransmogrification->GetItemLink(newItem, session), action * 100, display, "Using this item for transmogrify will bind it to you and make it non-refundable and non-tradeable.\r\nDo you wish to continue?\r\n\r\n", price, false);
                                        }
                                    }
                                }
                            }
                        }
                    }

                    switch (action)
                    {
                        case EQUIPMENT_SLOT_BODY:
                        {
                            if (ItemTemplate const* newItem = sObjectMgr->GetItemTemplate(230017))
                            {
                                    player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_MONEY_BAG, sTransmogrification->GetSlotIcon(EQUIPMENT_SLOT_BODY, 30, 30, -18, 0) + newItem->Name1, action * 100, 0, "Using this item for transmogrify will bind it to you and make it non-refundable and non-tradeable.\r\nDo you wish to continue?\r\n\r\n" + newItem->Name1, price, false);
                            }
                            break;
                        }
                        case EQUIPMENT_SLOT_TABARD:
                        {
                            if (ItemTemplate const* newItem = sObjectMgr->GetItemTemplate(230018))
                            {
                                    player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_MONEY_BAG, sTransmogrification->GetSlotIcon(EQUIPMENT_SLOT_TABARD, 30, 30, -18, 0) + newItem->Name1, action * 100, 0, "Using this item for transmogrify will bind it to you and make it non-refundable and non-tradeable.\r\nDo you wish to continue?\r\n\r\n" + newItem->Name1, price, false);
                            }
                            break;
                        }
                    }

                    char removeOnePopup[250];
                    snprintf(removeOnePopup, 250, "Remove transmogrification from %s?", GetSlotName(action, session));
                    player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/INV_Enchant_Disenchant:30:30:-18:0|tRemove transmogrification", TRANSMOG_ACTION_REMOVE_TRANSMOG, action, removeOnePopup, 0, false);
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TABARD, "|TInterface/BUTTONS/UI-RotationLeft-Button-Up:35:35:-23:5|tBack", GOSSIP_SENDER_MAIN, TRANSMOG_ACTION_SHOW_TRANSMOGRIFIER);
                    player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
                }
                else
                    OnGossipHello(player, creature);
            }
            break;
            case TRANSMOG_ACTION_REMOVE_TRANSMOG: // Remove Transmogrification from single item
            {
                if (Item* newItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, action))
                {
                    if (newItem->TransmogEntry)
                    {
                        player->UpdateUInt32Value(PLAYER_VISIBLE_ITEM_1_ENTRYID + (newItem->GetSlot() * 2), newItem->GetEntry());

                        newItem->TransmogEntry = 0;
                        TransmogItemInformation tItemInfo;
                        tItemInfo.TransmogEntry = newItem->TransmogEntry;
                        tItemInfo.TransmogEnchant = newItem->TransmogEnchant;
                        player->transmogItemsSaveQueue[newItem->GetGUIDLow()] = tItemInfo;

                        session->SendAreaTriggerMessage("%s transmogrification removed", GetSlotName(action, session));
                        player->PlayDirectSound(3337);
                    }
                    else
                        session->SendNotification("No transmogrification on %s slot", GetSlotName(action, session));
                }
                OnGossipSelect(player, creature, TRANSMOG_ACTION_LIST_SLOT, action);
            } break;
            default: // Transmogrify
            {
                sLog->outString("TRANSMOG_CALL");
                uint32 lowGUID = player->GetGUIDLow();
                if (Item* oldItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, sender / 100))
                {
                        if (_items[lowGUID].find(action) != _items[lowGUID].end() && _items[lowGUID][action]->IsInWorld())
                        {
                            Item* newItem = _items[lowGUID][action];

                            if (newItem->GetOwnerGUID() == player->GetGUIDLow() && (newItem->IsInBag() || newItem->GetBagSlot() == INVENTORY_SLOT_BAG_0) && player->CanTransmogItem(oldItem->GetTemplate(), newItem->GetTemplate(), true))
                            {
                                switch(sTransmogrification->GetRequireGold())
                                {
                                    case 1: { player->ModifyMoney(-1*(uint32)(GetFakePrice(oldItem)*sTransmogrification->GetGoldModifier())); } break;
                                    case 2: { player->ModifyMoney(-1*(unsigned int)sTransmogrification->GetGoldCost()); } break;
                                }
                                player->UpdateUInt32Value(PLAYER_VISIBLE_ITEM_1_ENTRYID + (oldItem->GetSlot() * 2), newItem->GetEntry());

                                oldItem->TransmogEntry = newItem->GetEntry();
                                newItem->SetNotRefundable(player);
                                newItem->SetBinding(true);
                                TransmogItemInformation tItemInfo;
                                tItemInfo.TransmogEntry = oldItem->TransmogEntry;
                                tItemInfo.TransmogEnchant = oldItem->TransmogEnchant;
                                player->transmogItemsSaveQueue[oldItem->GetGUIDLow()] = tItemInfo;
                                player->PlayDirectSound(3337);
                                session->SendAreaTriggerMessage("%s transmogrified", GetSlotName(sender / 100, session));
                            }
                        }
                        else if (!action)
                        {
                            ItemTemplate const* newItem = NULL;
                            switch (sender / 100)
                            {
                                case EQUIPMENT_SLOT_BODY:
                                    newItem = sObjectMgr->GetItemTemplate(230017);
                                    break;
                                case EQUIPMENT_SLOT_TABARD:
                                    newItem = sObjectMgr->GetItemTemplate(230018);
                                    break;
                            }

                            if (newItem && player->CanTransmogItem(oldItem->GetTemplate(), newItem, true))
                            {
                                switch(sTransmogrification->GetRequireGold())
                                {
                                case 1: { player->ModifyMoney(-1*(uint32)(GetFakePrice(oldItem)*sTransmogrification->GetGoldModifier())); } break;
                                case 2: { player->ModifyMoney(-1*(unsigned int)sTransmogrification->GetGoldCost()); } break;
                                }

                                player->UpdateUInt32Value(PLAYER_VISIBLE_ITEM_1_ENTRYID + (oldItem->GetSlot() * 2), newItem->ItemId);

                                oldItem->TransmogEntry = newItem->ItemId;

                                TransmogItemInformation tItemInfo;
                                tItemInfo.TransmogEntry = oldItem->TransmogEntry;
                                tItemInfo.TransmogEnchant = oldItem->TransmogEnchant;
                                player->transmogItemsSaveQueue[oldItem->GetGUIDLow()] = tItemInfo;

                                player->PlayDirectSound(3337);
                                session->SendAreaTriggerMessage("%s transmogrified", GetSlotName(sender / 100, session));
                            }
                            else if (!newItem)
                                session->SendNotification("Selected item does not exist");
                        }
                        else
                            session->SendNotification("Selected item does not exist");
                }

                _items[lowGUID].clear();
                OnGossipSelect(player, creature, TRANSMOG_ACTION_LIST_SLOT, sender / 100);
            } break;
        }

        return true;
    }

    void ShowMaterialTransmogOptions(Player* player, Creature* creature)
    {
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/INV_Helmet_134:30:30:-18:0|tGeneral Armor Sets", TRANSMOG_ACTION_PURSHASE_TRANSMOGS, TRANSMOG_ACTION_PURCHASE_NOCLASS);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/INV_Staff_30:30:30:-18:0|tPriest Armor Sets", TRANSMOG_ACTION_PURSHASE_TRANSMOGS, TRANSMOG_ACTION_PURCHASE_PRIEST); // |cffFFFFFF
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/INV_Staff_13.:30:30:-18:0|tMage Armor Sets", TRANSMOG_ACTION_PURSHASE_TRANSMOGS, TRANSMOG_ACTION_PURCHASE_MAGE); // |cff69CCF0
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/Spell_Nature_Drowsy:30:30:-18:0|tWarlock Armor Sets", TRANSMOG_ACTION_PURSHASE_TRANSMOGS, TRANSMOG_ACTION_PURCHASE_WARLOCK); // |cff9482C9
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/INV_Misc_MonsterClaw_04:30:30:-18:0|tDruid Armor Sets", TRANSMOG_ACTION_PURSHASE_TRANSMOGS, TRANSMOG_ACTION_PURCHASE_DRUID); // |cffb75500
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/INV_ThrowingKnife_04:30:30:-18:0|tRogue Armor Sets", TRANSMOG_ACTION_PURSHASE_TRANSMOGS, TRANSMOG_ACTION_PURCHASE_ROGUE); // |cffFFF569
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/Ability_ThunderBolt:30:30:-18:0|tPaladin Armor Sets", TRANSMOG_ACTION_PURSHASE_TRANSMOGS, TRANSMOG_ACTION_PURCHASE_PALADIN); // |cffe2639a
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/INV_Sword_27:30:30:-18:0|tWarrior Armor Sets", TRANSMOG_ACTION_PURSHASE_TRANSMOGS, TRANSMOG_ACTION_PURCHASE_WARRIOR); // |cff6b4721
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/Spell_Deathknight_ClassIcon:30:30:-18:0|tDeath Knight Armor Sets", TRANSMOG_ACTION_PURSHASE_TRANSMOGS, TRANSMOG_ACTION_PURCHASE_DEATHKNIGHT); // |cffC41F3B
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/INV_Jewelry_Talisman_04:30:30:-18:0|tShaman Armor Sets", TRANSMOG_ACTION_PURSHASE_TRANSMOGS, TRANSMOG_ACTION_PURCHASE_SHAMAN); // |cff0070DE
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/INV_Weapon_Bow_07:30:30:-18:0|tHunter Armor Sets", TRANSMOG_ACTION_PURSHASE_TRANSMOGS, TRANSMOG_ACTION_PURCHASE_HUNTER); // |cff72c400

        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/INV_Axe_109:30:30:-18:0|t|cffFF0000Weapons", GOSSIP_SENDER_MAIN, TRANSMOG_ACTION_PURSHASE_TRANSMOGS);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/INV_Misc_Cape_23:30:30:-18:0|t|cffFF0000Cloaks", GOSSIP_SENDER_MAIN, TRANSMOG_ACTION_PURSHASE_TRANSMOGS);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TABARD, "|TInterface/BUTTONS/UI-RotationLeft-Button-Up:35:35:-23:5|tBack", GOSSIP_SENDER_MAIN, GOSSIP_SENDER_MAIN);
        player->SEND_GOSSIP_MENU(1, creature->GetGUID());
    }
 
    void ShowTransmogOptions(Player* player, Creature* creature)
    {
        WorldSession* session = player->GetSession();
        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; slot++)
        {
            if (Item* newItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            {
                if (sTransmogrification->AllowedQuality(newItem->GetTemplate()->Quality, newItem->GetTemplate()->InventoryType))
                {
                    if (const char* slotName = GetSlotName(slot, session))
                    {
                        Item* newItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
                        uint32 entry = newItem ? newItem->TransmogEntry : 0;
                        std::string icon = entry ? sTransmogrification->GetItemIcon(entry,30,30,-18,0) : sTransmogrification->GetSlotIcon(slot, 30, 30, -18, 0);
                        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, icon+std::string(slotName), TRANSMOG_ACTION_LIST_SLOT, slot);
                    }
                }
            }
        }
        player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\spell_Shadow_SacrificialShield:30:30:-18:0|tRemove All Armor Transmogs", TRANSMOG_ACTION_SHOW_REMOVE_ITEM_TRANSMOG_OPTIONS, TRANSMOG_ACTION_SELECT_REMOVE_ALL_ARMOR, "Are you sure you wish to remove all armor transmogrifications?", 0, false);
        player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\spell_Shadow_SacrificialShield:30:30:-18:0|tRemove All Weapon Transmogs", TRANSMOG_ACTION_SHOW_REMOVE_ITEM_TRANSMOG_OPTIONS, TRANSMOG_ACTION_SELECT_REMOVE_ALL_WEAPON, "Are you sure you wish to remove all weapon transmogrifications?", 0, false);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TABARD, "|TInterface/BUTTONS/UI-RotationLeft-Button-Up:35:35:-23:5|tBack", GOSSIP_SENDER_MAIN, GOSSIP_SENDER_MAIN);

        player->SEND_GOSSIP_MENU(1, creature->GetGUID());
    }

    void ShowTransmogEnchants(Player* player, Creature* creature)
    {
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\Inv_misc_gem_pearl_06:25:25:-18:0|tMongoose", TRANSMOG_ACTION_SHOW_ENCHANTS, TRANSMOG_ENCHANT_MONGOOSE);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\Inv_elemental_primal_air:25:25:-18:0|tExecutioner", TRANSMOG_ACTION_SHOW_ENCHANTS, TRANSMOG_ENCHANT_EXECUTIONER);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\Inv_elemental_mote_nether:25:25:-18:0|tSunfire", TRANSMOG_ACTION_SHOW_ENCHANTS, TRANSMOG_ENCHANT_SUNFIRE);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\Inv_elemental_mote_water01:25:25:-18:0|tSoulfrost", TRANSMOG_ACTION_SHOW_ENCHANTS, TRANSMOG_ENCHANT_SOULFROST);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\Inv_elemental_mote_fire01:25:25:-18:0|tBlood Draining", TRANSMOG_ACTION_SHOW_ENCHANTS, TRANSMOG_ENCHANT_BLOOD_DRAINING);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\Inv_elemental_primal_mana:25:25:-18:0|tRazorice", TRANSMOG_ACTION_SHOW_ENCHANTS, TRANSMOG_ENCHANT_RAZORICE);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\Inv_elemental_mote_fire01:25:25:-18:0|tBerserking", TRANSMOG_ACTION_SHOW_ENCHANTS, TRANSMOG_ENCHANT_BERSERKING);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\Inv_elemental_mote_mana:25:25:-18:0|tSpellsurge", TRANSMOG_ACTION_SHOW_ENCHANTS, TRANSMOG_ENCHANT_SPELLSURGE);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\Inv_elemental_primal_life:25:25:-18:0|tPotency", TRANSMOG_ACTION_SHOW_ENCHANTS, TRANSMOG_ENCHANT_POTENCY);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\Inv_enchant_essenceeternallarge:25:25:-18:0|tDeathfrost", TRANSMOG_ACTION_SHOW_ENCHANTS, TRANSMOG_ENCHANT_DEATHFROST);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\Inv_enchant_essenceeternalsmall:25:25:-18:0|tSparkles", TRANSMOG_ACTION_SHOW_ENCHANTS, TRANSMOG_ENCHANT_SPARKLES);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\Inv_elemental_mote_fire01:25:25:-18:0|tRed Glow", TRANSMOG_ACTION_SHOW_ENCHANTS, TRANSMOG_ENCHANT_GLOW_RED);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\Inv_elemental_primal_nether:25:25:-18:0|tYellow Glow", TRANSMOG_ACTION_SHOW_ENCHANTS, TRANSMOG_ENCHANT_GLOW_YELLOW);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\Inv_elemental_primal_mana:25:25:-18:0|tBlue Glow", TRANSMOG_ACTION_SHOW_ENCHANTS, TRANSMOG_ENCHANT_GLOW_BLUE);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\Inv_elemental_primal_shadow:25:25:-18:0|tPurple Glow", TRANSMOG_ACTION_SHOW_ENCHANTS, TRANSMOG_ENCHANT_GLOW_PURPLE);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\Inv_elemental_primal_fire:25:25:-18:0|tOrange Glow", TRANSMOG_ACTION_SHOW_ENCHANTS, TRANSMOG_ENCHANT_GLOW_ORANGE);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\Inv_elemental_primal_life:25:25:-18:0|tGreen Glow", TRANSMOG_ACTION_SHOW_ENCHANTS, TRANSMOG_ENCHANT_GLOW_GREEN);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\Inv_elemental_primal_earth:25:25:-18:0|tGrey Glow", TRANSMOG_ACTION_SHOW_ENCHANTS, TRANSMOG_ENCHANT_GLOW_GREY);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\Inv_misc_gem_pearl_08:25:25:-18:0|tWhite Glow", TRANSMOG_ACTION_SHOW_ENCHANTS, TRANSMOG_ENCHANT_GLOW_WHITE);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\Inv_sword_04:25:25:-18:0|tNo Visual", TRANSMOG_ACTION_SHOW_ENCHANTS, TRANSMOG_ENCHANT_NO_VISUAL);

        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\spell_Shadow_SacrificialShield:30:30:-18:0|tRemove Enchant Transmogs", GOSSIP_SENDER_MAIN, TRANSMOG_ACTION_SHOW_REMOVE_ENCHANT_TRANSMOG_OPTIONS);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TABARD, "|TInterface/BUTTONS/UI-RotationLeft-Button-Up:35:35:-23:5|tBack", GOSSIP_SENDER_MAIN, GOSSIP_SENDER_MAIN);

        player->SEND_GOSSIP_MENU(1, creature->GetGUID());
    }

    void ShowRemoveTransmogEnchantOptions(Player* player, Creature* creature)
    {
        if (player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND))
            player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\Inv_sword_67:30:30:-18:0|tMain Hand", TRANSMOG_ACTION_SHOW_REMOVE_ENCHANT_TRANSMOG_OPTIONS, TRANSMOG_ACTION_SELECT_REMOVE_MAINHAND_ENCHANT, "Are you sure you wish to remove your main hand enchant transmogrifications?", 0, false);

        if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND))
            if (item->GetTemplate()->Class == ITEM_CLASS_WEAPON)
                player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\Inv_sword_66:30:30:-18:0|tOff Hand", TRANSMOG_ACTION_SHOW_REMOVE_ENCHANT_TRANSMOG_OPTIONS, TRANSMOG_ACTION_SELECT_REMOVE_OFFHAND_ENCHANT, "Are you sure you wish to remove your off hand enchant transmogrifications?", 0, false);

        player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_MONEY_BAG, "|TInterface\\icons\\spell_Shadow_SacrificialShield:30:30:-18:0|tRemove All Enchant Transmogs", TRANSMOG_ACTION_SHOW_REMOVE_ENCHANT_TRANSMOG_OPTIONS, TRANSMOG_ACTION_SELECT_REMOVE_ALL_ENCHANT, "Are you sure you wish to remove all enchant transmogrifications?", 0, false);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/Ability_Spy:25:25:-18:0|tBack", GOSSIP_SENDER_MAIN, TRANSMOG_ACTION_SHOW_ENCHANTS);

        player->SEND_GOSSIP_MENU(1, creature->GetGUID());
    }

    void TransmogrifyEnchant(Player* player, Creature* creature, uint16 action)
    {
        if (action && player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND) != 0)
        {
            if (Item* pItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND))
                if (const ItemTemplate* pItemTemplate = pItem->GetTemplate())
                    if (pItemTemplate->Class == ITEM_CLASS_WEAPON)
                    {
                        player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND)->TransmogEnchant = action;
                        player->SetUInt16Value(TRANSMOG_SLOT_MAINHAND_ENCHANT, 0, action);
                        player->SetUInt16Value(TRANSMOG_SLOT_MAINHAND_ENCHANT, 1, action);
                        TransmogItemInformation tItemInfo;
                        tItemInfo.TransmogEntry = pItem->TransmogEntry;
                        tItemInfo.TransmogEnchant = pItem->TransmogEnchant;
                        player->transmogItemsSaveQueue[pItem->GetGUIDLow()] = tItemInfo;
                        player->PlayDirectSound(3337);
                    }
        }

        if (action && player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND) != 0)
        {
            if (Item* pItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND))
                if (const ItemTemplate* pItemTemplate = pItem->GetTemplate())
                    if (pItemTemplate->Class == ITEM_CLASS_WEAPON)
                    {
                        player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND)->TransmogEnchant = action;
                        player->SetUInt16Value(TRANSMOG_SLOT_OFFHAND_ENCHANT, 0, action);
                        player->SetUInt16Value(TRANSMOG_SLOT_OFFHAND_ENCHANT, 1, action);
                        TransmogItemInformation tItemInfo;
                        tItemInfo.TransmogEntry = pItem->TransmogEntry;
                        tItemInfo.TransmogEnchant = pItem->TransmogEnchant;
                        player->transmogItemsSaveQueue[pItem->GetGUIDLow()] = tItemInfo;
                        player->PlayDirectSound(3337);
                    }
        }

        ShowTransmogEnchants(player, creature);
    }

    void RemoveItemTransmog(Player* player, uint16 action)
    {
        uint8 itemSlot = sTransmogrification->GetItemSlotByAction(action);

        if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, itemSlot))
        {
            item->TransmogEntry = 0;
            player->SetUInt32Value(PLAYER_VISIBLE_ITEM_1_ENTRYID + (itemSlot * 2), item->GetEntry());
            TransmogItemInformation tItemInfo;
            tItemInfo.TransmogEntry = item->TransmogEntry;
            tItemInfo.TransmogEnchant = item->TransmogEnchant;
            player->transmogItemsSaveQueue[item->GetGUIDLow()] = tItemInfo;
        }

        player->CLOSE_GOSSIP_MENU();
    }

    void RemoveEnchantTransmog(Player* player, uint16 action)
    {
        uint8 itemSlot = sTransmogrification->GetItemSlotByAction(action);

        if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, itemSlot))
        {
            item->TransmogEnchant = 0;
            player->SetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (itemSlot * 2), 0, item->GetEnchantmentId(PERM_ENCHANTMENT_SLOT));
            player->SetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (itemSlot * 2), 1, item->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT));
            TransmogItemInformation tItemInfo;
            tItemInfo.TransmogEntry = item->TransmogEntry;
            tItemInfo.TransmogEnchant = item->TransmogEnchant;
            player->transmogItemsSaveQueue[item->GetGUIDLow()] = tItemInfo;
        }

        player->CLOSE_GOSSIP_MENU();
    }

    void RemoveAllArmorTransmog(Player* player)
    {
        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; slot++)
        {
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            {
                if (slot == EQUIPMENT_SLOT_HEAD ||
                    slot == EQUIPMENT_SLOT_SHOULDERS ||
                    slot == EQUIPMENT_SLOT_BACK ||
                    slot == EQUIPMENT_SLOT_CHEST ||
                    slot == EQUIPMENT_SLOT_BODY ||
                    slot == EQUIPMENT_SLOT_TABARD ||
                    slot == EQUIPMENT_SLOT_WRISTS ||
                    slot == EQUIPMENT_SLOT_HANDS ||
                    slot == EQUIPMENT_SLOT_WAIST ||
                    slot == EQUIPMENT_SLOT_LEGS ||
                    slot == EQUIPMENT_SLOT_FEET)
                {
                    item->TransmogEntry = 0;
                    player->SetUInt32Value(PLAYER_VISIBLE_ITEM_1_ENTRYID + (slot * 2), item->GetEntry());
                    TransmogItemInformation tItemInfo;
                    tItemInfo.TransmogEntry = item->TransmogEntry;
                    tItemInfo.TransmogEnchant = item->TransmogEnchant;
                    player->transmogItemsSaveQueue[item->GetGUIDLow()] = tItemInfo;
                }
            }
        }

        player->CLOSE_GOSSIP_MENU();
    }

    void RemoveAllWeaponTransmog(Player* player)
    {
        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; slot++)
        {
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            {
                if (slot == EQUIPMENT_SLOT_MAINHAND ||
                    slot == EQUIPMENT_SLOT_OFFHAND ||
                    slot == EQUIPMENT_SLOT_RANGED)
                {
                    item->TransmogEntry = 0;
                    player->SetUInt32Value(PLAYER_VISIBLE_ITEM_1_ENTRYID + (slot * 2), item->GetEntry());
                    TransmogItemInformation tItemInfo;
                    tItemInfo.TransmogEntry = item->TransmogEntry;
                    tItemInfo.TransmogEnchant = item->TransmogEnchant;
                    player->transmogItemsSaveQueue[item->GetGUIDLow()] = tItemInfo;
                }
            }
        }

        player->CLOSE_GOSSIP_MENU();
    }

    void RemoveAllEnchantTransmog(Player* player)
    {
        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; slot++)
        {
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            {
                if (slot == EQUIPMENT_SLOT_MAINHAND ||
                    slot == EQUIPMENT_SLOT_OFFHAND ||
                    slot == EQUIPMENT_SLOT_RANGED)
                {
                    item->TransmogEnchant = 0;
                    player->SetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (slot * 2), 0, item->GetEnchantmentId(PERM_ENCHANTMENT_SLOT));
                    player->SetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (slot * 2), 1, item->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT));
                    TransmogItemInformation tItemInfo;
                    tItemInfo.TransmogEntry = item->TransmogEntry;
                    tItemInfo.TransmogEnchant = item->TransmogEnchant;
                    player->transmogItemsSaveQueue[item->GetGUIDLow()] = tItemInfo;
                }
            }
        }

        player->CLOSE_GOSSIP_MENU();
    }

    void RemoveAllTransmog(Player* player)
    {
        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; slot++)
        {
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            {
                if (slot == EQUIPMENT_SLOT_HEAD ||
                    slot == EQUIPMENT_SLOT_SHOULDERS ||
                    slot == EQUIPMENT_SLOT_BACK ||
                    slot == EQUIPMENT_SLOT_CHEST ||
                    slot == EQUIPMENT_SLOT_BODY ||
                    slot == EQUIPMENT_SLOT_TABARD ||
                    slot == EQUIPMENT_SLOT_WRISTS ||
                    slot == EQUIPMENT_SLOT_HANDS ||
                    slot == EQUIPMENT_SLOT_WAIST ||
                    slot == EQUIPMENT_SLOT_LEGS ||
                    slot == EQUIPMENT_SLOT_FEET ||
                    slot == EQUIPMENT_SLOT_MAINHAND ||
                    slot == EQUIPMENT_SLOT_OFFHAND ||
                    slot == EQUIPMENT_SLOT_RANGED)
                {
                    item->TransmogEntry = 0;
                    player->SetUInt32Value(PLAYER_VISIBLE_ITEM_1_ENTRYID + (slot * 2), item->GetEntry());
                    TransmogItemInformation tItemInfo;
                    tItemInfo.TransmogEntry = item->TransmogEntry;
                    tItemInfo.TransmogEnchant = item->TransmogEnchant;
                    player->transmogItemsSaveQueue[item->GetGUIDLow()] = tItemInfo;
                }

                if (slot == EQUIPMENT_SLOT_MAINHAND ||
                    slot == EQUIPMENT_SLOT_OFFHAND ||
                    slot == EQUIPMENT_SLOT_RANGED)
                {
                    item->TransmogEnchant = 0;
                    player->SetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (slot * 2), 0, item->GetEnchantmentId(PERM_ENCHANTMENT_SLOT));
                    player->SetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (slot * 2), 1, item->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT));
                    TransmogItemInformation tItemInfo;
                    tItemInfo.TransmogEntry = item->TransmogEntry;
                    tItemInfo.TransmogEnchant = item->TransmogEnchant;
                    player->transmogItemsSaveQueue[item->GetGUIDLow()] = tItemInfo;
                }
            }
        }

        player->CLOSE_GOSSIP_MENU();
    }
private:
    std::map<uint64, std::map<uint32, Item*> > _items; // _items[lowGUID][DISPLAY] = item

    const char * GetSlotName(uint8 slot, WorldSession* session)
    {
        switch (slot)
        {
        case EQUIPMENT_SLOT_HEAD      : return "Head";
        case EQUIPMENT_SLOT_SHOULDERS : return "Shoulders";
        case EQUIPMENT_SLOT_BODY      : return "Shirt";
        case EQUIPMENT_SLOT_CHEST     : return "Chest";
        case EQUIPMENT_SLOT_WAIST     : return "Waist";
        case EQUIPMENT_SLOT_LEGS      : return "Legs";
        case EQUIPMENT_SLOT_FEET      : return "Boots";
        case EQUIPMENT_SLOT_WRISTS    : return "Wrists";
        case EQUIPMENT_SLOT_HANDS     : return "Hands";
        case EQUIPMENT_SLOT_BACK      : return "Cloak";
        case EQUIPMENT_SLOT_MAINHAND  : return "Main Hand";
        case EQUIPMENT_SLOT_OFFHAND   : return "Off Hand";
        case EQUIPMENT_SLOT_RANGED    : return "Ranged";
        case EQUIPMENT_SLOT_TABARD    : return "Tabard";
        default: return NULL;
        }
    }

    uint32 GetFakePrice(Item* item)
    {
        uint32 sellPrice = item->GetTemplate()->SellPrice;
        uint32 minPrice = item->GetTemplate()->RequiredLevel * 1176;
        if (sellPrice < minPrice)
            sellPrice = minPrice;
        return sellPrice;
    }
};

class config_Transmogrify : public WorldScript
{
public:
    config_Transmogrify() : WorldScript("config_Transmogrify") { }

    void OnConfigLoad(bool reload)
    {
        sTransmogrification->LoadConfig();
    }
};

void AddSC_npc_transmogrify()
{
    new npc_transmogrify();
    new config_Transmogrify();
}
