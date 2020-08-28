#include "ClientSocket.h"
#include "NodeSocket.h"
#include "RoutingHelper.h"

#include "ObjectMgr.h"
#include "Player.h"

#include "ClientSession.h"
#include "Opcodes.h"
#include "WorldPacket.h"
#include "ByteBuffer.h"
#include "Common.h"
#include "Log.h"
#include "zlib.h"
#include "SocialMgr.h"
#include "WardenWin.h"
#include "WardenMac.h"

#include "ClusterDefines.h"

#include "ControlSessionMgr.h"

ClientSession::ClientSession(uint32 AccountID, ClientSocket *sock, AccountTypes sec, uint8 expansion, time_t mute_time, LocaleConstant locale, uint32 recruiter, bool isRecruiter) :
_accountId(AccountID), _ClientSocket(sock), _security(sec), _expansion(expansion), _muteTime(mute_time), _sessionDbcLocale(sLogon->GetAvailableDbcLocale(locale)),
_sessionDbLocaleIndex(locale), _nodeId(0), _player(NULL), m_loginDelay(0), m_DoSStrikes(0), _antiSpam(0), _referenceCount(1)
{
    //Default settings
    _NodeSocket = NULL;
    _sessionflags = 0;
    _authed = true;
    _callback_timer = 5000;
    _clientaddress = "";
    if (sock)
    {
        _clientaddress = sock->GetRemoteAddress();
        sock->add_reference();
        LoginDatabase.PExecute("UPDATE account SET online = 1 WHERE id = %u;", GetAccountId());
    }
    //sLog->outString("[1] _nodeId: %u", _nodeId);
    _nodeId = sRoutingHelper->ConnectToMaster();
    if (_nodeId == 0)
        _reconnect_timer = 2000000;
    else
        _reconnect_timer = 100000; //Set the reconnect Timer
    //sLog->outString("[2] _nodeId: %u", _nodeId);
    SetFlag(FLAG_ACCOUNT_RECONNECT); //Die Verbindung baut nun der Reconnect auf
}

ClientSession::~ClientSession()
{
    RemoveFlag(FLAG_ACCOUNT_RECONNECT);

    ///- Remove the Player, if any
    if (_player)
        PlayerLogout();

    if (_ClientSocket)
    {
        if (!_ClientSocket->IsClosing())
            _ClientSocket->close();
        _ClientSocket->remove_reference();
        _ClientSocket = NULL;
    }

    if (_NodeSocket)
    {
        if (!_NodeSocket->IsClosing())
            _NodeSocket->close();
        _NodeSocket->remove_reference();
        _NodeSocket = NULL;
    }

    LoginDatabase.PExecute("UPDATE account SET online = 0 WHERE id = %u;", GetAccountId());
}

ACE_Event_Handler::Reference_Count ClientSession::add_reference()
{
    return ++_referenceCount;
}

ACE_Event_Handler::Reference_Count ClientSession::remove_reference()
{
    ACE_Event_Handler::Reference_Count result = --_referenceCount;

    if (result == 0)
        delete this;

    return result;
}

/*********************************************************/
/***                  ACCOUNT SETTINGS                 ***/
/*********************************************************/
void ClientSession::LoadGlobalAccountData()
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ACCOUNT_DATA);
    stmt->setUInt32(0, GetAccountId());
    LoadAccountData(CharacterDatabase.Query(stmt), GLOBAL_CACHE_MASK);
}

void ClientSession::LoadAccountData(PreparedQueryResult result, uint32 mask)
{
    for (uint32 i = 0; i < NUM_ACCOUNT_DATA_TYPES; ++i)
        if (mask & (1 << i))
            _accountData[i] = AccountData();

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        uint32 type = fields[0].GetUInt8();
        if (type >= NUM_ACCOUNT_DATA_TYPES)
        {
            sLog->outError("Table `%s` have invalid account data type (%u), ignore.", mask == GLOBAL_CACHE_MASK ? "account_data" : "character_account_data", type);
            continue;
        }

        if ((mask & (1 << type)) == 0)
        {
            sLog->outError("Table `%s` have non appropriate for table  account data type (%u), ignore.", mask == GLOBAL_CACHE_MASK ? "account_data" : "character_account_data", type);
            continue;
        }

        _accountData[type].Time = time_t(fields[1].GetUInt32());
        _accountData[type].Data = fields[2].GetString();
    }
    while (result->NextRow());
}

