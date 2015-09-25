//
//  Socket.h
//  cio
//
//  Created by chenjianjun on 15/9/22.
//  Copyright © 2015年 cio. All rights reserved.
//

#ifndef __Socket_h_
#define __Socket_h_

#include "../base/Common.h"
#include "./SocketAddress.h"

namespace NAME_SPACE {
    
    inline bool IsBlockingError(int e)
    {
        return (e == EWOULDBLOCK) || (e == EAGAIN) || (e == EINPROGRESS);
    }
    
    class Socket
    {
    public:
        virtual ~Socket() {}
        
        virtual SocketAddress GetLocalAddress() const = 0;
        virtual SocketAddress GetRemoteAddress() const = 0;
        
        virtual int Bind(const SocketAddress& addr) = 0;
        virtual int Connect(const SocketAddress& addr) = 0;
        virtual int Send(const void *pv, size_t cb) = 0;
        virtual int SendTo(const void *pv, size_t cb, const SocketAddress& addr) = 0;
        virtual int Recv(void *pv, size_t cb) = 0;
        virtual int RecvFrom(void *pv, size_t cb, SocketAddress *paddr) = 0;
        virtual int Listen(int backlog) = 0;
        virtual Socket *Accept(SocketAddress *paddr) = 0;
        virtual int Close() = 0;
        virtual int GetError() const = 0;
        virtual void SetError(int error) = 0;
        inline bool IsBlocking() const { return IsBlockingError(GetError()); }
        
        enum ConnState {
            CS_CLOSED,
            CS_CONNECTING,
            CS_CONNECTED
        };
        virtual ConnState GetState() const = 0;
        
        virtual int EstimateMTU(uint16* mtu) = 0;
        
        enum Option {
            OPT_DONTFRAGMENT,
            OPT_RCVBUF,      // receive buffer size
            OPT_SNDBUF,      // send buffer size
            OPT_NODELAY,     // whether Nagle algorithm is enabled
            OPT_IPV6_V6ONLY, // Whether the socket is IPv6 only.
            OPT_DSCP,        // DSCP code
            OPT_RTP_SENDTIME_EXTN_ID,  // This is a non-traditional socket option param.
            // This is specific to libjingle and will be used
            // if SendTime option is needed at socket level.
        };
        virtual int GetOption(Option opt, int* value) = 0;
        virtual int SetOption(Option opt, int value) = 0;
        
    protected:
        Socket(){}
        
    private:
        DISALLOW_EVIL_CONSTRUCTORS(Socket);
    };
    
}

#endif /* __Socket_h_ */
