/*
 * Copyright (C) 2005-2011 MaNGOS <http://www.getmangos.com/>
 *
 * Copyright (C) 2008-2011 Trinity <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/** \file
    \ingroup Trinityd
*/

#include <ace/Sig_Handler.h>

#include "Common.h"
#include "SystemConfig.h"
#include "SignalHandler.h"
#include "Proxy.h"
#include "ProxyRunnable.h"
#include "ClientSocket.h"
#include "NodeSocket.h"
#include "ControlSocket.h"
#include "Configuration/Config.h"
#include "Database/DatabaseEnv.h"
#include "Database/DatabaseWorkerPool.h"

#include "CliRunnable.h"
#include "Log.h"
#include "Master.h"
//#include "RARunnable.h"
//#include "TCSoap.h"
#include "Timer.h"
#include "Util.h"
#include "AuthSocket.h"
#include "RealmList.h"

#include "BigNumber.h"

#include "Network.h"

#ifdef _WIN32
#include "ServiceWin32.h"
extern int m_ServiceStatus;
#endif

/// Handle cored's termination signals
class CoredSignalHandler : public Trinity::SignalHandler
{
    public:
        virtual void HandleSignal(int SigNum)
        {
            switch (SigNum)
            {
                case SIGINT:
                    Logon::StopNow(RESTART_EXIT_CODE);
                    break;
                case SIGTERM:
                #ifdef _WIN32
                case SIGBREAK:
                    if (m_ServiceStatus != 1)
                #endif /* _WIN32 */
                    Logon::StopNow(SHUTDOWN_EXIT_CODE);
                    break;
            }
        }
};

class FreezeDetectorRunnable : public ACE_Based::Runnable
{
public:
    FreezeDetectorRunnable() { _delaytime = 0; }
    uint32 m_loops, m_lastchange;
    uint32 w_loops, w_lastchange;
    uint32 _delaytime;
    void SetDelayTime(uint32 t) { _delaytime = t; }
    void run(void)
    {
        if (!_delaytime)
            return;
        sLog->outString("Starting up anti-freeze thread (%u seconds max stuck time)...", _delaytime/1000);
        m_loops = 0;
        w_loops = 0;
        m_lastchange = 0;
        w_lastchange = 0;
        while (!Logon::IsStopped())
        {
            ACE_Based::Thread::Sleep(1000);
            uint32 curtime = getMSTime();
            // normal work
            if (w_loops != Logon::m_logonLoopCounter)
            {
                w_lastchange = curtime;
                w_loops = Logon::m_logonLoopCounter;
            }
            // possible freeze
            else if (getMSTimeDiff(w_lastchange,curtime) > _delaytime)
            {
                sLog->outError("World Thread hangs, kicking out server!");
                *((uint32 volatile*)NULL) = 0;                       // bang crash
            }
        }
        sLog->outString("Anti-freeze thread exiting without problems.");
    }
};

Master::Master()
{
}

Master::~Master()
{
}