void ClientSession::SendAccountDataTimes(uint32 mask)
{
    WorldPacket data(SMSG_ACCOUNT_DATA_TIMES, 4 + 1 + 4 + 8 * 4); // changed in WotLK
    data << uint32(time(NULL));                             // unix time of something
    data << uint8(1);
    data << uint32(mask);                                   // type mask
    for (uint32 i = 0; i < NUM_ACCOUNT_DATA_TYPES; ++i)
        if (mask & (1 << i))
            data << uint32(GetAccountData(AccountDataType(i))->Time);// also unix time
    SendPacket(&data);
}

void ClientSession::SetAccountData(AccountDataType type, time_t tm, std::string data)
{
    uint32 id = 0;
    uint32 index = 0;
    if ((1 << type) & GLOBAL_CACHE_MASK)
    {
        id = GetAccountId();
        index = CHAR_REP_ACCOUNT_DATA;
    }
    else
    {
        // _player can be NULL and packet received after logout but m_GUID still store correct guid
        if (!_GUIDLow)
            return;

        id = _GUIDLow;
        index = CHAR_REP_PLAYER_ACCOUNT_DATA;
    }

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(index);
    stmt->setUInt32(0, id);
    stmt->setUInt8 (1, type);
    stmt->setUInt32(2, uint32(tm));
    stmt->setString(3, data);
    CharacterDatabase.Execute(stmt);

    _accountData[type].Time = tm;
    _accountData[type].Data = data;
}


/*********************************************************/
/***                     ADDON                         ***/
/*********************************************************/
void ClientSession::ReadAddonsInfo(WorldPacket &data)
{
    if (data.rpos() + 4 > data.size())
        return;
    uint32 size;
    data >> size;

    if (!size)
        return;

    if (size > 0xFFFFF)
    {
        sLog->outError("WorldSession::ReadAddonsInfo addon info too big, size %u", size);
        return;
    }

    uLongf uSize = size;

    uint32 pos = data.rpos();

    ByteBuffer addonInfo;
    addonInfo.resize(size);

    if (uncompress(const_cast<uint8 *>(addonInfo.contents()), &uSize, const_cast<uint8 *>(data.contents() + pos), data.size() - pos) == Z_OK)
    {
        uint32 addonsCount;
        addonInfo >> addonsCount;                         // addons count

        for (uint32 i = 0; i < addonsCount; ++i)
        {
            std::string addonName;
            uint8 enabled;
            uint32 crc, unk1;

            // check next addon data format correctness
            if (addonInfo.rpos() + 1 > addonInfo.size())
                return;

            addonInfo >> addonName;

            addonInfo >> enabled >> crc >> unk1;

            sLog->outDetail("ADDON: Name: %s, Enabled: 0x%x, CRC: 0x%x, Unknown2: 0x%x", addonName.c_str(), enabled, crc, unk1);

            AddonInfo addon(addonName, enabled, crc, 2, true);

            SavedAddon const* savedAddon = sAddonMgr->GetAddonInfo(addonName);
            if (savedAddon)
            {
                bool match = true;

                if (addon.CRC != savedAddon->CRC)
                    match = false;

                if (!match)
                    sLog->outDetail("ADDON: %s was known, but didn't match known CRC (0x%x)!", addon.Name.c_str(), savedAddon->CRC);
                else
                    sLog->outDetail("ADDON: %s was known, CRC is correct (0x%x)", addon.Name.c_str(), savedAddon->CRC);
            }
            else
            {
                sAddonMgr->SaveAddon(addon);

                sLog->outDetail("ADDON: %s (0x%x) was not known, saving...", addon.Name.c_str(), addon.CRC);
            }

            // TODO: Find out when to not use CRC/pubkey, and other possible states.
            _addonsList.push_back(addon);
        }

        uint32 currentTime;
        addonInfo >> currentTime;
        //sLog->outString("ADDON: CurrentTime: %u", currentTime);

        if (addonInfo.rpos() != addonInfo.size())
            sLog->outString("packet under-read!");
    }
    else
        sLog->outError("Addon packet uncompress error!");
}

