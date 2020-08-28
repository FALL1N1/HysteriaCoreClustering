#include "Chat.h"
#include "Language.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "WaypointManager.h"

class progressive_commandscript : public CommandScript
{
public:
    progressive_commandscript() : CommandScript("progressive_commandscript") { }

    ChatCommand* GetCommands() const
    {
        static ChatCommand progressiveCT[] =
        {
            { "npc",                  SEC_GAMEMASTER,     false, &HandleWpAddCommand,                "", NULL },
            { "object",               SEC_GAMEMASTER,     false, &HandleWpAddCommand,                "", NULL },
            { "item",                 SEC_GAMEMASTER,     false, &HandleWpAddCommand,                "", NULL },
            { "quest",                SEC_GAMEMASTER,     false, &HandleWpAddCommand,                "", NULL },
            // { NULL,             0,                  false, NULL,                               "", NULL }
        };
        static ChatCommand commandTable[] =
        {
            { "progressive",             SEC_GAMEMASTER,     false, NULL,                     "", progressiveCT },
            // { NULL,             0,                  false, NULL,                               "", NULL }
        };
        return commandTable;
    }
    /**
    * Add a waypoint to a creature.
    *
    * The user can either select an npc or provide its GUID.
    *
    * The user can even select a visual waypoint - then the new waypoint
    * is placed *after* the selected one - this makes insertion of new
    * waypoints possible.
    *
    * eg:
    * .wp add 12345
    * -> adds a waypoint to the npc with the GUID 12345
    *
    * .wp add
    * -> adds a waypoint to the currently selected creature
    *
    *
    * @param args if the user did not provide a GUID, it is NULL
    *
    * @return true - command did succeed, false - something went wrong
    */
    static bool HandleWpAddCommand(ChatHandler* handler, const char* args)
    {
        Creature* target = handler->getSelectedCreature();
    
        if(!target)
            handler->PSendSysMessage("You have to target NPC first in order to make it progressable.");

        handler->PSendSysMessage("%s%s%u%s%u%s|r", "|cff00ff00", "PathID: |r|cff00ffff", pathid, "|r|cff00ff00: Waypoint |r|cff00ffff", point+1, "|r|cff00ff00 created. ");
        return true;
    }                                                           // HandleWpAddCommand
};

void AddSC_progressive_commandscript()
{
    new progressive_commandscript();
}
