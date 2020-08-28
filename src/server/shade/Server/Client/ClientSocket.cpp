/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#include <ace/Message_Block.h>
#include <ace/OS_NS_string.h>
#include <ace/OS_NS_unistd.h>
#include <ace/os_include/arpa/os_inet.h>
#include <ace/os_include/netinet/os_tcp.h>
#include <ace/os_include/sys/os_types.h>
#include <ace/os_include/sys/os_socket.h>
#include <ace/OS_NS_string.h>
#include <ace/Reactor.h>
#include <ace/Auto_Ptr.h>

#include "ClientSocket.h"
#include "Common.h"

#include "Util.h"
#include "Proxy.h"
#include "WorldPacket.h"
#include "SharedDefines.h"
#include "ByteBuffer.h"
#include "Opcodes.h"
#include "DatabaseEnv.h"
#include "BigNumber.h"
#include "SHA1.h"
#include "ClientSocket.h"
#include "ClientSessionMgr.h"
#include "Log.h"
#include "AccountMgr.h"

#include "BlizzardHeader.hpp"

ClientSocket::ClientSocket(): AcceptSocket(sizeof(Blizzard::ClientPktHeader)),
    m_LastPingTime(ACE_Time_Value::zero),
    m_OverSpeedPings(0),
    m_Seed(static_cast<uint32> (rand32())),
    _latency(0),
    _lastActionTime(time(NULL))
{
}

ClientSocket::~ClientSocket()
{
}

void ClientSocket::_buildAuthChallengePacket(WorldPacket &packet)
{
    packet.Initialize(SMSG_AUTH_CHALLENGE, 40);
    packet << uint32(1);                                    // 1...31
    packet << m_Seed;

    BigNumber seed1;
    seed1.SetRand(16 * 8);
    packet.append(seed1.AsByteArray(16).get(), 16);               // new encryption seeds

    BigNumber seed2;
    seed2.SetRand(16 * 8);
    packet.append(seed2.AsByteArray(16).get(), 16);               // new encryption seeds
}

uint32 ClientSocket::GetLatency() const
{
    return _latency;
}

uint32 ClientSocket::GetLastActionTime() const
{
    return _lastActionTime;
}

int ClientSocket::_sendPacket(const WorldPacket &pct)
{
    Blizzard::ServerPktHeader header(pct.size() + 2, pct.GetOpcode());
    m_Crypt.EncryptSend((uint8*) header.header, header.getHeaderLength());

    if (_outBuffer->space() >= pct.size() + header.getHeaderLength() && msg_queue()->is_empty())
    {
        // Put the packet on the buffer.
        if (_outBuffer->copy((char*) header.header, header.getHeaderLength()) == -1)
            ACE_ASSERT(false);

        if (!pct.empty())
            if (_outBuffer->copy((char*) pct.contents(), pct.size()) == -1)
                ACE_ASSERT(false);
    }
    else
    {
        // Enqueue the packet.
        ACE_Message_Block* mb;

        ACE_NEW_RETURN(mb, ACE_Message_Block(pct.size() + header.getHeaderLength()), -1);

        mb->copy((char*) header.header, header.getHeaderLength());

        if (!pct.empty())
            mb->copy((const char*) pct.contents(), pct.size());

        if (msg_queue()->enqueue_tail(mb, (ACE_Time_Value*) &ACE_Time_Value::zero) == -1)
        {
            sLog->outError("ClientSocket::SendPacket enqueue_tail failed");
            mb->release();
            return -1;
        }
    }

    return 0;
}

int ClientSocket::_handleInputHeader()
{
    ACE_ASSERT(_recvWPct == NULL);

    ACE_ASSERT(_header.length() == sizeof(Blizzard::ClientPktHeader));

    m_Crypt.DecryptRecv((uint8*) _header.rd_ptr(), sizeof(Blizzard::ClientPktHeader));

    Blizzard::ClientPktHeader& header = *((Blizzard::ClientPktHeader*) _header.rd_ptr());

    EndianConvertReverse(header.size);
    EndianConvert(header.cmd);

    if ((header.size < 4) || (header.size > 10240) || (header.cmd > 10240))
    {
        sLog->outError("ClientSocket::_handleInputHeader failed");
        errno = EINVAL;
        return -1;
    }

    header.size -= 4;

    ACE_NEW_RETURN(_recvWPct, WorldPacket((uint16) header.cmd, header.size), -1);

    if (header.size > 0)
    {
        _recvWPct->resize(header.size);
        _recvPct.base((char*) _recvWPct->contents(), _recvWPct->size());
    }
    else
    {
        ACE_ASSERT(_recvPct.space() == 0);
    }

    return 0;
}