void ClientSession::SendAddonsInfo()
{
    uint8 addonPublicKey[256] =
    {
        0xC3, 0x5B, 0x50, 0x84, 0xB9, 0x3E, 0x32, 0x42, 0x8C, 0xD0, 0xC7, 0x48, 0xFA, 0x0E, 0x5D, 0x54,
        0x5A, 0xA3, 0x0E, 0x14, 0xBA, 0x9E, 0x0D, 0xB9, 0x5D, 0x8B, 0xEE, 0xB6, 0x84, 0x93, 0x45, 0x75,
        0xFF, 0x31, 0xFE, 0x2F, 0x64, 0x3F, 0x3D, 0x6D, 0x07, 0xD9, 0x44, 0x9B, 0x40, 0x85, 0x59, 0x34,
        0x4E, 0x10, 0xE1, 0xE7, 0x43, 0x69, 0xEF, 0x7C, 0x16, 0xFC, 0xB4, 0xED, 0x1B, 0x95, 0x28, 0xA8,
        0x23, 0x76, 0x51, 0x31, 0x57, 0x30, 0x2B, 0x79, 0x08, 0x50, 0x10, 0x1C, 0x4A, 0x1A, 0x2C, 0xC8,
        0x8B, 0x8F, 0x05, 0x2D, 0x22, 0x3D, 0xDB, 0x5A, 0x24, 0x7A, 0x0F, 0x13, 0x50, 0x37, 0x8F, 0x5A,
        0xCC, 0x9E, 0x04, 0x44, 0x0E, 0x87, 0x01, 0xD4, 0xA3, 0x15, 0x94, 0x16, 0x34, 0xC6, 0xC2, 0xC3,
        0xFB, 0x49, 0xFE, 0xE1, 0xF9, 0xDA, 0x8C, 0x50, 0x3C, 0xBE, 0x2C, 0xBB, 0x57, 0xED, 0x46, 0xB9,
        0xAD, 0x8B, 0xC6, 0xDF, 0x0E, 0xD6, 0x0F, 0xBE, 0x80, 0xB3, 0x8B, 0x1E, 0x77, 0xCF, 0xAD, 0x22,
        0xCF, 0xB7, 0x4B, 0xCF, 0xFB, 0xF0, 0x6B, 0x11, 0x45, 0x2D, 0x7A, 0x81, 0x18, 0xF2, 0x92, 0x7E,
        0x98, 0x56, 0x5D, 0x5E, 0x69, 0x72, 0x0A, 0x0D, 0x03, 0x0A, 0x85, 0xA2, 0x85, 0x9C, 0xCB, 0xFB,
        0x56, 0x6E, 0x8F, 0x44, 0xBB, 0x8F, 0x02, 0x22, 0x68, 0x63, 0x97, 0xBC, 0x85, 0xBA, 0xA8, 0xF7,
        0xB5, 0x40, 0x68, 0x3C, 0x77, 0x86, 0x6F, 0x4B, 0xD7, 0x88, 0xCA, 0x8A, 0xD7, 0xCE, 0x36, 0xF0,
        0x45, 0x6E, 0xD5, 0x64, 0x79, 0x0F, 0x17, 0xFC, 0x64, 0xDD, 0x10, 0x6F, 0xF3, 0xF5, 0xE0, 0xA6,
        0xC3, 0xFB, 0x1B, 0x8C, 0x29, 0xEF, 0x8E, 0xE5, 0x34, 0xCB, 0xD1, 0x2A, 0xCE, 0x79, 0xC3, 0x9A,
        0x0D, 0x36, 0xEA, 0x01, 0xE0, 0xAA, 0x91, 0x20, 0x54, 0xF0, 0x72, 0xD8, 0x1E, 0xC7, 0x89, 0xD2
    };

    WorldPacket data(SMSG_ADDON_INFO, 4);

    for (AddonsList::iterator itr = _addonsList.begin(); itr != _addonsList.end(); ++itr)
    {
        data << uint8(itr->State);

        uint8 crcpub = itr->UsePublicKeyOrCRC;
        data << uint8(crcpub);
        if (crcpub)
        {
            uint8 usepk = (itr->CRC != STANDARD_ADDON_CRC); // If addon is Standard addon CRC
            data << uint8(usepk);
            if (usepk)                                      // if CRC is wrong, add public key (client need it)
            {
                sLog->outDetail("ADDON: CRC (0x%x) for addon %s is wrong (does not match expected 0x%x), sending pubkey",
                    itr->CRC, itr->Name.c_str(), STANDARD_ADDON_CRC);

                data.append(addonPublicKey, sizeof(addonPublicKey));
            }

            data << uint32(0);                              // TODO: Find out the meaning of this.
        }

        uint8 unk3 = 0;                                     // 0 is sent here
        data << uint8(unk3);
        if (unk3)
        {
            // String, length 256 (null terminated)
            data << uint8(0);
        }
    }

    _addonsList.clear();

    data << uint32(0); // count for an unknown for loop

    SendPacket(&data);
}


/*********************************************************/
/***                  I/O HANDLING                     ***/
/*********************************************************/

