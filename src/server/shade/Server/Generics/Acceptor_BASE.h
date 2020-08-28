#ifndef __ACCEPTOR_BASE_HPP
#define __ACCEPTOR_BASE_HPP

#pragma once

#include <ace/Acceptor.h>
#include <ace/SOCK_Acceptor.h>

#include "Log.h"

template <class SocketType>
class Acceptor_BASE : public ACE_Acceptor<SocketType, ACE_SOCK_Acceptor>
{
public:
    Acceptor_BASE()
    {
        ;
    }

    virtual ~Acceptor_BASE()
    {
        if (this->reactor())
            this->reactor()->cancel_timer(this, 1);
    }

protected:
    virtual int handle_timeout(const ACE_Time_Value& /*current_time*/, const void* /*act = 0*/)
    {
        sLog->outString("Resuming acceptor");
        this->reactor()->cancel_timer(this, 1);
        return this->reactor()->register_handler(this, ACE_Event_Handler::ACCEPT_MASK);
    }

    virtual int handle_accept_error()
    {
#if defined(ENFILE) && defined(EMFILE)
        if (errno == ENFILE || errno == EMFILE)
        {
            sLog->outError("Out of file descriptors, suspending incoming connections for 10 seconds");
            this->reactor()->remove_handler(this, ACE_Event_Handler::ACCEPT_MASK | ACE_Event_Handler::DONT_CALL);
            this->reactor()->schedule_timer(this, NULL, ACE_Time_Value(10));
        }
#endif
        return 0;
    }
};

#endif /* __ACCEPTOR_BASE_HPP */
