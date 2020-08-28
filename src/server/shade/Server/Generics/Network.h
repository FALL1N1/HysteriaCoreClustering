#ifndef __NETWORKMGR_HPP
#define __NETWORKMGR_HPP

#pragma once

#include <ace/Singleton.h>
#include <ace/Reactor.h>
#include <ace/Atomic_Op.h>

class Socket;

class NetworkMgr
{
public:
    friend class ACE_Singleton<NetworkMgr, ACE_Thread_Mutex>;

    int open();
    void close();
    void wait();

    int GetCompressionLevel() const;
    int GetSockOutKBuff() const;
    int GetSockOutUBuff() const;
    bool UseNoDelay() const;

    ACE_Reactor * reactor();

private:
    NetworkMgr();
    ~NetworkMgr();

    size_t _threadsCount;
    int _threadGroup;

    ACE_Reactor * _reactor;

    bool _useNoDelay;
    int _sockOutKBuff;
    int _sockOutUBuff;
    int _compressionLevel;
};

#define sNetworkMgr ACE_Singleton<NetworkMgr, ACE_Thread_Mutex>::instance()

#endif /* __NETWORKMGR_HPP */