void ClientSession::HandlePacket(WorldPacket *packet, uint16 opcode, uint8 procedure)
{
    if ((procedure & PACKET_SEND_NODE) == PACKET_SEND_NODE)
        SendPacketToNode(packet);

    if ((procedure & PACKET_SEND_CLIENT) == PACKET_SEND_CLIENT)
        SendPacket(packet);

    if ((procedure & PACKET_PROCESS) == PACKET_PROCESS)
    {
        try
        {
            (this->*opcodeTable[packet->GetOpcode()].handler)(*packet);
        }
        catch (ByteBufferException &)
        {
            if (sLog->IsOutDebug())
            {
                sLog->outString("Dumping error causing packet:");
                packet->hexlike();
            }
        }
    }
}

void ClientSession::UpdateSingleNodeIO(uint32 diff)
{
    WorldPacket* packet = NULL;

    while (_NodeSocket && _NodeSocket->GetNextSinglePacket(packet))
    {
        if (packet->GetOpcode() < NUM_MSG_TYPES)
        {
            OpcodeHandler& opHandle = opcodeTable[packet->GetOpcode()];

            uint8 procedure = 0;
            switch (opHandle.from_node)
            {
                case PACKET_STATE_PASS:
                    procedure |= PACKET_SEND_CLIENT;
                    break;
                case PACKET_STATE_BOTH:
                    procedure |= PACKET_SEND_CLIENT;
                case PACKET_STATE_LOGON:
                    procedure |= PACKET_PROCESS;
                    break;
            }

            HandlePacket(packet, packet->GetOpcode(), procedure);
        }

        delete packet;
    }
}

void ClientSession::UpdateSingleClientIO(uint32 diff)
{
    WorldPacket* packet = NULL;
    uint32 packet_count = 0;

//#define MAX_COUNT 25

    while (/*packet_count++ < MAX_COUNT && */_ClientSocket && _ClientSocket->GetNextSinglePacket(packet))
    {
        if (packet->GetOpcode() < NUM_MSG_TYPES)
        {
            OpcodeHandler& opHandle = opcodeTable[packet->GetOpcode()];

            if ((opHandle.status == STATUS_AUTHED && _authed) ||
                (opHandle.status == STATUS_LOGGEDIN && _player) ||
                (opHandle.status == STATUS_TRANSFER && _player))
            {
                uint8 procedure = 0;
                switch (opHandle.to_node)
                {
                    case PACKET_STATE_NODE:
                        procedure |= PACKET_SEND_NODE;
                        break;
                    case PACKET_STATE_BOTH:
                        procedure |= PACKET_SEND_NODE;
                    case PACKET_STATE_LOGON:
                        procedure |= PACKET_PROCESS;
                        break;
                }

                HandlePacket(packet, packet->GetOpcode(), procedure);
            }
        }

        delete packet;
    }

//#undef MAX_COUNT
}

bool ClientSession::UpdateSingle(uint32 diff)
{
    if (HasFlag(FLAG_ACCOUNT_KICKED))
        return false;

    UpdateSingleNodeIO(diff);

    if (!_ClientSocket || _ClientSocket->IsClosing())
        return false;

    UpdateSingleClientIO(diff);
    return true;
}

void ClientSession::UpdateMultiNodeIO(uint32 diff)
{
    WorldPacket* packet = NULL;

    while (_NodeSocket && _NodeSocket->GetNextMultiPacket(packet))
    {
        //sLog->outString("SESSION_NODE: Received opcode 0x%.4X (%s)", packet->GetOpcode(), LookupOpcodeName(packet->GetOpcode()));

        if (packet->GetOpcode() < NUM_MSG_TYPES)
        {
            OpcodeHandler& opHandle = opcodeTable[packet->GetOpcode()];

            uint8 procedure = 0;
            switch (opHandle.from_node)
            {
                case PACKET_STATE_PASS:
                    procedure |= PACKET_SEND_CLIENT;
                    break;
                case PACKET_STATE_BOTH:
                    procedure |= PACKET_SEND_CLIENT;
                case PACKET_STATE_LOGON:
                    procedure |= PACKET_PROCESS;
                    break;
            }

            HandlePacket(packet, packet->GetOpcode(), procedure);
        }

        delete packet;
    }
}

