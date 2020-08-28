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

#include "NodeSocket.h"
#include "Common.h"

#include "Util.h"
#include "Proxy.h"
#include "WorldPacket.h"
#include "SharedDefines.h"
#include "ByteBuffer.h"
#include "Opcodes.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "BigNumber.h"
#include "SHA1.h"
#include "ClientSession.h"
#include "Log.h"

#include "FlexiHeader.hpp"

NodeSocket::NodeSocket() : Socket(sizeof(Flexi::ClientPktHeader)),
_seed(static_cast<uint32> (rand32()))
{
}

int NodeSocket::_sendPacket(const WorldPacket &pct)
{
    Flexi::ServerPktHeader header(pct.size() + 2, pct.GetOpcode());
    _crypt.EncryptSend((uint8 *) header.header, header.getHeaderLength());

    if (_outBuffer->space() >= pct.size() + header.getHeaderLength() && msg_queue()->is_empty())
    {
        // Put the packet on the buffer.
        if (_outBuffer->copy((char *) header.header, header.getHeaderLength()) == -1)
            ACE_ASSERT(false);

        if (!pct.empty())
            if (_outBuffer->copy((char *) pct.contents(), pct.size()) == -1)
                ACE_ASSERT(false);
    }
    else
    {
        // Enqueue the packet.
        ACE_Message_Block *mb;

        ACE_NEW_RETURN(mb, ACE_Message_Block(pct.size() + header.getHeaderLength()), -1);

        mb->copy((char *) header.header, header.getHeaderLength());

        if (!pct.empty())
            mb->copy((const char *) pct.contents(), pct.size());

        if (msg_queue()->enqueue_tail(mb, (ACE_Time_Value *) &ACE_Time_Value::zero) == -1)
        {
            sLog->outError("NodeSocket::SendPacket enqueue_tail failed");
            mb->release();
            return -1;
        }
    }

    return 0;
}

int NodeSocket::_handleInputHeader()
{
    ACE_ASSERT(_recvWPct == NULL);

    ACE_ASSERT(_header.length() == sizeof (Flexi::ClientPktHeader));

    _crypt.DecryptRecv((uint8 *) _header.rd_ptr(), sizeof (Flexi::ClientPktHeader));

    Flexi::ClientPktHeader &header = *((Flexi::ClientPktHeader *) _header.rd_ptr());

    EndianConvertReverse(header.size);
    EndianConvert(header.cmd);

    if (header.size < 2)
    {
        errno = EINVAL;
        return -1;
    }

    header.size -= 2;

    ACE_NEW_RETURN(_recvWPct, WorldPacket((uint16) header.cmd, (uint32) header.size), -1);

    if (header.size > 0)
    {
        _recvWPct->resize((uint32) header.size);
        _recvPct.base((char *) _recvWPct->contents(), _recvWPct->size());
    }
    else
    {
        ACE_ASSERT(_recvPct.space() == 0);
    }

    return 0;
}

int NodeSocket::_processIncoming(WorldPacket *newPacket)
{
    ACE_ASSERT(newPacket);

    // manage memory ;)
    ACE_Auto_Ptr<WorldPacket> aptr(newPacket);

    const ACE_UINT16 opcode = newPacket->GetOpcode();

    /*sLog->outDebug (LOG_FILTER_NETWORKIO, "NodeSocket_IN: SOCKET: %u LENGTH: %u OPCODE: %s (0x%.4X)",
                   (uint32) get_handle (),
                   new_pct->size (),
                   LookupOpcodeName (new_pct->GetOpcode ()),
                   new_pct->GetOpcode ());*/

    //We've got no Session for the node, so we have to handle all NodeOpcodes here :)
    try
    {
        if (opcode < NUM_MSG_TYPES)
        {
            aptr.release();

            if (opcodeTable[opcode].multipass)
                _multiPacketQueue.add(newPacket);
            else
                _singlePacketQueue.add(newPacket);
        }

        return 0;
    }
    catch (ByteBufferException &)
    {
        if (sLog->IsOutDebug())
        {
            sLog->outString("Dumping error causing packet:");
            newPacket->hexlike();
        }

        return -1;
    }

    ACE_NOTREACHED(return 0);
    return 0;
}
