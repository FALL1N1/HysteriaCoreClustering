/*

 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* ScriptData
Name: server_commandscript
%Complete: 100
Comment: All server related commands
Category: commandscripts
EndScriptData */

#include "Chat.h"
#include "Config.h"
#include "Language.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SystemConfig.h"
#include "AvgDiffTracker.h"

class server_commandscript : public CommandScript
{
public:
    server_commandscript() : CommandScript("server_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> serverIdleRestartCommandTable =
        {
            { "cancel",         SEC_ADMINISTRATOR,  true,  &HandleServerShutDownCancelCommand,      ""},
            { ""   ,            SEC_ADMINISTRATOR,  true,  &HandleServerIdleRestartCommand,         ""},
            // { NULL,             0,                  false, NULL,                                    ""}
        };

        static std::vector<ChatCommand> serverIdleShutdownCommandTable =
        {
            { "cancel",         SEC_ADMINISTRATOR,  true,  &HandleServerShutDownCancelCommand,      ""},
            { ""   ,            SEC_ADMINISTRATOR,  true,  &HandleServerIdleShutDownCommand,        ""},
            // { NULL,             0,                  false, NULL,                                    ""}
        };

        static std::vector<ChatCommand> serverRestartCommandTable =
        {
            { "cancel",         SEC_ADMINISTRATOR,  true,  &HandleServerShutDownCancelCommand,      ""},
            { ""   ,            SEC_ADMINISTRATOR,  true,  &HandleServerRestartCommand,             ""},
            // { NULL,             0,                  false, NULL,                                    ""}
        };

        static std::vector<ChatCommand> serverShutdownCommandTable =
        {
            { "cancel",         SEC_ADMINISTRATOR,  true,  &HandleServerShutDownCancelCommand,      ""},
            { ""   ,            SEC_ADMINISTRATOR,  true,  &HandleServerShutDownCommand,            ""},
            // { NULL,             0,                  false, NULL,                                    ""}
        };

        static std::vector<ChatCommand> serverSetCommandTable =
        {
            { "difftime",       SEC_CONSOLE,        true,  &HandleServerSetDiffTimeCommand,         ""},
            { "loglevel",       SEC_CONSOLE,        true,  &HandleServerSetLogLevelCommand,         ""},
            { "logfilelevel",   SEC_CONSOLE,        true,  &HandleServerSetLogFileLevelCommand,     ""},
            { "motd",           SEC_ADMINISTRATOR,  true,  &HandleServerSetMotdCommand,             ""},
            { "closed",         SEC_ADMINISTRATOR,  true,  &HandleServerSetClosedCommand,           ""},
            // { NULL,             0,                  false, NULL,                                    ""}
        };

        static std::vector<ChatCommand> serverCommandTable =
        {
            { "corpses",        SEC_GAMEMASTER,     true,  &HandleServerCorpsesCommand,             ""},
            { "exit",           SEC_CONSOLE,        true,  &HandleServerExitCommand,                ""},
            { "idlerestart",    SEC_ADMINISTRATOR,  true,  NULL,                                    "", serverIdleRestartCommandTable },
            { "idleshutdown",   SEC_ADMINISTRATOR,  true,  NULL,                                    "", serverIdleShutdownCommandTable },
            { "info",           SEC_PLAYER,         true,  &HandleServerInfoCommand,                ""},
            {"stats",           SEC_PLAYER,         true,  &HandleServerStatsCommand, ""},
            { "motd",           SEC_PLAYER,         true,  &HandleServerMotdCommand,                ""},
            { "restart",        SEC_ADMINISTRATOR,  true,  NULL,                                    "", serverRestartCommandTable },
            { "shutdown",       SEC_ADMINISTRATOR,  true,  NULL,                                    "", serverShutdownCommandTable },
            { "set",            SEC_ADMINISTRATOR,  true,  NULL,                                    "", serverSetCommandTable },
            { "togglequerylog", SEC_CONSOLE,        true,  &HandleServerToggleQueryLogging,         ""},
            // { NULL,             0,                  false, NULL,                                    ""}
        };

         static std::vector<ChatCommand> CommandTable =
        {
            { "server",         SEC_ADMINISTRATOR,  true,  NULL,                                    "", serverCommandTable },
            // { NULL,             0,                  false, NULL,                                    ""}
        };
        return CommandTable;
    }