void ClientSession::UpdateMultiClientIO(uint32 diff)
{
    WorldPacket* packet;
    //uint32 packet_count = 0;
    //sLog->outString("PACKETS SENT");
//#define MAX_COUNT 1000

    while (/*++packet_count < MAX_COUNT && */_ClientSocket && _ClientSocket->GetNextMultiPacket(packet))
    {
        //sLog->outString("SESSION_CLIENT: Received opcode 0x%.4X (%s)", packet->GetOpcode(), LookupOpcodeName(packet->GetOpcode()));

        if (packet->GetOpcode() < NUM_MSG_TYPES)
        {
            OpcodeHandler& opHandle = opcodeTable[packet->GetOpcode()];

            if ((opHandle.status == STATUS_AUTHED && _authed) ||
                (opHandle.status == STATUS_LOGGEDIN && _player) ||
                (opHandle.status == STATUS_TRANSFER && _player))
            {
                uint8 procedure = 0;
                switch (opHandle.to_node)
                {
                    case PACKET_STATE_NODE:
                        procedure |= PACKET_SEND_NODE;
                        break;
                    case PACKET_STATE_BOTH:
                        procedure |= PACKET_SEND_NODE;
                    case PACKET_STATE_LOGON:
                        procedure |= PACKET_PROCESS;
                        break;
                }

                HandlePacket(packet, packet->GetOpcode(), procedure);
            }
        }

        delete packet;
    }
//#undef MAX_COUNT
}

bool ClientSession::UpdateMulti(uint32 diff)
{
    if (HasFlag(FLAG_ACCOUNT_KICKED))
        return false; 

    if (_ClientSocket && _ClientSocket->GetLastActionTime() > (time(NULL) + sLogon->getIntConfig(CONFIG_SOCKET_TIMEOUTTIME)))
        KickPlayer();

    if (_callback_timer <= diff)
    {
        _callback_timer = 50000;
        ProcessQueryCallbacks();
    }
    else
        _callback_timer -= diff;

    if (HasFlag(FLAG_ACCOUNT_RECONNECT))
    {
       if (_reconnect_timer > diff)
           _reconnect_timer -= diff;
       else
       {
            if (_nodeId == 0) {
                _nodeId = sRoutingHelper->ConnectToMaster();
                sLog->outString("Connection to master node (%u) failed, trying to reconnect...", _nodeId);
            } 
            else if (sRoutingHelper->CanEnterNodeID(_nodeId))
            {
                if (NodeSocket* socket = sNodeSocketConnector->OpenNodeConnection(_nodeId))
                {
                    sLog->outString("Node(%u) connected successfully.", _nodeId);
                    socket->add_reference();
                    _NodeSocket = socket;

                    RemoveFlag(FLAG_ACCOUNT_RECONNECT);
                    SetFlag(FLAG_ACCOUNT_WAIT_FOR_OK);
                    SetStunned(false);
                }
            }

            _reconnect_timer = 5000000;
       }
    }

    UpdateMultiNodeIO(diff);

    if (_NodeSocket && _NodeSocket->IsClosing())
    {
        _NodeSocket->remove_reference();
        _NodeSocket = NULL;

        SetStunned(true);

        if (GetPlayer())
            SetFlag(FLAG_FORCE_TELEPORT);

        SetFlag(FLAG_ACCOUNT_RECONNECT);
    }

    UpdateMultiClientIO(diff);
    m_Warden->Update();

    if (_ClientSocket && _ClientSocket->IsClosing())
    {
        _ClientSocket->remove_reference();
        _ClientSocket = NULL;
    }

    return (_ClientSocket != NULL);
}

void ClientSession::CloseSocket()
{
    if (_ClientSocket && !_ClientSocket->IsClosing())
        _ClientSocket->close();
}

void ClientSession::CloseSubSocket()
{
    if (_NodeSocket && !_NodeSocket->IsClosing())
        _NodeSocket->close();
}

