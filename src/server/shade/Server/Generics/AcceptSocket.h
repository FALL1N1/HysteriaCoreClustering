#ifndef __ACCEPTSOCKET_HPP
#define __ACCEPTSOCKET_HPP

#include "Socket.h"
#include "Log.h"

class AcceptSocket : public Socket
{
public:
    AcceptSocket(uint32 ClientHeaderSize) : Socket(ClientHeaderSize), _authenticated(false)
    {
    }

    bool IsAuthenticated() const
    {
        return _authenticated;
    }

    virtual int open(void *a) override
    {
        ACE_UNUSED_ARG(a);

        if (this->_setup() == -1)
        {
            sLog->outError("AcceptSocket::open: unable to setup socket");
            return -1;
        }

        WorldPacket packet;
        _buildAuthChallengePacket(packet);

        if (this->_directlySendPacket(packet) == -1)
        {
            sLog->outError("AcceptSocket::open: unable to send auth packet");
            return -1;
        }

        // Register with ACE Reactor
        if (reactor()->register_handler(this, ACE_Event_Handler::READ_MASK | ACE_Event_Handler::WRITE_MASK) == -1)
        {
            sLog->outError("AcceptSocket::open: unable to register client handler errno = %s", ACE_OS::strerror(errno));
            return -1;
        }

        remove_reference();
        return 0;
    }

protected:
    void SetAuthenticated()
    {
        _authenticated = true;
    }

private:
    bool _authenticated;

    virtual void _buildAuthChallengePacket(WorldPacket &packet) = 0;
};

#endif /* __ACCEPTSOCKET_HPP */
