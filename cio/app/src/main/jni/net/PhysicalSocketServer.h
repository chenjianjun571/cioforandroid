//
//  PhysicalSocketServer.hpp
//  cio
//
//  Created by chenjianjun on 15/9/22.
//  Copyright © 2015年 cio. All rights reserved.
//

#ifndef __PhysicalSocketServer_h_
#define __PhysicalSocketServer_h_

#include <vector>

#include "../base/Common.h"
#include "SocketDispatcher.h"

namespace NAME_SPACE {
    
    class PhysicalSocketServer
    {
    public:
        PhysicalSocketServer();
        ~PhysicalSocketServer();
        
        Socket* CreateSocket(int type);
        Socket* CreateSocket(int family, int type);
        
        SocketDispatcher* CreateAsyncSocket(int type);
        SocketDispatcher* CreateAsyncSocket(int family, int type);
        
        // Internal Factory for Accept
        SocketDispatcher* WrapSocket(SOCKET s);
        
        bool Wait(int cms, bool process_io);
        void WakeUp();
        
        void Add(Dispatcher* dispatcher);
        void Remove(Dispatcher* dispatcher);
        
        virtual bool SetPosixSignalHandler(int signum, void (*handler)(int));
        
    protected:
        Dispatcher* signal_dispatcher();
        static void GlobalSignalHandler(int signum);
        static bool InstallSignal(int signum, void (*handler)(int));
        
    private:
        typedef std::vector<Dispatcher*> DispatcherList;
        typedef std::vector<size_t*> IteratorList;
        
        PosixSignalDispatcher* signal_dispatcher_;
        DispatcherList dispatchers_;
        IteratorList iterators_;
        Signaler* signal_wakeup_;
        CriticalSection crit_;
        bool fWait_;
    };
    
}

#endif /* __PhysicalSocketServer_h_ */