/*********************************************************/
/***                 SEND HANDLING                     ***/
/*********************************************************/
/// Send a packet to the client
void ClientSession::SendPacket(WorldPacket const* packet)
{
    if (!_ClientSocket)
        return;
    
    // can be used for the statistics page in the website, let's do that!

    // Code for network use statistic
    static uint64 sendPacketCount = 0;
    static uint64 sendPacketBytes = 0;

    static time_t firstTime = time(NULL);
    static time_t lastTime = firstTime;                     // next 60 secs start time

    static uint64 sendLastPacketCount = 0;
    static uint64 sendLastPacketBytes = 0;

    time_t cur_time = time(NULL);

    if ((cur_time - lastTime) < 60)
    {
        sendPacketCount+=1;
        sendPacketBytes+=packet->size();

        sendLastPacketCount+=1;
        sendLastPacketBytes+=packet->size();
    }
    else
    {
        uint64 minTime = uint64(cur_time - lastTime);
        uint64 fullTime = uint64(lastTime - firstTime);
        sLog->outString("Send all time packets count: " UI64FMTD " bytes: " UI64FMTD " avr.count/sec: %f avr.bytes/sec: %f time: %u",
            sendPacketCount,sendPacketBytes,float(sendPacketCount)/fullTime,float(sendPacketBytes)/fullTime,uint32(fullTime));
        sLog->outString("Send last min packets count: " UI64FMTD " bytes: " UI64FMTD " avr.count/sec: %f avr.bytes/sec: %f",
            sendLastPacketCount,sendLastPacketBytes,float(sendLastPacketCount)/minTime,float(sendLastPacketBytes)/minTime);
        
        // update our db data
        LogonDatabase.PQuery("REPLACE INTO node_statistics (nodeid, packetcount, bytes, pps, bs) VALUES (%u, %u, %u, %f, %f)",
            realmID, sendLastPacketCount, sendLastPacketBytes, abs(float(sendLastPacketCount) / minTime), abs(float(sendLastPacketBytes) / minTime));
        
        
        lastTime = cur_time;
        sendLastPacketCount = 1;
        sendLastPacketBytes = packet->wpos();               // wpos is real written size
    }
                                                // !TRINITY_DEBUG

    if (packet->GetOpcode() == CMSG_MESSAGECHAT)
        sLog->outString("[SENT PACKET]: Received opcode 0x%.4X (%s)", packet->GetOpcode(), LookupOpcodeName(packet->GetOpcode()));

    if (_ClientSocket->SendPacket(*packet) == -1)
        CloseSocket();
}
void ClientSession::SendPacketToNode(WorldPacket const* packet)
{
    if(packet->GetOpcode() == CMSG_MESSAGECHAT)
        sLog->outString("[SENT PACKET TO NODE]: Received opcode 0x%.4X (%s)", packet->GetOpcode(), LookupOpcodeName(packet->GetOpcode()));

    if (!_NodeSocket)
        return;

    if (_NodeSocket->SendPacket(*packet) == -1)
        CloseSubSocket();
}


void ClientSession::SendAuthWaitQue(uint32 position)
{
    if (position == 0)
    {
        WorldPacket packet(SMSG_AUTH_RESPONSE, 1);
        packet << uint8(AUTH_OK);
        SendPacket(&packet);
    }
    else
    {
        WorldPacket packet(SMSG_AUTH_RESPONSE, 6);
        packet << uint8(AUTH_WAIT_QUEUE);
        packet << uint32(position);
        packet << uint8(0);                                 // unk
        SendPacket(&packet);
    }
}

void ClientSession::SendCommandStringToNode(const char *args, ...)
{
    if (args)
    {
        va_list ap;
        char szStr[1024];
        szStr[0] = '\0';
        va_start(ap, args);
        vsnprintf(szStr, 1024, args, ap);
        va_end(ap);

        WorldPacket packet(CMSG_MESSAGECHAT);
        packet << uint32(CHAT_MSG_SAY);
        packet << uint32(LANG_UNIVERSAL);
        packet << szStr;
        SendPacketToNode(&packet);
    }
}

void ClientSession::SendPlayerLogin()
{
    WorldPacket packet (CMSG_PLAYER_LOGIN);
    packet << _player->GetGUID();
    SendPacketToNode(&packet);
}

/*********************************************************/
/***                  PLAYER STUFF                     ***/
/*********************************************************/
void ClientSession::SetPlayer(Player *plr)
{
    _player = plr;

    // set m_GUID that can be used while player loggined and later until m_playerRecentlyLogout not reset
    if (_player)
        _GUIDLow = _player->GetGUIDLow();
}

void ClientSession::PlayerLogout()
{
    // CleanUp account list
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ACCOUNT_ONLINE);
    stmt->setUInt32(0, GetAccountId());
    CharacterDatabase.Execute(stmt);

    if (_player)
    {
        ///- Logout
        _player->Logout(GetAccountId());

        ///- remove the Player
        sObjectMgr->Player_Remove(_player);
        SetPlayer(NULL);
        m_chlist.clear();
    }
}

void ClientSession::KickPlayer()
{
    SetFlag(FLAG_ACCOUNT_KICKED);
    RemoveFlag(FLAG_ACCOUNT_RECONNECT);
}

/*********************************************************/
/***                      FLAGS                        ***/
/*********************************************************/
void ClientSession::SetFlag(uint32 newFlag)
{
    _sessionflags.fetch_or(newFlag);
}

void ClientSession::RemoveFlag(uint32 oldFlag)
{
    _sessionflags.fetch_and(~oldFlag);
}