    static bool HandleServerStatsCommand(ChatHandler* handler, char const* args)
    {
        std::string realmName = sWorld->GetRealmName();
        uint32 playerCount = sWorld->GetPlayerCount();
        uint32 activeSessionCount = sWorld->GetActiveSessionCount();
        uint32 queuedSessionCount = sWorld->GetQueuedSessionCount();
        uint32 maxplrs = sWorld->GetMaxRealmPlayers();
        uint32 connPeak = sWorld->GetMaxActiveSessionCount();

        uint32 int_h = sWorld->GetHordeAllianceOnline(false);
        uint32 int_a = sWorld->GetHordeAllianceOnline(true);

        std::string uptime = secsToTimeString(sWorld->GetUptime()).append(".");
        uint32 updateTime = sWorld->GetUpdateTime();
        uint32 avgUpdateTime = avgDiffTracker.getAverage();
        /* let's generate our output */
        handler->PSendSysMessage("%s", _FULLVERSION);
        handler->PSendSysMessage("Current Patch: 3.0 - Echoes of Doom (Naxxramas)");
        handler->PSendSysMessage("Total: |cff3E9448%u|r | Ingame: |cff3E9448%u|r | Ever: |cff3E9448%u|r | Today: |cff3E9448%u|r", activeSessionCount, playerCount, maxplrs, connPeak);
        handler->PSendSysMessage("Horde has |cffFF0000%u|r online. Alliance has |cff0000FF%u|r online.", int_h, int_a);
        handler->PSendSysMessage("Server uptime is: |cff3E9448%s|r", uptime.c_str());
        handler->PSendSysMessage("|cffFF0000*|rNote: Players information is cached and updated once every 5 - 10 minutes.");

        if (sWorld->IsShuttingDown())
            handler->PSendSysMessage("Realm will be |cffFF0000taken down for updates|r, time left till restart: |cffFF0000%s|r", secsToTimeString(sWorld->GetShutDownTimeLeft()).append(".").c_str());
        return true;
    }


    // Triggering corpses expire check in world
    static bool HandleServerCorpsesCommand(ChatHandler* /*handler*/, char const* /*args*/)
    {
        sObjectAccessor->RemoveOldCorpses();
        return true;
    }

    static bool HandleServerInfoCommand(ChatHandler* handler, char const* /*args*/)
    {
        std::string realmName = sWorld->GetRealmName();
        uint32 playerCount = sWorld->GetPlayerCount();
        uint32 activeSessionCount = sWorld->GetActiveSessionCount();
        uint32 queuedSessionCount = sWorld->GetQueuedSessionCount();
        uint32 connPeak = sWorld->GetMaxActiveSessionCount();
        std::string uptime = secsToTimeString(sWorld->GetUptime()).append(".");
        uint32 updateTime = sWorld->GetUpdateTime();
        uint32 avgUpdateTime = avgDiffTracker.getAverage();

        handler->PSendSysMessage("Project Balnazzar © 2017");
        handler->PSendSysMessage("%s", _FULLVERSION);
        if (!queuedSessionCount)
            handler->PSendSysMessage("Connected Players: %u. Online: %u.", activeSessionCount, playerCount);
        else
            handler->PSendSysMessage("Connected Players: %u. Online: %u. In Queue: %u.", activeSessionCount, playerCount, queuedSessionCount);
        //handler->PSendSysMessage("Connection peak: %u.", connPeak);
        handler->PSendSysMessage(LANG_UPTIME, uptime.c_str());
        handler->PSendSysMessage("Update: %ums, avg: %ums.", updateTime, avgUpdateTime);
        handler->PSendSysMessage("DEV wavg: %ums, nsmax: %ums, nsavg: %ums. LFG avg: %ums, max: %ums.", avgDiffTracker.getTimeWeightedAverage(), devDiffTracker.getMax(), devDiffTracker.getAverage(), lfgDiffTracker.getAverage(), lfgDiffTracker.getMax());

        //! Can't use sWorld->ShutdownMsg here in case of console command
        if (sWorld->IsShuttingDown())
            handler->PSendSysMessage(LANG_SHUTDOWN_TIMELEFT, secsToTimeString(sWorld->GetShutDownTimeLeft()).append(".").c_str());

        return true;
    }
    // Display the 'Message of the day' for the realm
    static bool HandleServerMotdCommand(ChatHandler* handler, char const* /*args*/)
    {
        handler->PSendSysMessage(LANG_MOTD_CURRENT, sWorld->GetMotd());
        return true;
    }

