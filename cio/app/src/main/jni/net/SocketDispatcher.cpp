//
//  SocketDispatcher.cpp
//  cio
//
//  Created by chenjianjun on 15/9/23.
//  Copyright © 2015年 cio. All rights reserved.
//

#include "SocketDispatcher.h"

namespace NAME_SPACE {

    SocketDispatcher::SocketDispatcher(SOCKET s):PhysicalSocket(s)
    {}
    
    SocketDispatcher::SocketDispatcher()
    {}
    
    SocketDispatcher::~SocketDispatcher()
    {
        Close();
    }
    
    bool SocketDispatcher::Initialize()
    {
        fcntl(s_, F_SETFL, fcntl(s_, F_GETFL, 0) | O_NONBLOCK);
        return true;
    }
    
    bool SocketDispatcher::Create(int type)
    {
        return Create(AF_INET, type);
    }
    
    bool SocketDispatcher::Create(int family, int type)
    {
        // Change the socket to be non-blocking.
        if (!PhysicalSocket::Create(family, type))
            return false;
        
        return Initialize();
    }
    
    bool SocketDispatcher::IsDescriptorClosed()
    {
        // We don't have a reliable way of distinguishing end-of-stream
        // from readability.  So test on each readable call.  Is this
        // inefficient?  Probably.
        char ch;
        ssize_t res = ::recv(s_, &ch, 1, MSG_PEEK);
        if (res > 0)
        {
            // Data available, so not closed.
            return false;
        }
        else if (res == 0)
        {
            // EOF, so closed.
            return true;
        }
        else
        {  // error
            switch (errno) {
                    // Returned if we've already closed s_.
                case EBADF:
                    // Returned during ungraceful peer shutdown.
                case ECONNRESET:
                    return true;
                default:
                    // Assume that all other errors are just blocking errors, meaning the
                    // connection is still good but we just can't read from it right now.
                    // This should only happen when connecting (and at most once), because
                    // in all other cases this function is only called if the file
                    // descriptor is already known to be in the readable state. However,
                    // it's not necessary a problem if we spuriously interpret a
                    // "connection lost"-type error as a blocking error, because typically
                    // the next recv() will get EOF, so we'll still eventually notice that
                    // the socket is closed.
                    printf("Assuming benign blocking error.\n");
                    return false;
            }
        }
    }
    
    void SocketDispatcher::OnPreEvent(uint32 ff)
    {
        if ((ff & DE_CONNECT) != 0)
            state_ = CS_CONNECTED;
        if ((ff & DE_CLOSE) != 0)
            state_ = CS_CLOSED;
    }
    
    void SocketDispatcher::OnEvent(uint32 ff, int err)
    {
        // Make sure we deliver connect/accept first. Otherwise, consumers may see
        // something like a READ followed by a CONNECT, which would be odd.
        if ((ff & DE_CONNECT) != 0) {
            enabled_events_ &= ~DE_CONNECT;
            SignalConnectEvent(this);
        }
        if ((ff & DE_ACCEPT) != 0) {
            enabled_events_ &= ~DE_ACCEPT;
            SignalReadEvent(this);
        }
        if ((ff & DE_READ) != 0) {
            enabled_events_ &= ~DE_READ;
            SignalReadEvent(this);
        }
        if ((ff & DE_WRITE) != 0) {
            enabled_events_ &= ~DE_WRITE;
            SignalWriteEvent(this);
        }
        if ((ff & DE_CLOSE) != 0) {
            // The socket is now dead to us, so stop checking it.
            enabled_events_ = 0;
            SignalCloseEvent(this, err);
        }
    }
    
    int SocketDispatcher::Close()
    {
        if (s_ == INVALID_SOCKET)
            return 0;
        
        return PhysicalSocket::Close();
    }
    
}
