//
//  AsyncSocket.h
//  cio
//
//  Created by chenjianjun on 15/9/22.
//  Copyright © 2015年 cio. All rights reserved.
//

#ifndef __AsyncSocket_h_
#define __AsyncSocket_h_

#include "./Socket.h"
#include "../base/Sigslot.h"

namespace NAME_SPACE {
    
    class AsyncSocket:public Socket
    {
    public:
        AsyncSocket();
        ~AsyncSocket() override;
        
        AsyncSocket* Accept(SocketAddress* paddr) override = 0;
        
        // 读事件回调
        sigslot::signal1<AsyncSocket*, sigslot::multi_threaded_local> SignalReadEvent;
        // 写事件回调
        sigslot::signal1<AsyncSocket*, sigslot::multi_threaded_local> SignalWriteEvent;
        // 连接建立回调
        sigslot::signal1<AsyncSocket*> SignalConnectEvent;
        // 连接关闭回调
        sigslot::signal2<AsyncSocket*, int> SignalCloseEvent;
    };
    
}

#endif /* __AsyncSocket_h_ */
