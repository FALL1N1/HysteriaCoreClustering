#include "ScriptPCH.h"

class npc_temporarytransmog : public CreatureScript
{
public:
    npc_temporarytransmog() : CreatureScript("npc_temporarytransmog") { }

    bool OnGossipHello(Player *player, Creature *creature)
    {
        if (!sWorld->getBoolConfig(CUSTOM_SCRIPTS_ENABLED_OR_NOT))
            return false;

        player->ADD_GOSSIP_ITEM( 3, "|cffBB0000Legendaries", GOSSIP_SENDER_MAIN, 1);
        player->ADD_GOSSIP_ITEM( 3, "|cffFF0000No, thanks! I dont want to transform my gear.",GOSSIP_SENDER_MAIN, 999); 

        player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());         
        return true;
    }

    bool OnGossipSelect(Player *player, Creature *creature, uint32 sender, uint32 uiAction)
    {
        if (sender == GOSSIP_SENDER_MAIN)
        {
            player->PlayerTalkClass->ClearMenus();
            switch(uiAction)
            {
                case 999:
                    player->CLOSE_GOSSIP_MENU();
                    creature->MonsterWhisper("Come again, friend!", player);
                    break;			
                case 998:
                    player->CLOSE_GOSSIP_MENU(); 
                    break;
                case 1://Tier 3
                    player->ADD_GOSSIP_ITEM( 3, "|cff3300FFThunderfury", GOSSIP_SENDER_MAIN, 100);
                    player->ADD_GOSSIP_ITEM( 3, "|cff3300FFWarglaive of Azzinoth(Left)", GOSSIP_SENDER_MAIN, 101);
                    player->ADD_GOSSIP_ITEM( 3, "|cff3300FFWarglaive of Azzinoth(Right)", GOSSIP_SENDER_MAIN, 102);
                    player->ADD_GOSSIP_ITEM( 3, "|cff3300FFThori'dal", GOSSIP_SENDER_MAIN, 103);
                    player->ADD_GOSSIP_ITEM( 3, "|cff3300FFNetherstrand Longbow", GOSSIP_SENDER_MAIN, 104);
                    player->ADD_GOSSIP_ITEM( 3, "|cff3300FFDevastation", GOSSIP_SENDER_MAIN, 105);
                    player->ADD_GOSSIP_ITEM( 3, "|cff3300FFVal'Anyr", GOSSIP_SENDER_MAIN, 106);
                    player->ADD_GOSSIP_ITEM( 3, "|cff3300FFSulfuras", GOSSIP_SENDER_MAIN, 107);
                    player->ADD_GOSSIP_ITEM( 3, "|cff3300FFWarp Slicer", GOSSIP_SENDER_MAIN, 108);
                    player->ADD_GOSSIP_ITEM( 3, "|cff3300FFStaff of Disintegration", GOSSIP_SENDER_MAIN, 109);
                    player->ADD_GOSSIP_ITEM( 3, "|cff3300FFAndonisus", GOSSIP_SENDER_MAIN, 110);
                    player->ADD_GOSSIP_ITEM( 3, "|cff3300FFInfinity Blade", GOSSIP_SENDER_MAIN, 111);
                                
                    player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());         
                    break;		
                case 100:
                    player->UpdateUInt32Value(PLAYER_VISIBLE_ITEM_16_ENTRYID, 19019); 
                    player->CLOSE_GOSSIP_MENU();
                    creature->MonsterWhisper("Your weapon has been Transformed!", player);
                    break;
                case 101:
                    player->UpdateUInt32Value(PLAYER_VISIBLE_ITEM_16_ENTRYID, 32837); 
                    player->CLOSE_GOSSIP_MENU();
                    creature->MonsterWhisper("Your weapon has been Transformed!", player);
                    break;
                case 102:
                    player->UpdateUInt32Value(PLAYER_VISIBLE_ITEM_17_ENTRYID, 32838);
                    player->CLOSE_GOSSIP_MENU();
                    creature->MonsterWhisper("Your weapon has been Transformed!", player);
                    break;
                case 103:
                    player->UpdateUInt32Value(PLAYER_VISIBLE_ITEM_18_ENTRYID, 34334); 
                    player->CLOSE_GOSSIP_MENU();
                    creature->MonsterWhisper("Your weapon has been Transformed!", player);
                    break;
                case 104:
                    player->UpdateUInt32Value(PLAYER_VISIBLE_ITEM_18_ENTRYID, 30318); 
                    player->CLOSE_GOSSIP_MENU();
                    creature->MonsterWhisper("Your weapon has been Transformed!", player);
                    break;
                case 105:
                    player->UpdateUInt32Value(PLAYER_VISIBLE_ITEM_16_ENTRYID, 30316); 
                    player->CLOSE_GOSSIP_MENU();
                    creature->MonsterWhisper("Your weapon has been Transformed!", player);
              break;
                case 106:
                    player->UpdateUInt32Value(PLAYER_VISIBLE_ITEM_16_ENTRYID, 46017); 
                    player->CLOSE_GOSSIP_MENU();
                    creature->MonsterWhisper("Your weapon has been Transformed!", player);
                    break;
                case 107:
                    player->UpdateUInt32Value(PLAYER_VISIBLE_ITEM_16_ENTRYID, 17182); 
                    player->CLOSE_GOSSIP_MENU();
                    creature->MonsterWhisper("Your weapon has been Transformed!", player);
                    break;
                case 108:
                    player->UpdateUInt32Value(PLAYER_VISIBLE_ITEM_16_ENTRYID, 30311); 
                    player->CLOSE_GOSSIP_MENU();
                    creature->MonsterWhisper("Your weapon has been Transformed!", player);
                    break;
                case 109:
                    player->UpdateUInt32Value(PLAYER_VISIBLE_ITEM_16_ENTRYID, 30313); 
                    player->CLOSE_GOSSIP_MENU();
                    creature->MonsterWhisper("Your weapon has been Transformed!", player);
                    break;
                case 110:
                    player->UpdateUInt32Value(PLAYER_VISIBLE_ITEM_16_ENTRYID, 22736); 
                    player->CLOSE_GOSSIP_MENU();
                    creature->MonsterWhisper("Your weapon has been Transformed!", player);
                    break;
                case 111:
                    player->UpdateUInt32Value(PLAYER_VISIBLE_ITEM_16_ENTRYID, 30312); 
                    player->CLOSE_GOSSIP_MENU();
                    creature->MonsterWhisper("Your weapon has been Transformed!", player);
                    break;
                default:
                    break;                   
            }
        } return true;
    }
};

void AddSC_npc_temporarytransmog()
{
    new npc_temporarytransmog();
}