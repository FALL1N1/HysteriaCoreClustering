#include "PoolSocket.h"

#include "ObjectMgr.h"
#include "Player.h"
#include "AccountMgr.h"
#include "PoolSession.h"
#include "WorldSession.h"
#include "WorldPacket.h"
#include "ByteBuffer.h"
#include "Common.h"
#include "Log.h"
#include "zlib.h"
#include "ClusterDefines.h"
#include "GroupMgr.h"
PoolSession::PoolSession(uint32 ServerID, PoolSocket *sock)
    : m_PoolSocket(sock), _serverId(ServerID) {}

PoolSession::~PoolSession()
{
    /// - If have unclosed socket, close it
    CloseSockets();

    ///- empty incoming packet queue
    WorldPacket* packet = NULL;
    while (_recvPoolQueue.next(packet))
        delete packet;
}

void PoolSession::CloseSockets()
{
    if (m_PoolSocket && !m_PoolSocket->IsClosed())
    {
        m_PoolSocket->CloseSocket();
        m_PoolSocket->RemoveReference();
        m_PoolSocket = NULL;
    }
}

void PoolSession::QueueServerPacket(WorldPacket* new_packet)
{
    _recvPoolQueue.add(new_packet);
}

bool PoolSession::Update(uint32 diff)
{
    if (!UpdateServerIO(diff))
        return false;

    return true;
}

bool PoolSession::UpdateServerIO(uint32 /*diff*/)
{
    //Would be changed later
    if (!m_PoolSocket || m_PoolSocket->IsClosed())
        return false;

    WorldPacket* packet;

    while (m_PoolSocket && !m_PoolSocket->IsClosed() && _recvPoolQueue.next(packet))
    {
        sLog->outDebug(LOG_FILTER_NETWORKIO, "SESSION_CLIENT: Received opcode 0x%.4X (%s)", packet->GetOpcode(), LookupOpcodeName(packet->GetOpcode()));
        if (packet->GetOpcode() >= NUM_MSG_TYPES)
        {
            sLog->outError("SESSION_NODE: received non-existed opcode %s (0x%.4X)",
                LookupOpcodeName(packet->GetOpcode()),
                packet->GetOpcode());
        }
        else
        {
            try
            {
                switch (packet->GetOpcode())
                {
                    case LOGON_INIT_NODE:
                        Handle_LOGON_INIT_NODE(*packet);
                        break;
                    case LOGON_SYNC_DATA:
                        Handle_LOGON_SYNC_DATA(*packet);
                        break;
                }
            }
            catch (ByteBufferException &)
            {
                //sLog->outError("WorldSession::Update ByteBufferException occured while parsing a packet (opcode: %u) from client %s, accountid=%i. Skipped packet.",
                //       packet->GetOpcode(), GetRemoteAddress().c_str(), GetAccountId());
                if (sLog->IsOutDebug())
                {
                    sLog->outDebug(LOG_FILTER_NETWORKIO, "Dumping error causing packet:");
                    packet->hexlike();
                }
            }
        }
        delete packet;
    }

    return true;
}

//Sendhandlings
/// Send a packet to the client
void PoolSession::SendPacket(WorldPacket const* packet)
{
    if (!m_PoolSocket)
        return;
    /*
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
        sLog->outString("Send all time packets count: " UI64FMTD " bytes: " UI64FMTD " avr.count/sec: %f avr.bytes/sec: %f time: %u",sendPacketCount,sendPacketBytes,float(sendPacketCount)/fullTime,float(sendPacketBytes)/fullTime,uint32(fullTime));
        sLog->outString("Send last min packets count: " UI64FMTD " bytes: " UI64FMTD " avr.count/sec: %f avr.bytes/sec: %f",sendLastPacketCount,sendLastPacketBytes,float(sendLastPacketCount)/minTime,float(sendLastPacketBytes)/minTime);

        lastTime = cur_time;
        sendLastPacketCount = 1;
        sendLastPacketBytes = packet->wpos();               // wpos is real written size
    }                                                   // !TRINITY_DEBUG
    */
    if (m_PoolSocket->SendPacket(*packet) == -1)
        m_PoolSocket->CloseSocket();
}

void PoolSession::SendInitACK(uint8 ack)
{
    //Send ACK
    WorldPacket packet(NODE_INIT_ACK);
    packet << uint8(ack);
    SendPacket(&packet);
}

