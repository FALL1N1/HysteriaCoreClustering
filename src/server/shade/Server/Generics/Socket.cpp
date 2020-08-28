#include "Socket.h"
#include "ace/os_include/netinet/os_tcp.h"

#include "Network.h"

Socket::Socket(uint32 ClientHeaderSize) : OSSocket(),
    _outBufferSize(65536),
    _outBuffer(0),
    _recvWPct(0),
    _recvPct(),
    _header(ClientHeaderSize),
    _clientHeaderSize(ClientHeaderSize),
    _closing(false)
{
    msg_queue()->high_water_mark(8*1024*1024);
    msg_queue()->low_water_mark(8*1024*1024);

    reference_counting_policy().value(Reference_Counting_Policy::ENABLED);
}

Socket::~Socket()
{
    WorldPacket* packet = NULL;
    while (_multiPacketQueue.next(packet))
        delete packet;

    while (_singlePacketQueue.next(packet))
        delete packet;

    if (_recvWPct)
    {
        delete _recvWPct;
        _recvWPct = NULL;
    }
}

bool Socket::IsClosing() const
{
    return _closing || closing_;
}

const std::string& Socket::GetRemoteAddress() const
{
    return _address;
}

int Socket::_directlySendPacket(const WorldPacket &pct)
{
    ACE_GUARD_RETURN(LockType, Guard, _outBufferLock, -1);

    if (IsClosing())
    {
        sLog->outError("Socket::SendPacket IsClosing() during Authentication");
        return -1;
    }

    return _sendPacket(pct);
}

int Socket::SendPacket(const WorldPacket &pct)
{
    ACE_GUARD_RETURN(LockType, Guard, _outBufferLock, -1);

    if (IsClosing())
    {
        sLog->outError("Socket::SendPacket IsClosing()");
        return -1;
    }

    if (_sendPacket(pct) < 0)
        return -1;

    return schedule_wakeup_output();
}

bool Socket::GetNextSinglePacket(WorldPacket* &packet)
{
    if (IsClosing())
        return false;

    return _singlePacketQueue.next(packet);
}

bool Socket::GetNextMultiPacket(WorldPacket* &packet)
{
    if (IsClosing())
        return false;

    return _multiPacketQueue.next(packet);
}

int Socket::_setup()
{
    // Prevent double call to this func.
    if (_outBuffer)
    {
        sLog->outError("Socket::SetSocketOptions double call");
        return -1;
    }

    int SockOutKBuff = sNetworkMgr->GetSockOutKBuff();
    if (SockOutKBuff >= 0)
    {
        if (peer().set_option(SOL_SOCKET, SO_SNDBUF, (void*)&SockOutKBuff, sizeof(int)) == -1 && errno != ENOTSUP)
        {
            sLog->outError("Socket::SetSocketOptions set_option SO_SNDBUF errno = %s", ACE_OS::strerror(errno));
            return -1;
        }
    }

    if (sNetworkMgr->UseNoDelay())
    {
        const int ndoption = 1;

        if (peer().set_option(ACE_IPPROTO_TCP, TCP_NODELAY, (void*)&ndoption, sizeof(int)) == -1)
        {
            sLog->outError("Socket::SetSocketOptions: peer().set_option TCP_NODELAY errno = %s", ACE_OS::strerror(errno));
            return -1;
        }
    }

    _outBufferSize = size_t(sNetworkMgr->GetSockOutUBuff());

    // Allocate the buffer.
    ACE_NEW_RETURN(_outBuffer, ACE_Message_Block(_outBufferSize), -1);

    // Store peer address.
    ACE_INET_Addr remote_addr;

    if (peer().get_remote_addr(remote_addr) == -1)
    {
        sLog->outError("Socket::open: peer().get_remote_addr errno = %s", ACE_OS::strerror(errno));
        return -1;
    }

    _address = remote_addr.get_host_addr();

    return 0;
}

