#ifndef __SOCKETCONNECTOR_HPP
#define __SOCKETCONNECTOR_HPP

#include <ace/Connector.h>
#include <ace/SOCK_Connector.h>

#include "Network.h"

template <class SocketType>
class SocketConnector
{
public:
    SocketConnector()
    {
        m_Connector = new ACE_Connector<SocketType, ACE_SOCK_Connector>;
    }

    ~SocketConnector()
    {
        delete m_Connector;
    }

    int open()
    {
        return m_Connector->open(sNetworkMgr->reactor(), ACE_NONBLOCK);
    }

    int close()
    {
        return m_Connector->close();
    }

    virtual SocketType * OpenConnection(uint16 port, std::string IP)
    {
        SocketType *sock = new SocketType();
        ACE_INET_Addr port_to_connect(port, IP.c_str());

        if (m_Connector->connect(sock, port_to_connect) < 0)
        {
            delete sock;
            return NULL;
        }

        return sock;
    }

private:
    ACE_Connector<SocketType, ACE_SOCK_Connector> *m_Connector;
};

#endif /* __SOCKETCONNECTOR_HPP */
