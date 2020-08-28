#ifndef __SOCKET_H_
#define __SOCKET_H_

#include <ace/Basic_Types.h>
#include <ace/Synch_Traits.h>
#include <ace/SOCK_Stream.h>
#include <ace/Thread_Mutex.h>
#include <ace/Guard_T.h>
#include <ace/Message_Block.h>
#include <ace/Svc_Handler.h>
#include <ace/Unbounded_Queue.h>

#include "Log.h"
#include "WorldPacket.h"

struct ReactorInfo;

typedef ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH> OSSocket;

class Socket : public OSSocket
{
public:
    typedef ACE_Thread_Mutex LockType;
    typedef ACE_Guard<LockType> GuardType;

    Socket(uint32 ClientHeaderSize);
    virtual ~Socket();

    bool IsClosing() const;
    const std::string& GetRemoteAddress() const;

    int SendPacket(const WorldPacket &pct);

    bool GetNextSinglePacket(WorldPacket* &packet);
    bool GetNextMultiPacket(WorldPacket* &packet);

    // Zeug der internen Verwaltung
    virtual int open(void *a) override;
    virtual int handle_output(ACE_HANDLE = ACE_INVALID_HANDLE);
    virtual int handle_input(ACE_HANDLE = ACE_INVALID_HANDLE);
    virtual int handle_close(ACE_HANDLE = ACE_INVALID_HANDLE,
        ACE_Reactor_Mask = ACE_Event_Handler::ALL_EVENTS_MASK);

protected:
    ACE_Based::LockedQueue<WorldPacket*, ACE_RW_Mutex> _singlePacketQueue;
    ACE_Based::LockedQueue<WorldPacket*, ACE_RW_Mutex> _multiPacketQueue;

    LockType _outBufferLock;
    size_t _outBufferSize;
    ACE_Message_Block *_outBuffer;

    const uint32 _clientHeaderSize;
    ACE_Message_Block _header;
    WorldPacket *_recvWPct;
    ACE_Message_Block _recvPct;

    int _setup();
    int _directlySendPacket(const WorldPacket &pct);

private:
    virtual int _sendPacket(const WorldPacket &pct) = 0;
    virtual int _handleInputHeader() = 0;
    virtual int _processIncoming(WorldPacket *new_pct) = 0;

    int cancel_wakeup_output();
    int schedule_wakeup_output();
    int handle_output_queue(GuardType &g);

    int handle_input_payload();
    int handle_input_missing_data();

    bool _closing;
    std::string _address;
};

#endif /* __SOCKET_H_ */
