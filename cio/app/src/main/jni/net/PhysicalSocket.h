//
//  PhysicalSocket.h
//  cio
//
//  Created by chenjianjun on 15/9/23.
//  Copyright © 2015年 cio. All rights reserved.
//

#ifndef __PhysicalSocket_h_
#define __PhysicalSocket_h_

#include "../base/Common.h"
#include "./AsyncSocket.h"
#include "../base/Sigslot.h"
#include "../base/CriticalSection.h"

namespace NAME_SPACE {
    
    class PhysicalSocket : public AsyncSocket
    {
    public:
        PhysicalSocket(SOCKET s = INVALID_SOCKET);
        ~PhysicalSocket() override;
        virtual bool Create(int family, int type);
        SocketAddress GetLocalAddress() const override;
        SocketAddress GetRemoteAddress() const override;
        int Bind(const SocketAddress& bind_addr) override;
        int Connect(const SocketAddress& addr) override;
        int DoConnect(const SocketAddress& connect_addr);
        int GetError() const override;
        void SetError(int error) override;
        ConnState GetState() const override { return state_; }
        int GetOption(Option opt, int* value) override;
        int SetOption(Option opt, int value) override;
        int Send(const void* pv, size_t cb) override;
        int SendTo(const void* buffer, size_t length, const SocketAddress& addr) override;
        int Recv(void* buffer, size_t length) override;
        int RecvFrom(void* buffer, size_t length, SocketAddress* out_addr) override;
        int Listen(int backlog) override;
        AsyncSocket* Accept(SocketAddress* out_addr) override;
        int Close() override;
        int EstimateMTU(uint16* mtu) override;
        
    protected:
        static int TranslateOption(Option opt, int* slevel, int* sopt);
        void UpdateLastError() { SetError(LAST_SYSTEM_ERROR); }
        void MaybeRemapSendError();
        
        SOCKET s_;
        uint8 enabled_events_;
        bool udp_;
        int error_;
        // Protects |error_| that is accessed from different threads.
        mutable CriticalSection crit_;
        ConnState state_;
    };
    
}

#endif /* __PhysicalSocket_h_ */