int ClientSocket::_processIncoming(WorldPacket *newPacket)
{
    ACE_ASSERT(newPacket);

    // manage memory ;)
    ACE_Auto_Ptr<WorldPacket> aptr(newPacket);

    uint16 opcode = newPacket->GetOpcode();

    if (IsClosing())
    {
        sLog->outError("ClientSocket::_processIncoming IsClosing");
        return -1;
    }

    try
    {
        switch (opcode)
        {
            case CMSG_PING:
                return _handlePing(*newPacket);
            case CMSG_AUTH_SESSION:
                if (IsAuthenticated())
                {
                    sLog->outError("ClientSocket::ProcessIncoming: IP %s send CMSG_AUTH_SESSION again", GetRemoteAddress().c_str());
                    return -1;
                }
                return _handleAuthSession(*newPacket);
            case CMSG_KEEP_ALIVE:
                sLog->outStaticDebug("CMSG_KEEP_ALIVE, size: " UI64FMTD, uint64(newPacket->size()));
                return 0;
            default:
            {
                if (IsAuthenticated() && opcode < NUM_MSG_TYPES)
                {
                    _lastActionTime = time(NULL);

                    aptr.release();

                    if (opcodeTable[opcode].multipass)
                        _multiPacketQueue.add(newPacket);
                    else
                        _singlePacketQueue.add(newPacket);
                }
                return 0;
            }
        }
    }
    catch (ByteBufferException &)
    {
        sLog->outError("ClientSocket::ProcessIncoming ByteBufferException occured while parsing an instant handled packet (opcode: %u) from client %s. Disconnected client.",
            opcode, GetRemoteAddress().c_str());

        if (sLog->IsOutDebug())
        {
            sLog->outString("Dumping error causing packet:");
            newPacket->hexlike();
        }

        return -1;
    }

    ACE_NOTREACHED(return 0);
}