//misc
void ClientSession::ProcessQueryCallbacks()
{
    {
        PreparedQueryResult result;

        //! HandleCharEnumOpcode
        if (_charEnumCallback.ready())
        {
            _charEnumCallback.get(result);
            HandleCharEnum(result);
            _charEnumCallback.cancel();
        }
        //! HandlePlayerLoginOpcode
        if (_charLoginCallback.ready())
        {
            SQLQueryHolder* param;
            _charLoginCallback.get(param);
            HandlePlayerLogin((LoginQueryHolder*)param);
            _charLoginCallback.cancel();
        }
        //- HandleCharAddIgnoreOpcode
        if (_addIgnoreCallback.ready())
        {
            _addIgnoreCallback.get(result);
            HandleAddIgnoreOpcodeCallBack(result);
            _addIgnoreCallback.cancel();
        }
        //! HandleAddFriendOpcode
        if (_addFriendCallback.IsReady())
        {
            std::string param = _addFriendCallback.GetParam();
            _addFriendCallback.GetResult(result);
            HandleAddFriendOpcodeCallBack(result, param);
            _addFriendCallback.FreeResult();
        }
    }
}

/*********************************************************/
/***                   TUTORIALS                       ***/
/*********************************************************/

//Tutorial
void ClientSession::LoadTutorialsData()
{
    memset(_Tutorials, 0, sizeof(uint32) * MAX_ACCOUNT_TUTORIAL_VALUES);

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_TUTORIALS);
    stmt->setUInt32(0, GetAccountId());
    if (PreparedQueryResult result = CharacterDatabase.Query(stmt))
        for (uint8 i = 0; i < MAX_ACCOUNT_TUTORIAL_VALUES; ++i)
            _Tutorials[i] = (*result)[i].GetUInt32();

    _TutorialsChanged = false;
}

void ClientSession::SaveTutorialsData(SQLTransaction& trans)
{
    if (!_TutorialsChanged)
        return;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_HAS_TUTORIALS);
    stmt->setUInt32(0, GetAccountId());
    bool hasTutorials = !CharacterDatabase.Query(stmt).null();
    // Modify data in DB
    stmt = CharacterDatabase.GetPreparedStatement(hasTutorials ? CHAR_UPD_TUTORIALS : CHAR_INS_TUTORIALS);
    for (uint8 i = 0; i < MAX_ACCOUNT_TUTORIAL_VALUES; ++i)
        stmt->setUInt32(i, _Tutorials[i]);
    stmt->setUInt32(MAX_ACCOUNT_TUTORIAL_VALUES, GetAccountId());
    trans->Append(stmt);

    _TutorialsChanged = false;
}

void ClientSession::SendTutorialsData()
{
    WorldPacket data(SMSG_TUTORIAL_FLAGS, 4 * MAX_ACCOUNT_TUTORIAL_VALUES);
    for (uint8 i = 0; i < MAX_ACCOUNT_TUTORIAL_VALUES; ++i)
        data << _Tutorials[i];
    SendPacket(&data);
}

void ClientSession::Handle_SMSG_AUTH_RESPONSE(WorldPacket& recvPacket)
{
    uint8 code;
    recvPacket >> code;
    if (code == AUTH_OK)
    {
        if (HasFlag(FLAG_ACCOUNT_WAIT_FOR_OK))
        {
            if (_player)
            {
                //
                SendPlayerLogin();
                //Kein Master? Dann ein paar ZusatzInfos reinquetschen
                //if (sRoutingHelper->GetNodeConnectionData(_nodeId).type == 2)
                //{
                //    recvPacket << _player->;
                //}
            }
            RemoveFlag(FLAG_ACCOUNT_WAIT_FOR_OK);
        }

        if (HasFlag(FLAG_TRANSFER_PENDING))
        {
            RemoveFlag(FLAG_TRANSFER_PENDING);
        }
    }
}

void ClientSession::Handle_NODE_TRANSFER_ACK(WorldPacket& recvPacket)
{
    uint32 _transguid;
    uint8 action;
    WorldPacket packet;

    recvPacket >> action;
    switch (action)
    {
        case CL_DEF_TRANSFER_OK:
            recvPacket >> _transguid;
            packet.Initialize(CMSG_PLAYER_LOGIN);
            packet << uint64(_transguid);
            SendPacketToNode(&packet);
        break;
        default:
            break;
    }
}
//Handle
void ClientSession::Handle_NULL(WorldPacket &recv_data)
{}

void ClientSession::Handle_ServerSide(WorldPacket &recv_data)
{}

const char *ClientSession::GetTrinityString(int32 entry) const
{
    return sObjectMgr->GetTrinityString(entry, GetSessionDbLocaleIndex());
}

void ClientSession::InitWarden(BigNumber* K, std::string const& os)
{
    if (os == "Win")
    {
        m_Warden = new WardenWin();
        m_Warden->Init(this, K);
    }
    else if (os == "OSX")
    {
        // NO OSX/LINUX SUPPORT
    }
}