/// Main function
int Master::Run()
{
    BigNumber seed1;
    seed1.SetRand(16 * 8);

    sLog->outString("      ________________________________________________________________________________________");
    sLog->outString("      |                                                                                       |");
    sLog->outString("      |                       ,,                                                              |");
    sLog->outString("      | `7MM\"\"\"Yp,          `7MM                       MMM\"\"\"AMV MMM\"\"\"AMV                    |");
    sLog->outString("      |   MM    Yb            MM                       M'   AMV  M'   AMV                     |");
    sLog->outString("      |   MM    dP  ,6\"Yb.    MM  `7MMpMMMb.   ,6\"Yb.  '   AMV   '   AMV    ,6\"Yb.  `7Mb,od8  |");
    sLog->outString("      |   MM\"\"\"bg. 8)   MM    MM    MM    MM  8)   MM     AMV       AMV    8)   MM    MM' \"'  |");
    sLog->outString("      |   MM    `Y  ,pm9MM    MM    MM    MM   ,pm9MM    AMV   ,   AMV   ,  ,pm9MM    MM      |");
    sLog->outString("      |   MM    ,9 8M   MM    MM    MM    MM  8M   MM   AMV   ,M  AMV   ,M 8M   MM    MM      |");
    sLog->outString("      | .JMMmmmd9  `Moo9^Yo..JMML..JMML  JMML.`Moo9^Yo.AMVmmmmMM AMVmmmmMM `Moo9^Yo..JMML.    |");
    sLog->outString("      |                                                                                       |");
    sLog->outString("      |_______________________________________________________________________________________|");
    sLog->outString("                                   Copyright (c) Project Balnazzar 2016-2018       ");
    sLog->outString("                                           www.websitehere.com                  ");
    sLog->outString("                                 _______________________________________      ");
    sLog->outString("      %s", _FULLVERSION);
    sLog->outString("");
    sLog->outString("");
    sLog->outString("");
    sLog->outString("Starting \"Proxy DataStore Server\"...");
    //sLog->outDetail("%s (Library: %s)", OPENSSL_VERSION_TEXT, SSLeay_version(SSLEAY_VERSION));

#ifdef USE_SFMT_FOR_RNG
    sLog->outString("\n");
    sLog->outString("SFMT has been enabled as the random number generator, if worldserver");
    sLog->outString("freezes or crashes randomly, first, try disabling SFMT in CMAKE configuration");
    sLog->outString("\n");
#endif //USE_SFMT_FOR_RNG

    /// worldd PID file creation
    std::string pidfile = sConfigMgr->GetStringDefault("PidFile", "");
    if (!pidfile.empty())
    {
        uint32 pid = CreatePIDFile(pidfile);
        if (!pid)
        {
            sLog->outError("Cannot create PID file %s.\n", pidfile.c_str());
            return 1;
        }

        sLog->outString("Daemon PID: %u\n", pid);
    }

    ///- Start the databases
    if (!_StartDB())
        return 1;

    // set server offline (not connectable)
    LoginDatabase.DirectPExecute("UPDATE realmlist SET flag = %u WHERE id = '%d'", REALM_FLAG_OFFLINE, realmID);

    sLogon->LoadConfigSettings();
    sLogon->SetInitialLogonSettings();

    ///- Initialize the signal handlers
    CoredSignalHandler SignalINT, SignalTERM;
    #ifdef _WIN32
    CoredSignalHandler SignalBREAK;
    #endif /* _WIN32 */

    ///- Register core's signal handlers
    ACE_Sig_Handler Handler;
    Handler.register_handler(SIGINT, &SignalINT);
    Handler.register_handler(SIGTERM, &SignalTERM);
    #ifdef _WIN32
    Handler.register_handler(SIGBREAK, &SignalBREAK);
    #endif /* _WIN32 */

    ///- Launch WorldRunnable thread
    ACE_Based::Thread world_thread(new LogonRunnable);
    world_thread.setPriority(ACE_Based::Highest);

    ACE_Based::Thread* cliThread = NULL;

#ifdef _WIN32
    if (sConfigMgr->GetBoolDefault("Console.Enable", true) && (m_ServiceStatus == -1)/* need disable console in service mode*/)
#else
    if (sConfigMgr->GetBoolDefault("Console.Enable", true))
#endif
    {
        ///- Launch CliRunnable thread
        cliThread = new ACE_Based::Thread(new CliRunnable);
    }

    //ACE_Based::Thread rar_thread(new RARunnable);

    ///- Handle affinity for multiple processors and process priority on Windows
    #ifdef _WIN32
    {
        HANDLE hProcess = GetCurrentProcess();

        uint32 Aff = sConfigMgr->GetIntDefault("UseProcessors", 0);
        if (Aff > 0)
        {
            ULONG_PTR appAff;
            ULONG_PTR sysAff;

            if (GetProcessAffinityMask(hProcess, &appAff, &sysAff))
            {
                ULONG_PTR curAff = Aff & appAff;            // remove non accessible processors

                if (!curAff)
                {
                    sLog->outError("Processors marked in UseProcessors bitmask (hex) %x not accessible for Trinityd. Accessible processors bitmask (hex): %x", Aff, appAff);
                }
                else
                {
                    if (SetProcessAffinityMask(hProcess,curAff))
                        sLog->outString("Using processors (bitmask, hex): %x", curAff);
                    else
                        sLog->outError("Can't set used processors (hex): %x", curAff);
                }
            }
            sLog->outString();
        }

        bool Prio = sConfigMgr->GetBoolDefault("ProcessPriority", false);

        //if (Prio && (m_ServiceStatus == -1)  /* need set to default process priority class in service mode*/)
        if (Prio)
        {
            if (SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS))
                sLog->outString("ProxyDataServer process priority class set to HIGH");
            else
                sLog->outError("Can't set ProxyDataServer process priority class.");
            sLog->outString("");
        }
    }
    #endif
    //Start soap serving thread
    ACE_Based::Thread* soap_thread = NULL;

    ///- Start up freeze catcher thread
    if (uint32 freeze_delay = sConfigMgr->GetIntDefault("MaxCoreStuckTime", 0))
    {
        FreezeDetectorRunnable *fdr = new FreezeDetectorRunnable();
        fdr->SetDelayTime(freeze_delay * 1000);
        ACE_Based::Thread freeze_thread(fdr);
        freeze_thread.setPriority(ACE_Based::Highest);
    }

    ///- Launch the world listener socket
    uint16 wsport = sLogon->getIntConfig(CONFIG_PORT_LOGON);
    std::string bind_ip = sConfigMgr->GetStringDefault("BindIP", "0.0.0.0");

    if (sNetworkMgr->open() < 0)
    {
        sLog->outError("Failed to start network");
        Logon::StopNow(ERROR_EXIT_CODE);
    }

    if (sControlSocketConnector->open() < 0)
    {
        sLog->outError("Failed to start network (Control)");
        Logon::StopNow(ERROR_EXIT_CODE);
    }

    sRoutingHelper->SetupControler();

    if (sNodeSocketConnector->open() < 0)
    {
        sLog->outError("Failed to start network (Node)");
        Logon::StopNow(ERROR_EXIT_CODE);
    }

    if (sClientSocketAcceptor->open(wsport, bind_ip.c_str()) < 0)
    {
        sLog->outError("Failed to start network (Client)");
        Logon::StopNow(ERROR_EXIT_CODE);
    }

    // set server online (allow connecting now)
    LoginDatabase.DirectPExecute("UPDATE realmlist SET flag = %u, population = 0 WHERE id = '%u'", REALM_FLAG_NONE, realmID);

    sNetworkMgr->wait();

    // set server offline
    LoginDatabase.DirectPExecute("UPDATE realmlist SET flag = %u WHERE id = '%d'", REALM_FLAG_OFFLINE, realmID);

    // when the main thread closes the singletons get unloaded
    // since worldrunnable uses them, it will crash if unloaded after master
    world_thread.wait();
    //rar_thread.wait();

    ///- Clean database before leaving
    clearOnlineAccounts();

    _StopDB();

    sLog->outString("Halting process...");

    if (cliThread)
    {
        #ifdef _WIN32

        // this only way to terminate CLI thread exist at Win32 (alt. way exist only in Windows Vista API)
        //_exit(1);
        // send keyboard input to safely unblock the CLI thread
        INPUT_RECORD b[5];
        HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
        b[0].EventType = KEY_EVENT;
        b[0].Event.KeyEvent.bKeyDown = TRUE;
        b[0].Event.KeyEvent.uChar.AsciiChar = 'X';
        b[0].Event.KeyEvent.wVirtualKeyCode = 'X';
        b[0].Event.KeyEvent.wRepeatCount = 1;

        b[1].EventType = KEY_EVENT;
        b[1].Event.KeyEvent.bKeyDown = FALSE;
        b[1].Event.KeyEvent.uChar.AsciiChar = 'X';
        b[1].Event.KeyEvent.wVirtualKeyCode = 'X';
        b[1].Event.KeyEvent.wRepeatCount = 1;

        b[2].EventType = KEY_EVENT;
        b[2].Event.KeyEvent.bKeyDown = TRUE;
        b[2].Event.KeyEvent.dwControlKeyState = 0;
        b[2].Event.KeyEvent.uChar.AsciiChar = '\r';
        b[2].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
        b[2].Event.KeyEvent.wRepeatCount = 1;
        b[2].Event.KeyEvent.wVirtualScanCode = 0x1c;

        b[3].EventType = KEY_EVENT;
        b[3].Event.KeyEvent.bKeyDown = FALSE;
        b[3].Event.KeyEvent.dwControlKeyState = 0;
        b[3].Event.KeyEvent.uChar.AsciiChar = '\r';
        b[3].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
        b[3].Event.KeyEvent.wVirtualScanCode = 0x1c;
        b[3].Event.KeyEvent.wRepeatCount = 1;
        DWORD numb;
        WriteConsoleInput(hStdIn, b, 4, &numb);

        cliThread->wait();

        #else

        cliThread->destroy();

        #endif

        delete cliThread;
    }

    // Exit the process with specified return value
    return Logon::GetExitCode();
}