    static bool HandleServerShutDownCancelCommand(ChatHandler* handler, char const* /*args*/)
    {
        sWorld->ShutdownCancel();

        return true;
    }

    static bool HandleServerShutDownCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* timeStr = strtok((char*) args, " ");
        char* exitCodeStr = strtok(NULL, "");

        int32 time = atoi(timeStr);

        // Prevent interpret wrong arg value as 0 secs shutdown time
        if ((time == 0 && (timeStr[0] != '0' || timeStr[1] != '\0')) || time < 0)
            return false;

        if (exitCodeStr)
        {
            int32 exitCode = atoi(exitCodeStr);

            // Handle atoi() errors
            if (exitCode == 0 && (exitCodeStr[0] != '0' || exitCodeStr[1] != '\0'))
                return false;

            // Exit code should be in range of 0-125, 126-255 is used
            // in many shells for their own return codes and code > 255
            // is not supported in many others
            if (exitCode < 0 || exitCode > 125)
                return false;

            sObjectAccessor->SaveAllPlayers();
            sWorld->ShutdownServ(time, 0, exitCode);
        }
        else
        {
            sObjectAccessor->SaveAllPlayers();
            sWorld->ShutdownServ(time, 0, SHUTDOWN_EXIT_CODE);
        }

        return true;
    }

    static bool HandleServerRestartCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* timeStr = strtok((char*) args, " ");
        char* exitCodeStr = strtok(NULL, "");

        int32 time = atoi(timeStr);

        //  Prevent interpret wrong arg value as 0 secs shutdown time
        if ((time == 0 && (timeStr[0] != '0' || timeStr[1] != '\0')) || time < 0)
            return false;

        if (exitCodeStr)
        {
            int32 exitCode = atoi(exitCodeStr);

            // Handle atoi() errors
            if (exitCode == 0 && (exitCodeStr[0] != '0' || exitCodeStr[1] != '\0'))
                return false;

            // Exit code should be in range of 0-125, 126-255 is used
            // in many shells for their own return codes and code > 255
            // is not supported in many others
            if (exitCode < 0 || exitCode > 125)
                return false;

            sWorld->ShutdownServ(time, SHUTDOWN_MASK_RESTART, exitCode);
        }
        else
            sWorld->ShutdownServ(time, SHUTDOWN_MASK_RESTART, RESTART_EXIT_CODE);

            return true;
    }

    static bool HandleServerIdleRestartCommand(ChatHandler* /*handler*/, char const* args)
    {
        if (!*args)
            return false;

        char* timeStr = strtok((char*)args, " ");
        char* exitCodeStr = strtok(NULL, "");

        int32 time = atoi(timeStr);

        // Prevent interpret wrong arg value as 0 secs shutdown time
        if ((time == 0 && (timeStr[0] != '0' || timeStr[1] != '\0')) || time < 0)
            return false;

        if (exitCodeStr)
        {
            int32 exitCode = atoi(exitCodeStr);

            // Handle atoi() errors
            if (exitCode == 0 && (exitCodeStr[0] != '0' || exitCodeStr[1] != '\0'))
                return false;

            // Exit code should be in range of 0-125, 126-255 is used
            // in many shells for their own return codes and code > 255
            // is not supported in many others
            if (exitCode < 0 || exitCode > 125)
                return false;

            sObjectAccessor->SaveAllPlayers();
            sWorld->ShutdownServ(time, SHUTDOWN_MASK_RESTART | SHUTDOWN_MASK_IDLE, exitCode);
        }
        else
        {
            sObjectAccessor->SaveAllPlayers();
            sWorld->ShutdownServ(time, SHUTDOWN_MASK_RESTART | SHUTDOWN_MASK_IDLE, RESTART_EXIT_CODE);
        }
        return true;
    }

    static bool HandleServerIdleShutDownCommand(ChatHandler* /*handler*/, char const* args)
    {
        if (!*args)
            return false;

        char* timeStr = strtok((char*) args, " ");
        char* exitCodeStr = strtok(NULL, "");

        int32 time = atoi(timeStr);

        // Prevent interpret wrong arg value as 0 secs shutdown time
        if ((time == 0 && (timeStr[0] != '0' || timeStr[1] != '\0')) || time < 0)
            return false;

        if (exitCodeStr)
        {
            int32 exitCode = atoi(exitCodeStr);

            // Handle atoi() errors
            if (exitCode == 0 && (exitCodeStr[0] != '0' || exitCodeStr[1] != '\0'))
                return false;

            // Exit code should be in range of 0-125, 126-255 is used
            // in many shells for their own return codes and code > 255
            // is not supported in many others
            if (exitCode < 0 || exitCode > 125)
                return false;

            sWorld->ShutdownServ(time, SHUTDOWN_MASK_IDLE, exitCode);
        }
        else
            sWorld->ShutdownServ(time, SHUTDOWN_MASK_IDLE, SHUTDOWN_EXIT_CODE);
            return true;
    }

    // Exit the realm
    static bool HandleServerExitCommand(ChatHandler* handler, char const* /*args*/)
    {
        handler->SendSysMessage(LANG_COMMAND_EXIT);
        World::StopNow(SHUTDOWN_EXIT_CODE);
        return true;
    }

    // Define the 'Message of the day' for the realm
    static bool HandleServerSetMotdCommand(ChatHandler* handler, char const* args)
    {
        sWorld->SetMotd(args);
        handler->PSendSysMessage(LANG_MOTD_NEW, args);
        return true;
    }

    // Set whether we accept new clients
    static bool HandleServerSetClosedCommand(ChatHandler* handler, char const* args)
    {
        if (strncmp(args, "on", 3) == 0)
        {
            handler->SendSysMessage(LANG_WORLD_CLOSED);
            sWorld->SetClosed(true);
            return true;
        }
        else if (strncmp(args, "off", 4) == 0)
        {
            handler->SendSysMessage(LANG_WORLD_OPENED);
            sWorld->SetClosed(false);
            return true;
        }

        handler->SendSysMessage(LANG_USE_BOL);
        handler->SetSentErrorMessage(true);
        return false;
    }

    // Set the level of logging
    static bool HandleServerSetLogFileLevelCommand(ChatHandler* /*handler*/, char const* args)
    {
        if (!*args)
            return false;

        char* newLevel = strtok((char*)args, " ");
        if (!newLevel)
            return false;

        sLog->SetLogFileLevel(newLevel);
        return true;
    }

    // Set the level of logging
    static bool HandleServerSetLogLevelCommand(ChatHandler* /*handler*/, char const* args)
    {
        if (!*args)
            return false;

        char* newLevel = strtok((char*)args, " ");
        if (!newLevel)
            return false;

        sLog->SetLogLevel(newLevel);
        return true;
    }

    // set diff time record interval
    static bool HandleServerSetDiffTimeCommand(ChatHandler* /*handler*/, char const* args)
    {
        if (!*args)
            return false;

        char* newTimeStr = strtok((char*)args, " ");
        if (!newTimeStr)
            return false;

        int32 newTime = atoi(newTimeStr);
        if (newTime < 0)
            return false;

        sWorld->SetRecordDiffInterval(newTime);
        printf("Record diff every %u ms\n", newTime);

        return true;
    }

    // toggle sql driver query logging
    static bool HandleServerToggleQueryLogging(ChatHandler* handler, char const* /*args*/)
    {
        sLog->SetSQLDriverQueryLogging(!sLog->GetSQLDriverQueryLogging());

        if (sLog->GetSQLDriverQueryLogging())
            handler->PSendSysMessage(LANG_SQLDRIVER_QUERY_LOGGING_ENABLED);
        else
            handler->PSendSysMessage(LANG_SQLDRIVER_QUERY_LOGGING_DISABLED);
        return true;
    }
};

void AddSC_server_commandscript()
{
    new server_commandscript();
}
