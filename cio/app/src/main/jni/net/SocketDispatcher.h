//
//  SocketDispatcher.h
//  cio
//
//  Created by chenjianjun on 15/9/23.
//  Copyright © 2015年 cio. All rights reserved.
//

#ifndef __SocketDispatcher_h_
#define __SocketDispatcher_h_

#include "../base/Common.h"
#include "./Dispatcher.h"
#include "./PhysicalSocket.h"

namespace NAME_SPACE {
    
    class SocketDispatcher : public Dispatcher, public PhysicalSocket
    {
    public:
        SocketDispatcher();
        SocketDispatcher(SOCKET s);
        ~SocketDispatcher() override;
        
        bool Initialize();
        virtual bool Create(int type);
        bool Create(int family, int type) override;
        
        int GetDescriptor() override { return s_; }
        uint32 GetRequestedEvents() override { return enabled_events_; }
        bool IsDescriptorClosed() override;
        void OnPreEvent(uint32 ff) override;
        void OnEvent(uint32 ff, int err) override;
        int Close() override;
    };
    
}

#endif /* __SocketDispatcher_h_ */