/// Initialize connection to the databases
bool Master::_StartDB()
{
    MySQL::Library_Init();

    sLog->SetLogDB(false);
    std::string dbstring;
    uint8 async_threads, synch_threads;

    dbstring = sConfigMgr->GetStringDefault("WorldDatabaseInfo", "");
    if (dbstring.empty())
    {
        sLog->outError("World database not specified in configuration file");
        return false;
    }

    async_threads = sConfigMgr->GetIntDefault("WorldDatabase.WorkerThreads", 1);
    if (async_threads < 1 || async_threads > 32)
    {
        sLog->outError("World database: invalid number of worker threads specified. "
            "Please pick a value between 1 and 32.");
        return false;
    }

    synch_threads = sConfigMgr->GetIntDefault("WorldDatabase.SynchThreads", 1);
    ///- Initialise the world database
    if (!WorldDatabase.Open(dbstring, async_threads, synch_threads))
    {
        sLog->outError("Cannot connect to world database %s", dbstring.c_str());
        return false;
    }

    ///- Get character database info from configuration file
    dbstring = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
    if (dbstring.empty())
    {
        sLog->outError("Character database not specified in configuration file");
        return false;
    }

    async_threads = sConfigMgr->GetIntDefault("CharacterDatabase.WorkerThreads", 1);
    if (async_threads < 1 || async_threads > 32)
    {
        sLog->outError("Character database: invalid number of worker threads specified. "
            "Please pick a value between 1 and 32.");
        return false;
    }

    synch_threads = sConfigMgr->GetIntDefault("CharacterDatabase.SynchThreads", 2);

    ///- Initialise the Character database
    if (!CharacterDatabase.Open(dbstring, async_threads, synch_threads))
    {
        sLog->outError("Cannot connect to Character database %s", dbstring.c_str());
        return false;
    }

    ///- Get login database info from configuration file
    dbstring = sConfigMgr->GetStringDefault("LoginDatabaseInfo", "");
    if (dbstring.empty())
    {
        sLog->outError("Login database not specified in configuration file");
        return false;
    }

    async_threads = sConfigMgr->GetIntDefault("LoginDatabase.WorkerThreads", 1);
    if (async_threads < 1 || async_threads > 32)
    {
        sLog->outError("Login database: invalid number of worker threads specified. "
            "Please pick a value between 1 and 32.");
        return false;
    }

    synch_threads = sConfigMgr->GetIntDefault("LoginDatabase.SynchThreads", 1);
    ///- Initialise the login database
    if (!LoginDatabase.Open(dbstring, async_threads, synch_threads))
    {
        sLog->outError("Cannot connect to login database %s", dbstring.c_str());
        return false;
    }

    ///- Get logon database info from configuration file
    dbstring = sConfigMgr->GetStringDefault("LogonDatabaseInfo", "");
    if (dbstring.empty())
    {
        sLog->outError("Logon database not specified in configuration file");
        return false;
    }

    async_threads = sConfigMgr->GetIntDefault("LogonDatabase.WorkerThreads", 1);
    if (async_threads < 1 || async_threads > 32)
    {
        sLog->outError("Logon database: invalid number of worker threads specified. "
            "Please pick a value between 1 and 32.");
        return false;
    }

    synch_threads = sConfigMgr->GetIntDefault("LogonDatabase.SynchThreads", 1);
    ///- Initialise the logon database
    if (!LogonDatabase.Open(dbstring, async_threads, synch_threads))
    {
        sLog->outError("Cannot connect to logon database %s", dbstring.c_str());
        return false;
    }

    ///- Get the realm Id from the configuration file
    realmID = sConfigMgr->GetIntDefault("RealmID", 0);
    if (!realmID)
    {
        sLog->outError("Realm ID not defined in configuration file");
        return false;
    }
    sLog->outString("Realm running as realm ID %d", realmID);

    ///- Initialize the DB logging system
    //sLog->SetLogDBLater(sConfigMgr->GetBoolDefault("EnableLogDB", false)); // set var to enable DB logging once startup finished.
    sLog->SetLogDB(false);
    sLog->SetRealmID(realmID);

    ///- Clean the database before starting
    clearOnlineAccounts(); 
    return true;
}

void Master::_StopDB()
{
    CharacterDatabase.Close();
    WorldDatabase.Close();
    LoginDatabase.Close();

    MySQL::Library_End();
}

/// Clear 'online' status for all accounts with characters in this realm
void Master::clearOnlineAccounts()
{
    // Cleanup online status for characters hosted at current realm
    /// \todo Only accounts with characters logged on *this* realm should have online status reset. Move the online column from 'account' to 'realmcharacters'?
    LoginDatabase.DirectPExecute(
        "UPDATE account SET online = 0 WHERE online > 0 "
        "AND id IN (SELECT acctid FROM realmcharacters WHERE realmid = '%d')", realmID);

    CharacterDatabase.DirectExecute("UPDATE characters SET online = 0 WHERE online <> 0");

    // Battleground instance ids reset at server restart
    //CharacterDatabase.DirectExecute(CharacterDatabase.GetPreparedStatement(CHAR_RESET_PLAYERS_BGDATA));
}
