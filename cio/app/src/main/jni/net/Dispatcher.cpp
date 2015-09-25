//
//  Dispatcher.cpp
//  cio
//
//  Created by chenjianjun on 15/9/22.
//  Copyright © 2015年 cio. All rights reserved.
//

#include "Dispatcher.h"
#include "../base/CriticalSection.h"

namespace NAME_SPACE {
    
    EventDispatcher::EventDispatcher():fSignaled_(false)
    {
        if (pipe(afd_) < 0)
        {
            printf("pipe failed");
        }
    }
    
    EventDispatcher::~EventDispatcher()
    {
        close(afd_[0]);
        close(afd_[1]);
    }
    
    void EventDispatcher::Signal()
    {
        CriticalSectionScoped cs(&crit_);
        
        if (!fSignaled_)
        {
            const uint8 b[1] = { 0 };
            if (write(afd_[1], b, sizeof(b)) == 1)
            {
                fSignaled_ = true;
            }
        }
    }
    
    void EventDispatcher::OnPreEvent(uint32 ff)
    {
        CriticalSectionScoped cs(&crit_);
        
        if (fSignaled_)
        {
            uint8 b[4];
            read(afd_[0], b, sizeof(b));
            fSignaled_ = false;
        }
    }
    
    Signaler::Signaler(bool* pf):pf_(pf) {}
    
    Signaler::~Signaler() {}
    
    void Signaler::OnEvent(uint32 ff, int err)
    {
        if (pf_)
        {
            *pf_ = false;
        }
    }
    
    PosixSignalDispatcher::PosixSignalHandler* PosixSignalDispatcher::PosixSignalHandler::Instance()
    {
        LIBJINGLE_DEFINE_STATIC_LOCAL(PosixSignalHandler, instance, ());
        return &instance;
    }
    
    bool PosixSignalDispatcher::PosixSignalHandler::IsSignalSet(int signum) const
    {
        if (signum < ARRAY_SIZE(received_signal_)) {
            return received_signal_[signum];
        } else {
            return false;
        }
    }
    
    void PosixSignalDispatcher::PosixSignalHandler::ClearSignal(int signum)
    {
        if (signum < ARRAY_SIZE(received_signal_))
        {
            // 清除信号标志
            received_signal_[signum] = false;
        }
    }
    
    int PosixSignalDispatcher::PosixSignalHandler::GetDescriptor() const
    {
        return afd_[0];
    }
    
    void PosixSignalDispatcher::PosixSignalHandler::OnPosixSignalReceived(int signum)
    {
        if (signum >= ARRAY_SIZE(received_signal_))
        {
            return;
        }
        
        // 设置信号标志.
        received_signal_[signum] = true;
        
        // 通知应用程序代码，我们得到一个信号
        const uint8 b[1] = { 0 };
        if (-1 == write(afd_[1], b, sizeof(b)))
        {
            return;
        }
    }
    
    
    PosixSignalDispatcher::PosixSignalHandler::PosixSignalHandler()
    {
        if (pipe(afd_) < 0)
        {
            return;
        }
        
        // 设置为非阻塞
        fcntl(afd_[0], F_SETFL, O_NONBLOCK);
        fcntl(afd_[1], F_SETFL, O_NONBLOCK);
        
        ::memset(const_cast<void *>(static_cast<volatile void *>(received_signal_)),0,sizeof(received_signal_));
    }
    
    PosixSignalDispatcher::PosixSignalHandler::~PosixSignalHandler()
    {
        int fd1 = afd_[0];
        int fd2 = afd_[1];
        afd_[0] = -1;
        afd_[1] = -1;
        close(fd1);
        close(fd2);
    }
    
    PosixSignalDispatcher::PosixSignalDispatcher() {}
    
    PosixSignalDispatcher::~PosixSignalDispatcher() {}
    
    void PosixSignalDispatcher::OnPreEvent(uint32 ff)
    {
        // 读取的字节多一点，确保管道里面是空的
        uint8 b[16];
        ssize_t ret = read(GetDescriptor(), b, sizeof(b));
        if (ret < 0)
        {
            printf("Error in read()");
        }
        else if (ret == 0)
        {
            printf("Should have read at least one byte");
        }
    }
    
    void PosixSignalDispatcher::OnEvent(uint32 ff, int err)
    {
        for (int signum = 0; signum < PosixSignalHandler::kNumPosixSignals; ++signum)
        {
            if (PosixSignalHandler::Instance()->IsSignalSet(signum))
            {
                PosixSignalHandler::Instance()->ClearSignal(signum);
                HandlerMap::iterator i = handlers_.find(signum);
                if (i == handlers_.end())
                {
                    // 如果没有设置信号捕捉，发生未知信号时，程序能知道
                    printf("Received signal with no handler: %d", signum);
                }
                else
                {
                    (*i->second)(signum);
                }
            }
        }
    }
    
    int PosixSignalDispatcher::GetDescriptor()
    {
        return PosixSignalHandler::Instance()->GetDescriptor();
    }
    
    void PosixSignalDispatcher::SetHandler(int signum, void (*handler)(int))
    {
        handlers_[signum] = handler;
    }
    
    void PosixSignalDispatcher::ClearHandler(int signum)
    {
        handlers_.erase(signum);
    }
    
    bool PosixSignalDispatcher::HasHandlers()
    {
        return !handlers_.empty();
    }
}
