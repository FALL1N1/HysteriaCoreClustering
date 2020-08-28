#include "ScriptMgr.h"
#include "ObjectMgr.h"
#include "Chat.h"

class node_commandscript : public CommandScript
{
public:
    node_commandscript() : CommandScript("node_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> nodeCommandTable =
        {
            { "nodes",               SEC_ADMINISTRATOR,  false, &HandleNodeCommand,       "" },
            { "goto",             SEC_ADMINISTRATOR,  false, &HandleNodeChangeCommand, "" }
        };
        static std::vector<ChatCommand> CommandTable =
        {
            { "cluster",             SEC_ADMINISTRATOR,  false, NULL,            "", nodeCommandTable },
        };
        return CommandTable;
    }

    static bool HandleNodeCommand(ChatHandler* handler, const char* args)
    {
        if (*args)
            return false;

        std::stringstream str1, str2;
        handler->SendSysMessage("--------------------------------");
        handler->SendSysMessage("- Project BalnaZZar Clustering -");
        str1 << "Hello, " << handler->GetSession()->GetPlayerName() << "!";
        handler->SendSysMessage(str1.str().c_str());
        str2 << "You are located at node(" << realmID << ")";
        handler->SendSysMessage(str2.str().c_str());
        handler->SendSysMessage("--------------------------------");
        return true;
    }

    static bool HandleNodeChangeCommand(ChatHandler* handler, const char* args)
    {
        if (!*args)
            return false;

        Unit* unit = handler->getSelectedUnit();
        Player* player = NULL;
        if (!unit || (unit->GetTypeId() != TYPEID_PLAYER))
            player = handler->GetSession()->GetPlayer();
        else
            player = (Player*)unit;

        if (!unit)
            unit = player;

        uint32 nodeId = atoi(args);
        player->GetSession()->SendChangeNode(nodeId);

        return true;
    }

    static bool HandleNodeListCommand(ChatHandler* handler, const char* args)
    {
        handler->SendSysMessage("Not implemented");
        return true;
    }
};

void AddSC_node_commandscript()
{
    new node_commandscript();
}