//Handle
void PoolSession::Handle_NULL(WorldPacket &/*recv_data*/)
{
}

void PoolSession::Handle_ServerSide(WorldPacket &/*recv_data*/)
{
}

//Range,Item,Mail,...
void PoolSession::Handle_LOGON_INIT_NODE(WorldPacket &recvPacket)
{
    float rate_float_value[38];
    uint32 rate_int_value[10];
    uint32 ItemGuid;
    uint32 MailGuid;
    uint32 GroupGuid;
    uint32 CorpseGuid;

    recvPacket >> sObjectMgr->_hiRange;
    recvPacket >> ItemGuid;
    if (ItemGuid >= 0xFFFFFFFE)
    {
        sLog->outError("Low Item GUID %u is higher then %u", ItemGuid, 0xFFFFFFFE);
        SendInitACK(NODE_INIT_ACK_ITEM_GUID_FAIL);
        return;
    }
    sObjectMgr->_hiItemGuid = ItemGuid;
    //sLog->outError("PoolSession::Handle_LOGON_INIT_NODE RANGE %u ItemGuid %u", sObjectMgr->_hiRange, sObjectMgr->_hiItemGuid);

    recvPacket >> MailGuid;
    if (MailGuid >= 0xFFFFFFFE)
    {
        sLog->outError("Low Mail GUID %u is higher then %u", ItemGuid, 0xFFFFFFFE);
        SendInitACK(NODE_INIT_ACK_MAIL_GUID_FAIL);
        return;
    }
    sObjectMgr->_mailId = MailGuid;

    recvPacket >> GroupGuid;
    //sGroupMgr->SetNextGroupDbStoreId(GroupGuid);
    //sGroupMgr->NextGroupId = GroupGuid;

    recvPacket >> CorpseGuid;
    if (CorpseGuid >= 0xFFFFFFFE)
    {
        sLog->outError("Low Corpse GUID %u is higher then %u", CorpseGuid, 0xFFFFFFFE);
        SendInitACK(NODE_INIT_ACK_CORPSE_GUID_FAIL);
        return;
    }
    sObjectMgr->_hiCorpseGuid = CorpseGuid;

    for (int i = 0; i < 38; i++)
        recvPacket >> rate_float_value[i];

    sWorld->setRate(RATE_HEALTH, rate_float_value[0]);
    sWorld->setRate(RATE_POWER_MANA, rate_float_value[1]);
    sWorld->setRate(RATE_POWER_RAGE_INCOME, rate_float_value[2]);
    sWorld->setRate(RATE_POWER_RAGE_LOSS, rate_float_value[3]);
    sWorld->setRate(RATE_POWER_RUNICPOWER_INCOME, rate_float_value[4]);
    sWorld->setRate(RATE_POWER_RUNICPOWER_LOSS, rate_float_value[5]);
    sWorld->setRate(RATE_POWER_FOCUS, rate_float_value[6]);
    sWorld->setRate(RATE_POWER_ENERGY, rate_float_value[7]);
    sWorld->setRate(RATE_SKILL_DISCOVERY, rate_float_value[8]);
    sWorld->setRate(RATE_DROP_ITEM_POOR, rate_float_value[9]);
    sWorld->setRate(RATE_DROP_ITEM_NORMAL, rate_float_value[10]);
    sWorld->setRate(RATE_DROP_ITEM_UNCOMMON, rate_float_value[11]);
    sWorld->setRate(RATE_DROP_ITEM_RARE, rate_float_value[12]);
    sWorld->setRate(RATE_DROP_ITEM_EPIC, rate_float_value[13]);
    sWorld->setRate(RATE_DROP_ITEM_LEGENDARY, rate_float_value[14]);
    sWorld->setRate(RATE_DROP_ITEM_ARTIFACT, rate_float_value[15]);
    sWorld->setRate(RATE_DROP_ITEM_REFERENCED, rate_float_value[16]);
    sWorld->setRate(RATE_DROP_ITEM_REFERENCED_AMOUNT, rate_float_value[17]);
    sWorld->setRate(RATE_DROP_MONEY, rate_float_value[18]);
    sWorld->setRate(RATE_XP_KILL, rate_float_value[19]);
    sWorld->setRate(RATE_XP_QUEST, rate_float_value[20]);
    sWorld->setRate(RATE_XP_EXPLORE, rate_float_value[21]);
    sWorld->setRate(RATE_REPAIRCOST, rate_float_value[22]);
    sWorld->setRate(RATE_REST_INGAME, rate_float_value[23]);
    sWorld->setRate(RATE_REST_OFFLINE_IN_TAVERN_OR_CITY, rate_float_value[24]);
    sWorld->setRate(RATE_REST_OFFLINE_IN_WILDERNESS, rate_float_value[25]);
    sWorld->setRate(RATE_DAMAGE_FALL, rate_float_value[26]);
    sWorld->setRate(RATE_AUCTION_TIME, rate_float_value[27]);
    sWorld->setRate(RATE_AUCTION_DEPOSIT, rate_float_value[28]);
    sWorld->setRate(RATE_AUCTION_CUT, rate_float_value[29]);
    sWorld->setRate(RATE_HONOR, rate_float_value[30]);
//    sWorld->setRate(RATE_MINING_AMOUNT, rate_float_value[31]);
//    sWorld->setRate(RATE_MINING_NEXT, rate_float_value[32]);
    sWorld->setRate(RATE_TALENT, rate_float_value[33]);
    sWorld->setRate(RATE_REPUTATION_GAIN, rate_float_value[34]);
    sWorld->setRate(RATE_REPUTATION_LOWLEVEL_KILL, rate_float_value[35]);
    sWorld->setRate(RATE_REPUTATION_LOWLEVEL_QUEST, rate_float_value[36]);
    sWorld->setRate(RATE_REPUTATION_RECRUIT_A_FRIEND_BONUS, rate_float_value[37]);

    for (int i = 0; i < 10; i++)
        recvPacket >> rate_int_value[i];

    sWorld->setIntConfig(CONFIG_SKILL_GAIN_CRAFTING, rate_int_value[0]);
    sWorld->setIntConfig(CONFIG_SKILL_GAIN_DEFENSE, rate_int_value[1]);
    sWorld->setIntConfig(CONFIG_SKILL_GAIN_GATHERING, rate_int_value[2]);
    sWorld->setIntConfig(CONFIG_SKILL_GAIN_WEAPON, rate_int_value[3]);
    sWorld->setIntConfig(CONFIG_SKILL_CHANCE_ORANGE, rate_int_value[4]);
    sWorld->setIntConfig(CONFIG_SKILL_CHANCE_YELLOW, rate_int_value[5]);
    sWorld->setIntConfig(CONFIG_SKILL_CHANCE_GREEN, rate_int_value[6]);
    sWorld->setIntConfig(CONFIG_SKILL_CHANCE_GREY, rate_int_value[7]);
    sWorld->setIntConfig(CONFIG_SKILL_CHANCE_MINING_STEPS, rate_int_value[8]);
    sWorld->setIntConfig(CONFIG_SKILL_CHANCE_SKINNING_STEPS, rate_int_value[9]);

    SendInitACK(NODE_INIT_ACK_OK);

    //if (sWorld->getIntConfig(CONFIG_CORE_TYPE) == NODE_TYPE_MASTER)
        //sObjectMgr->HandleLostMailItems();
}

void PoolSession::Handle_LOGON_SYNC_DATA(WorldPacket &recvPacket)
{
    uint32 cmd = recvPacket.read<uint32>();

    switch (cmd)
    {
        case CL_DEF_GUID_SYNC:
        {
            uint32 subCommand = recvPacket.read<uint32>();

            switch (subCommand)
            {
                case ENTITYID_ITEM:
                    recvPacket >> sObjectMgr->_hiItemGuid;
                    break;
                case ENTITYID_MAIL:
                    recvPacket >> sObjectMgr->_mailId;
                    break;
                case ENTITYID_GROUP:
                    //recvPacket >> sGroupMgr->NextGroupId;
                    break;
                case ENTITYID_CORPSE:
                    recvPacket >> sObjectMgr->_hiCorpseGuid;
                    break;
            }
            recvPacket >> sObjectMgr->_hiRange;
            break;
        }
        case CL_DEF_SHUTDOWN:
        {
            uint32 time;
            recvPacket >> time;
            sWorld->ShutdownServ(time, 0, SHUTDOWN_EXIT_CODE);
            break;
        }
    }
}