int Socket::open (void *a)
{
    ACE_UNUSED_ARG (a);

    if (this->_setup() == -1)
        return -1;

    // Register with ACE Reactor
    if (reactor()->register_handler(this, ACE_Event_Handler::READ_MASK | ACE_Event_Handler::WRITE_MASK) == -1)
    {
        sLog->outError("Socket::open: unable to register client handler errno = %s", ACE_OS::strerror (errno));
        return -1;
    }

    remove_reference();

    return 0;
}

int Socket::cancel_wakeup_output()
{
    if (reactor()->cancel_wakeup(this, ACE_Event_Handler::WRITE_MASK) == -1)
    {
        // would be good to store errno from reactor with errno guard
        sLog->outError("Socket::cancel_wakeup_output [%s]", ACE_OS::strerror(errno));
        return -1;
    }

    return 0;
}

int Socket::schedule_wakeup_output()
{
    if (reactor()->schedule_wakeup(this, ACE_Event_Handler::WRITE_MASK) == -1)
    {
        sLog->outError("Socket::schedule_wakeup_output [%s]", ACE_OS::strerror(errno));
        return -1;
    }

    return 0;
}

int Socket::handle_output_queue(GuardType &g)
{
    if (msg_queue()->is_empty())
        return cancel_wakeup_output();

    ACE_Message_Block *mblk;

    if (msg_queue()->dequeue_head(mblk, (ACE_Time_Value*) &ACE_Time_Value::zero) == -1)
    {
        sLog->outError("Socket::handle_output_queue dequeue_head");
        return -1;
    }

    const size_t send_len = mblk->length();

#ifdef MSG_NOSIGNAL
    ssize_t n = peer().send(mblk->rd_ptr(), send_len, MSG_NOSIGNAL);
#else
    ssize_t n = peer().send(mblk->rd_ptr(), send_len);
#endif // MSG_NOSIGNAL

    if (n == 0)
    {
        sLog->outError("Socket::handle_output_queue enqueue_head");
        mblk->release();
        return -1;
    }
    else if (n == -1)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
            msg_queue()->enqueue_head(mblk, (ACE_Time_Value*) &ACE_Time_Value::zero);
            return schedule_wakeup_output();
        }

        sLog->outError("Socket::handle_output_queue enqueue_head");
        mblk->release();
        return -1;
    }
    else if (n < (ssize_t)send_len) //now n > 0
    {
        mblk->rd_ptr(static_cast<size_t> (n));

        if (msg_queue()->enqueue_head(mblk, (ACE_Time_Value*) &ACE_Time_Value::zero) == -1)
        {
            sLog->outError("Socket::handle_output_queue enqueue_head");
            mblk->release();
            return -1;
        }

        return schedule_wakeup_output();
    }
    else //now n == send_len
    {
        mblk->release();

        return msg_queue()->is_empty() ? cancel_wakeup_output() : ACE_Event_Handler::WRITE_MASK;
    }

    ACE_NOTREACHED(return -1);
}

int Socket::handle_output(ACE_HANDLE)
{
    ACE_GUARD_RETURN(LockType, Guard, _outBufferLock, -1);

    if (IsClosing())
    {
        sLog->outError("Socket::handle_output IsClosing()");
        return -1;
    }

    size_t send_len = _outBuffer->length();

    if (send_len == 0)
        return handle_output_queue(Guard);

#ifdef MSG_NOSIGNAL
    ssize_t n = peer().send(_outBuffer->rd_ptr(), send_len, MSG_NOSIGNAL);
#else
    ssize_t n = peer().send(_outBuffer->rd_ptr(), send_len);
#endif // MSG_NOSIGNAL

    if (n == 0)
        return -1;
    else if (n == -1)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
            return schedule_wakeup_output();

        return -1;
    }
    else if (n < (ssize_t)send_len) //now n > 0
    {
        _outBuffer->rd_ptr(static_cast<size_t> (n));

        // move the data to the base of the buffer
        _outBuffer->crunch();

        return schedule_wakeup_output();
    }
    else //now n == send_len
    {
        _outBuffer->reset();

        return handle_output_queue(Guard);
    }

    ACE_NOTREACHED(return 0);
}

