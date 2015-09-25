//
//  PhysicalSocketServer.cpp
//  cio
//
//  Created by chenjianjun on 15/9/22.
//  Copyright © 2015年 cio. All rights reserved.
//

#include "PhysicalSocketServer.h"

namespace NAME_SPACE {
    
    PhysicalSocketServer::PhysicalSocketServer():fWait_(false),signal_dispatcher_(nullptr)
    {
        signal_wakeup_ = new Signaler(&fWait_);
        Add(signal_wakeup_);
    }
    
    PhysicalSocketServer::~PhysicalSocketServer()
    {
        Remove(signal_wakeup_);
        delete signal_wakeup_;
        
        if (signal_dispatcher_)
        {
            Remove(signal_dispatcher_);
            delete signal_dispatcher_;
            signal_dispatcher_ = nullptr;
        }
    }
    
    void PhysicalSocketServer::WakeUp()
    {
        signal_wakeup_->Signal();
    }
    
    Socket* PhysicalSocketServer::CreateSocket(int type) {
        return CreateSocket(AF_INET, type);
    }
    
    Socket* PhysicalSocketServer::CreateSocket(int family, int type)
    {
        PhysicalSocket* socket = new PhysicalSocket();
        if (socket->Create(family, type))
        {
            return socket;
        } else {
            delete socket;
            return 0;
        }
    }
    
    SocketDispatcher* PhysicalSocketServer::CreateAsyncSocket(int type)
    {
        return CreateAsyncSocket(AF_INET, type);
    }
    
    SocketDispatcher* PhysicalSocketServer::CreateAsyncSocket(int family, int type)
    {
        SocketDispatcher* dispatcher = new SocketDispatcher();
        if (dispatcher->Create(family, type))
        {
            return dispatcher;
        }
        else
        {
            delete dispatcher;
            return nullptr;
        }
    }
    
    SocketDispatcher* PhysicalSocketServer::WrapSocket(SOCKET s)
    {
        SocketDispatcher* dispatcher = new SocketDispatcher(s);
        if (dispatcher->Initialize())
        {
            return dispatcher;
        }
        else
        {
            delete dispatcher;
            return nullptr;
        }
    }
    
    void PhysicalSocketServer::Add(Dispatcher *pdispatcher)
    {
        CriticalSectionScoped cs(&crit_);
        // Prevent duplicates. This can cause dead dispatchers to stick around.
        DispatcherList::iterator pos = std::find(dispatchers_.begin(),
                                                 dispatchers_.end(),
                                                 pdispatcher);
        if (pos != dispatchers_.end())
            return;
        
        dispatchers_.push_back(pdispatcher);
    }
    
    void PhysicalSocketServer::Remove(Dispatcher *pdispatcher)
    {
        CriticalSectionScoped cs(&crit_);
        DispatcherList::iterator pos = std::find(dispatchers_.begin(),
                                                 dispatchers_.end(),
                                                 pdispatcher);
        // We silently ignore duplicate calls to Add, so we should silently ignore
        // the (expected) symmetric calls to Remove. Note that this may still hide
        // a real issue, so we at least log a warning about it.
        if (pos == dispatchers_.end()) {
            return;
        }
        
        size_t index = pos - dispatchers_.begin();
        dispatchers_.erase(pos);
        for (IteratorList::iterator it = iterators_.begin(); it != iterators_.end(); ++it)
        {
            if (index < **it)
            {
                --**it;
            }
        }
    }
    
