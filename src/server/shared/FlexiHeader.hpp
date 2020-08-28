#ifndef __FLEXIHEADER_HPP
#define __FLEXIHEADER_HPP

#pragma once

#include <ace/Basic_Types.h>

namespace Flexi
{
    #if defined(__GNUC__)
        #pragma pack(1)
    #else
        #pragma pack(push, 1)
    #endif

    struct ServerPktHeader
    {
        ServerPktHeader(ACE_UINT32 size, ACE_UINT16 cmd) : m_size(size)
        {
            ACE_UINT8 headerIndex = 0;

            header[headerIndex++] = 0xFF & (m_size>>24);
            header[headerIndex++] = 0xFF & (m_size>>16);
            header[headerIndex++] = 0xFF & (m_size>>8);
            header[headerIndex++] = 0xFF & m_size;

            header[headerIndex++] = 0xFF & cmd;
            header[headerIndex++] = 0xFF & (cmd>>8);
        }

        ACE_UINT8 getHeaderLength()
        {
            // cmd = 2 bytes, size= 4 bytes
            return 6;
        }

        const ACE_UINT32 m_size;
        ACE_UINT8 header[6];
    };

    struct ClientPktHeader
    {
        ACE_UINT32 size;
        ACE_UINT16 cmd;
    };

    #if defined(__GNUC__)
        #pragma pack()
    #else
        #pragma pack(pop)
    #endif

}

#endif /* __FLEXIHEADER_HPP */