int Socket::handle_input_payload()
{
    // set errno properly here on error !!!
    // now have a header and payload

    ACE_ASSERT(_recvPct.space() == 0);
    ACE_ASSERT(_header.space() == 0);
    ACE_ASSERT(_recvWPct != NULL);

    const int ret = _processIncoming(_recvWPct);

    _recvPct.base(NULL, 0);
    _recvPct.reset();
    _recvWPct = NULL;

    _header.reset();

    if (ret == -1)
        errno = EINVAL;

    return ret;
}

int Socket::handle_input_missing_data()
{
    char buf [4096];

    ACE_Data_Block db(sizeof(buf),
                        ACE_Message_Block::MB_DATA,
                        buf,
                        0,
                        0,
                        ACE_Message_Block::DONT_DELETE,
                        0);

    ACE_Message_Block message_block(&db, ACE_Message_Block::DONT_DELETE, 0);

    const size_t recv_size = message_block.space();

    const ssize_t n = peer().recv(message_block.wr_ptr(), recv_size);

    if (n <= 0)
        return int(n);

    message_block.wr_ptr (n);

    while (message_block.length() > 0)
    {
        if (_header.space() > 0)
        {
            //need to receive the header
            const size_t to_header = (message_block.length() > _header.space() ? _header.space() : message_block.length());
            _header.copy(message_block.rd_ptr(), to_header);
            message_block.rd_ptr(to_header);

            if (_header.space() > 0)
            {
                // Couldn't receive the whole header this time.
                ACE_ASSERT(message_block.length() == 0);
                errno = EWOULDBLOCK;
                return -1;
            }

            // We just received nice new header
            if (_handleInputHeader() == -1)
            {
                ACE_ASSERT((errno != EWOULDBLOCK) && (errno != EAGAIN));
                return -1;
            }
        }

        // Its possible on some error situations that this happens
        // for example on closing when epoll receives more chunked data and stuff
        // hope this is not hack, as proper m_RecvWPct is asserted around
        if (!_recvWPct)
        {
            sLog->outError("Forcing close on input m_RecvWPct = NULL");
            errno = EINVAL;
            return -1;
        }

        // We have full read header, now check the data payload
        if (_recvPct.space() > 0)
        {
            //need more data in the payload
            const size_t to_data = (message_block.length() > _recvPct.space() ? _recvPct.space() : message_block.length());
            _recvPct.copy(message_block.rd_ptr(), to_data);
            message_block.rd_ptr(to_data);

            if (_recvPct.space() > 0)
            {
                // Couldn't receive the whole data this time.
                ACE_ASSERT(message_block.length() == 0);
                errno = EWOULDBLOCK;
                return -1;
            }
        }

        //just received fresh new payload
        if (handle_input_payload() == -1)
        {
            ACE_ASSERT((errno != EWOULDBLOCK) && (errno != EAGAIN));
            return -1;
        }
    }

    return size_t(n) == recv_size ? 1 : 2;
}

int Socket::handle_input(ACE_HANDLE)
{
    if (IsClosing())
    {
        sLog->outError("Socket::handle_input IsClosing()");
        return -1;
    }

    switch (handle_input_missing_data())
    {
        case -1 :
        {
            if ((errno == EWOULDBLOCK) || (errno == EAGAIN))
                return IsClosing() ? -1 : 0;

            sLog->outString("Socket::handle_input: Peer error closing connection errno = %s", ACE_OS::strerror(errno));

            errno = ECONNRESET;
            return -1;
        }
        case 0:
        {
            sLog->outString("Socket::handle_input: Peer has closed connection");

            errno = ECONNRESET;
            return -1;
        }
        case 1:
            return 1;
        default:
            return IsClosing() ? -1 : 0;
    }

    ACE_NOTREACHED(return -1);
}

int Socket::handle_close(ACE_HANDLE handle, ACE_Reactor_Mask mask)
{
    if (IsClosing())
        return 0;

    _closing = true;

    mask = ACE_Event_Handler::ALL_EVENTS_MASK | ACE_Event_Handler::DONT_CALL;

    reactor()->remove_handler(this, mask);

    return ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>::handle_close(handle, mask);
}
