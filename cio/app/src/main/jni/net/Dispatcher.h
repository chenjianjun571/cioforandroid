//
//  Dispatcher.h
//  cio
//
//  Created by chenjianjun on 15/9/22.
//  Copyright © 2015年 cio. All rights reserved.
//

#ifndef __Dispatcher_h_
#define __Dispatcher_h_

#include <map>

#include "../base/Common.h"
#include "./SocketAddress.h"
#include "../base/CriticalSection.h"

// 标记了final的virtual函数，例如上面的B2::f，GNU C++ 前端会识别出，这个函数不能被覆盖，因此会将其从类的虚表中删除。而标记为final的类，例如上面的 B1，编译器则根本不会生成虚表
// 标记了override的函数，如果父类没有这个函数者会报错

namespace NAME_SPACE {
    
    // Event constants for the Dispatcher class.
    enum DispatcherEvent {
        DE_READ    = 0x0001,
        DE_WRITE   = 0x0002,
        DE_CONNECT = 0x0004,
        DE_CLOSE   = 0x0008,
        DE_ACCEPT  = 0x0010,
    };
    
    class Dispatcher
    {
    public:
        virtual ~Dispatcher() {}
        virtual uint32 GetRequestedEvents() = 0;
        virtual void OnPreEvent(uint32 ff) = 0;
        virtual void OnEvent(uint32 ff, int err) = 0;
        virtual int GetDescriptor() = 0;
        virtual bool IsDescriptorClosed() = 0;
    };
    
    class EventDispatcher:public Dispatcher
    {
    public:
        EventDispatcher();
        ~EventDispatcher();
        virtual void Signal();
        void OnPreEvent(uint32 ff) override;
        uint32 GetRequestedEvents() override { return DE_READ; }
        void OnEvent(uint32 ff, int err) override { }
        int GetDescriptor() override { return afd_[0]; }
        bool IsDescriptorClosed() override { return false; }
        
    private:
        int afd_[2];
        bool fSignaled_;
        CriticalSection crit_;
    };
    
    class Signaler:public EventDispatcher
    {
    public:
        explicit Signaler(bool* pf);
        ~Signaler() override;
        
        // 处理事件
        void OnEvent(uint32 ff, int err) override;
        
    private:
        bool *pf_;
    };
    
    class PosixSignalDispatcher:public Dispatcher
    {
    public:
        class PosixSignalHandler
        {
        public:
            // 虽然POSIX只指定了32个信号，但是系统允许大于这个数值，所以我们设置为128
            static const int kNumPosixSignals = 128;
            static PosixSignalHandler* Instance();
            bool IsSignalSet(int signum) const;
            void ClearSignal(int signum);
            int GetDescriptor() const;
            void OnPosixSignalReceived(int signum);
            
        private:
            PosixSignalHandler();
            ~PosixSignalHandler();
            
        private:
            int afd_[2];
            volatile uint8 received_signal_[kNumPosixSignals];
        };
    public:
        PosixSignalDispatcher();
        ~PosixSignalDispatcher() override;
        uint32 GetRequestedEvents() override { return DE_READ; }
        void OnPreEvent(uint32 ff) override;
        void OnEvent(uint32 ff, int err) override;
        int GetDescriptor() override;
        bool IsDescriptorClosed() override { return false; };
        void SetHandler(int signum, void (*handler)(int));
        void ClearHandler(int signum);
        bool HasHandlers();
        
    private:
        typedef std::map<int, void (*)(int)> HandlerMap;
        
        HandlerMap handlers_;
    };
}

#endif /* __Dispatcher_h_ */