void ClientSession::SendServerMessageToPlayer(Player* player, const char* msg)
{
    if (!player)
        return;

    uint32 messageLength = (msg ? strlen(msg) : 0) + 1;
    WorldPacket data(SMSG_MESSAGECHAT, 100);
    data << uint8(CHAT_MSG_SYSTEM);
    data << uint32(LANG_UNIVERSAL);
    data << uint64(0);
    data << uint32(0);
    data << uint64(0);
    data << uint32(messageLength);
    data << msg;
    data << uint8(0);
    SendPacket(&data);
}

void ClientSession::SetStunned(bool apply)
{
    if (apply)
    {
        if (GetPlayer())
        { 
            SendServerMessageToPlayer(GetPlayer(), "|cffFF0000ALERT:|r We have lost connection to the node, please wait till we reconnect...");
            WorldPacket data2(SMSG_FORCE_MOVE_ROOT, 8);
            data2.append(GetPlayer()->GetPackGUID());
            data2 << uint32(0);
            SendPacket(&data2);
        }
    }
    else
    {
        if (GetPlayer())
        { 
            SendServerMessageToPlayer(GetPlayer(), "|cff00FF00ALERT:|r Reconnected to the node successfully.");
            WorldPacket data(SMSG_FORCE_MOVE_UNROOT, 8+4);
            data.append(GetPlayer()->GetPackGUID());
            data << uint32(0);
            SendPacket(&data);
        }
    }
}

void ClientSession::SendFakeChatNotification(uint8 type, std::string msg, uint32 lang)
{
    WorldPacket data(SMSG_MESSAGECHAT, 200);

    if (type == CHAT_MSG_WHISPER)
        type = CHAT_MSG_WHISPER_INFORM;

    GetPlayer()->BuildPlayerChat(&data, type, msg, lang);
    SendPacket(&data);
}

void ClientSession::SendFakeChannelNotification(std::string channel, std::string msg, uint32 lang)
{
    WorldPacket data(SMSG_MESSAGECHAT, 200);
    data << uint8(CHAT_MSG_CHANNEL);
    data << uint32(lang);
    data << uint64(GetPlayer()->GetGUID());     // 2.1.0
    data << uint32(0);                          // 2.1.0
    data << channel;
    data << uint64(GetPlayer()->GetGUID());
    data << uint32(msg.length() + 1);
    data << msg;
    data << uint8(GetPlayer()->GetChatTag());

    SendPacket(&data);
}

void ClientSession::Handle_SMSG_AUTH_CHALLENGE(WorldPacket& recvPacket)
{
    uint32 seed;

    recvPacket.read_skip<uint32>();
    recvPacket >> seed;

    QueryResult result = LoginDatabase.PQuery(
        "SELECT username FROM account WHERE id = '%u'",
        GetAccountId());

    // Stop if the account is not found
    if (!result)
    {
        sLog->outString("FAIL:: Auth_by_Node");
        KickPlayer();
    }

    Field *fields = result->Fetch();

    //Auth the Session
    WorldPacket packet(CMSG_AUTH_SESSION);
    packet << fields[0].GetString();
    packet << GetAccountId();
    packet << uint32(GetSecurity());
    packet << bool(IsAuthed());
    packet << bool(HasFlag(FLAG_FORCE_TELEPORT));
    if (HasFlag(FLAG_FORCE_TELEPORT))
        RemoveFlag(FLAG_FORCE_TELEPORT);

    SendPacketToNode(&packet);
}

void ClientSession::SendToNode(uint32 n, uint32 m)
{
    std::string s = "|cffcc0066][Clustering][ |cffb3ffffYou have been moved to node:|r ";
    s += "|cff";
    switch(n)
    {
        case 1:
            s += "3abec5";
            s += "Capital Cities Node (master node)";
            break;
        case 2:
            s += "3abec5";
            s += "Eastern Kingdoms Node (map node [1])";
            break;
        case 3:
            s += "3abec5";
            s += "Kalimdor Node (map node [1])";
            break;
        case 4:
            s += "3abec5";
            s += "Outland Node (map node [1])";
            break;
        case 5:
            s += "3abec5";
            s += "Northrend Node (map node [1])";
            break;
        case 6:
            s += "3abec5";
            s += "PvE Node (raids/dungeons) (map node [1])";
            break;
        case 7:
            s += "3abec5";
            s += "PvP Node (battlegrounds/arenas) (map node [1])";
            break;
        default:
            s += "ff0000 +error+ ";
            break;
    }
    s += "|r";

    const char* rslt = s.c_str();
    SendServerMessageToPlayer(GetPlayer(), rslt);
}