    bool PhysicalSocketServer::Wait(int cmsWait, bool process_io)
    {
        // Calculate timing information
        
        struct timeval *ptvWait = NULL;
        struct timeval tvWait;
        struct timeval tvStop;
        if (cmsWait > 0)
        {
            // Calculate wait timeval
            tvWait.tv_sec = cmsWait / 1000;
            tvWait.tv_usec = (cmsWait % 1000) * 1000;
            ptvWait = &tvWait;
            
            // Calculate when to return in a timeval
            gettimeofday(&tvStop, NULL);
            tvStop.tv_sec += tvWait.tv_sec;
            tvStop.tv_usec += tvWait.tv_usec;
            if (tvStop.tv_usec >= 1000000) {
                tvStop.tv_usec -= 1000000;
                tvStop.tv_sec += 1;
            }
        }
        
        // Zero all fd_sets. Don't need to do this inside the loop since
        // select() zeros the descriptors not signaled
        
        fd_set fdsRead;
        FD_ZERO(&fdsRead);
        fd_set fdsWrite;
        FD_ZERO(&fdsWrite);
        
        fWait_ = true;
        while (fWait_)
        {
            int fdmax = -1;
            {
                CriticalSectionScoped cr(&crit_);
                for (size_t i = 0; i < dispatchers_.size(); ++i)
                {
                    // Query dispatchers for read and write wait state
                    Dispatcher *pdispatcher = dispatchers_[i];
                    if (!process_io && (pdispatcher != signal_wakeup_))
                        continue;
                    int fd = pdispatcher->GetDescriptor();
                    if (fd > fdmax)
                        fdmax = fd;
                    
                    uint32 ff = pdispatcher->GetRequestedEvents();
                    if (ff & (DE_READ | DE_ACCEPT))
                        FD_SET(fd, &fdsRead);
                    if (ff & (DE_WRITE | DE_CONNECT))
                        FD_SET(fd, &fdsWrite);
                }
            }
            
            // Wait then call handlers as appropriate
            // < 0 means error
            // 0 means timeout
            // > 0 means count of descriptors ready
            int n = select(fdmax + 1, &fdsRead, &fdsWrite, NULL, ptvWait);
            
            // If error, return error.
            if (n < 0)
            {
                if (errno != EINTR)
                {
                    printf("select error. %d.\n", errno);
                    return false;
                }
                // Else ignore the error and keep going. If this EINTR was for one of the
                // signals managed by this PhysicalSocketServer, the
                // PosixSignalDeliveryDispatcher will be in the signaled state in the next
                // iteration.
            }
            else if (n == 0)
            {
                // If timeout, return success
                return true;
            }
            else
            {
                // We have signaled descriptors
                CriticalSectionScoped cr(&crit_);
                for (size_t i = 0; i < dispatchers_.size(); ++i)
                {
                    Dispatcher *pdispatcher = dispatchers_[i];
                    int fd = pdispatcher->GetDescriptor();
                    uint32 ff = 0;
                    int errcode = 0;
                    
                    // Reap any error code, which can be signaled through reads or writes.
                    // TODO: Should we set errcode if getsockopt fails?
                    if (FD_ISSET(fd, &fdsRead) || FD_ISSET(fd, &fdsWrite)) {
                        socklen_t len = sizeof(errcode);
                        ::getsockopt(fd, SOL_SOCKET, SO_ERROR, &errcode, &len);
                    }
                    
                    // Check readable descriptors. If we're waiting on an accept, signal
                    // that. Otherwise we're waiting for data, check to see if we're
                    // readable or really closed.
                    // TODO: Only peek at TCP descriptors.
                    if (FD_ISSET(fd, &fdsRead))
                    {
                        FD_CLR(fd, &fdsRead);
                        
                        if (pdispatcher->GetRequestedEvents() & DE_ACCEPT)
                        {
                            ff |= DE_ACCEPT;
                        }
                        else if (errcode || pdispatcher->IsDescriptorClosed())
                        {
                            ff |= DE_CLOSE;
                        }
                        else
                        {
                            ff |= DE_READ;
                        }
                    }
                    
                    // Check writable descriptors. If we're waiting on a connect, detect
                    // success versus failure by the reaped error code.
                    if (FD_ISSET(fd, &fdsWrite))
                    {
                        FD_CLR(fd, &fdsWrite);
                        
                        if (pdispatcher->GetRequestedEvents() & DE_CONNECT)
                        {
                            if (!errcode)
                            {
                                ff |= DE_CONNECT;
                            }
                            else
                            {
                                ff |= DE_CLOSE;
                            }
                        }
                        else
                        {
                            ff |= DE_WRITE;
                        }
                    }
                    
                    // Tell the descriptor about the event.
                    if (ff != 0)
                    {
                        pdispatcher->OnPreEvent(ff);
                        pdispatcher->OnEvent(ff, errcode);
                    }
                }
            }
            
            // Recalc the time remaining to wait. Doing it here means it doesn't get
            // calced twice the first time through the loop
            if (ptvWait)
            {
                ptvWait->tv_sec = 0;
                ptvWait->tv_usec = 0;
                struct timeval tvT;
                gettimeofday(&tvT, NULL);
                if ((tvStop.tv_sec > tvT.tv_sec) || ((tvStop.tv_sec == tvT.tv_sec)&& (tvStop.tv_usec > tvT.tv_usec)))
                {
                    ptvWait->tv_sec = tvStop.tv_sec - tvT.tv_sec;
                    ptvWait->tv_usec = tvStop.tv_usec - tvT.tv_usec;
                    if (ptvWait->tv_usec < 0)
                    {
                        ptvWait->tv_usec += 1000000;
                        ptvWait->tv_sec -= 1;
                    }
                }
            }
        }
        
        return true;
    }
    
    void PhysicalSocketServer::GlobalSignalHandler(int signum)
    {
        PosixSignalDispatcher::PosixSignalHandler::Instance()->OnPosixSignalReceived(signum);
    }
    
    bool PhysicalSocketServer::SetPosixSignalHandler(int signum, void (*handler)(int))
    {
        // If handler is SIG_IGN or SIG_DFL then clear our user-level handler,
        // otherwise set one.
        if (handler == SIG_IGN || handler == SIG_DFL)
        {
            if (!InstallSignal(signum, handler))
            {
                return false;
            }
            if (signal_dispatcher_)
            {
                signal_dispatcher_->ClearHandler(signum);
                if (!signal_dispatcher_->HasHandlers())
                {
                    Remove(signal_dispatcher_);
                    delete signal_dispatcher_;
                    signal_dispatcher_ = nullptr;
                }
            }
        }
        else
        {
            if (!signal_dispatcher_) {
                signal_dispatcher_ = new PosixSignalDispatcher();
                Add(signal_dispatcher_);
            }
            signal_dispatcher_->SetHandler(signum, handler);
            if (!InstallSignal(signum, &GlobalSignalHandler)) {
                return false;
            }
        }
        
        return true;
    }
    
    Dispatcher* PhysicalSocketServer::signal_dispatcher() {
        return signal_dispatcher_;
    }
    
    bool PhysicalSocketServer::InstallSignal(int signum, void (*handler)(int))
    {
        struct sigaction act;
        // It doesn't really matter what we set this mask to.
        if (sigemptyset(&act.sa_mask) != 0)
        {
            return false;
        }
        
        act.sa_handler = handler;
        act.sa_flags = 0;

        if (sigaction(signum, &act, NULL) != 0) {
            printf("Couldn't set sigaction.\n");
            return false;
        }
        
        return true;
    }
}