int ClientSocket::_handleAuthSession(WorldPacket& recvPacket)
{
    // NOTE: ATM the socket is singlethread, have this in mind ...
    uint8 digest[20];
    uint32 clientSeed;
    uint32 unk2, unk3, unk5, unk6, unk7;
    uint64 unk4;
    uint32 BuiltNumberClient;
    uint32 id, security;
    //uint8 expansion = 0;
    LocaleConstant locale;
    std::string account;
    SHA1Hash sha;
    WorldPacket packet, SendAddonPacked;

    BigNumber k;

    if (sLogon->IsClosed() || sLogon->IsHalted())
    {
        packet.Initialize(SMSG_AUTH_RESPONSE, 1);
        packet << uint8(AUTH_REJECT);
        SendPacket(packet);

        sLog->outError("ClientSocket::HandleAuthSession: World closed, denying client (%s).", GetRemoteAddress().c_str());
        return -1;
    }

    // Read the content of the packet
    recvPacket >> BuiltNumberClient;                        // for now no use
    recvPacket >> unk2;
    recvPacket >> account;
    recvPacket >> unk3;
    recvPacket >> clientSeed;
    recvPacket >> unk5 >> unk6 >> unk7;
    recvPacket >> unk4;
    recvPacket.read(digest, 20);

    sLog->outDetail("ClientSocket::HandleAuthSession: client %u, unk2 %u, account %s, unk3 %u, clientseed %u",
        BuiltNumberClient,
        unk2,
        account.c_str(),
        unk3,
        clientSeed);

    // Get the account information from the realmd database
    //         0           1        2       3          4         5       6          7   8
    // SELECT id, sessionkey, last_ip, locked, expansion, mutetime, locale, recruiter, os FROM account WHERE username = ?
    PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_INFO_BY_NAME);

    stmt->setString(0, account);

    PreparedQueryResult result = LoginDatabase.Query(stmt);

    // Stop if the account is not found
    if (!result)
    {
        packet.Initialize(SMSG_AUTH_RESPONSE, 1);
        packet << uint8(AUTH_UNKNOWN_ACCOUNT);

        SendPacket(packet);

        sLog->outError("ClientSocket::HandleAuthSession: Sent Auth Response (unknown account).");
        return -1;
    }

    Field* fields = result->Fetch();

    uint8 expansion = fields[4].GetUInt8();
    uint32 world_expansion = sLogon->getIntConfig(CONFIG_EXPANSION);
    if (expansion > world_expansion)
        expansion = world_expansion;

    ///- Re-check ip locking (same check as in realmd).
    if (fields[3].GetUInt8() == 1) // if ip is locked
    {
        if (strcmp(fields[2].GetCString(), GetRemoteAddress().c_str()))
        {
            packet.Initialize(SMSG_AUTH_RESPONSE, 1);
            packet << uint8(AUTH_FAILED);
            SendPacket(packet);

            sLog->outError("ClientSocket::HandleAuthSession: Sent Auth Response (Account IP differs).");
            return -1;
        }
    }

    id = fields[0].GetUInt32();
    /*
    if (security > SEC_ADMINISTRATOR)                        // prevent invalid security settings in DB
    security = SEC_ADMINISTRATOR;
    */

    k.SetHexStr(fields[1].GetCString());

    int64 mutetime = fields[5].GetInt64();
    //! Negative mutetime indicates amount of seconds to be muted effective on next login - which is now.
    if (mutetime < 0)
    {
        mutetime = time(NULL) + llabs(mutetime);

        PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_MUTE_TIME);

        stmt->setInt64(0, mutetime);
        stmt->setUInt32(1, id);

        LoginDatabase.Execute(stmt);
    }

    locale = LocaleConstant(fields[6].GetUInt8());
    if (locale >= TOTAL_LOCALES)
        locale = LOCALE_enUS;

    uint32 recruiter = fields[7].GetUInt32();
    std::string os = fields[8].GetString();

    // Checks gmlevel per Realm
    stmt = LoginDatabase.GetPreparedStatement(LOGIN_GET_GMLEVEL_BY_REALMID);

    stmt->setUInt32(0, id);
    stmt->setInt32(1, int32(realmID));

    result = LoginDatabase.Query(stmt);

    if (!result)
        security = 0;
    else
    {
        fields = result->Fetch();
        security = fields[0].GetUInt8();
    }

    // Re-check account ban (same check as in realmd)
    stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_BANS);

    stmt->setUInt32(0, id);
    stmt->setString(1, GetRemoteAddress());

    PreparedQueryResult banresult = LoginDatabase.Query(stmt);

    if (banresult) // if account banned
    {
        packet.Initialize(SMSG_AUTH_RESPONSE, 1);
        packet << uint8(AUTH_BANNED);
        SendPacket(packet);

        sLog->outError("ClientSocket::HandleAuthSession: Sent Auth Response (Account banned).");
        return -1;
    }

    // Check locked state for server
    AccountTypes allowedAccountType = sLogon->GetPlayerSecurityLimit();
    //sLog->outString("Allowed Level: %u Player Level %u", allowedAccountType, AccountTypes(security));
    if (AccountTypes(security) < allowedAccountType)
    {
        WorldPacket Packet(SMSG_AUTH_RESPONSE, 1);
        Packet << uint8(AUTH_UNAVAILABLE);

        SendPacket(packet);

        sLog->outError("ClientSocket::HandleAuthSession: User tries to login but his security level is not enough");
        return -1;
    }

    // Check that Key and account name are the same on client and server
    uint32 t = 0;
    uint32 seed = m_Seed;

    sha.UpdateData(account);
    sha.UpdateData((uint8 *) & t, 4);
    sha.UpdateData((uint8 *) & clientSeed, 4);
    sha.UpdateData((uint8 *) & seed, 4);
    sha.UpdateBigNumbers(&k, NULL);
    sha.Finalize();

    if (memcmp(sha.GetDigest(), digest, 20))
    {
        packet.Initialize(SMSG_AUTH_RESPONSE, 1);
        packet << uint8(AUTH_FAILED);

        SendPacket(packet);
        sLog->outError("ClientSocket::HandleAuthSession: Sent Auth Response (authentification failed).");
        return -1;
    }

    std::string address = GetRemoteAddress();

    sLog->outStaticDebug("ClientSocket::HandleAuthSession: Client '%s' authenticated successfully from %s.",
        account.c_str(),
        address.c_str());

    // Check if this user is by any chance a recruiter
    stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_RECRUITER);

    stmt->setUInt32(0, id);

    result = LoginDatabase.Query(stmt);

    bool isRecruiter = false;
    if (result)
        isRecruiter = true;

    // Update the last_ip in the database

    stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_LAST_IP);

    stmt->setString(0, address);
    stmt->setString(1, account);

    LoginDatabase.Execute(stmt);

    ClientSession *session;

    // NOTE ATM the socket is single-threaded, have this in mind ...
    ACE_NEW_RETURN(session, ClientSession(id, this, AccountTypes(security), expansion, mutetime, locale, recruiter, isRecruiter), -1);

    m_Crypt.Init(&k);

    session->ReadAddonsInfo(recvPacket);
    session->InitWarden(&k, os);

    SetAuthenticated();

    // Sleep this Network thread for
    uint32 sleepTime = sLogon->getIntConfig(CONFIG_SESSION_ADD_DELAY);
    ACE_OS::sleep(ACE_Time_Value(0, sleepTime));

    sClientSessionMgr->AddSession(session);
    session->remove_reference();

    return 0;
}

int ClientSocket::_handlePing(WorldPacket& recvPacket)
{
    uint32 ping;
    uint32 latency;

    // Get the ping packet content
    recvPacket >> ping;
    recvPacket >> latency;

    if (m_LastPingTime == ACE_Time_Value::zero)
        m_LastPingTime = ACE_OS::gettimeofday(); // for 1st ping
    else
    {
        ACE_Time_Value cur_time = ACE_OS::gettimeofday();
        ACE_Time_Value diff_time(cur_time);
        diff_time -= m_LastPingTime;
        m_LastPingTime = cur_time;

        if (diff_time < ACE_Time_Value(27))
        {
            ++m_OverSpeedPings;

            uint32 max_count = sLogon->getIntConfig(CONFIG_MAX_OVERSPEED_PINGS);

            if (max_count && m_OverSpeedPings > max_count)
            {
                sLog->outError("ClientSocket::_handlePing OverSpeedPings");
                return -1;
            }
        }
        else
            m_OverSpeedPings = 0;
    }

    _latency = latency;

    WorldPacket packet(SMSG_PONG, 4);
    packet << ping;
    return SendPacket(packet);
}
