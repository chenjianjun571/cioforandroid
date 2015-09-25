//
//  PhysicalSocket.cpp
//  cio
//
//  Created by chenjianjun on 15/9/23.
//  Copyright © 2015年 cio. All rights reserved.
//

#include "PhysicalSocket.h"
#include "SocketDispatcher.h"

namespace NAME_SPACE
{
    PhysicalSocket::PhysicalSocket(SOCKET s)
    :s_(s),
    enabled_events_(0),
    error_(0),
    state_((s == INVALID_SOCKET) ? CS_CLOSED : CS_CONNECTED)
    {
        if (s_ != INVALID_SOCKET)
        {
            enabled_events_ = DE_READ | DE_WRITE;
            int type = SOCK_STREAM;
            socklen_t len = sizeof(type);
            
            VERIFY(0 == getsockopt(s_, SOL_SOCKET, SO_TYPE, (SockOptArg)&type, &len));
            udp_ = (SOCK_DGRAM == type);
        }
    }
    
    PhysicalSocket::~PhysicalSocket()
    {
        Close();
    }
    
    bool PhysicalSocket::Create(int family, int type)
    {
        Close();
        
        s_ = ::socket(family, type, 0);
        if (INVALID_SOCKET == s_)
        {
            UpdateLastError();
            return false;
        }
        
        udp_ = (SOCK_DGRAM == type);
        if (udp_)
        {
            enabled_events_ = DE_READ | DE_WRITE;
        }
        
        return true;
    }
    
    SocketAddress PhysicalSocket::GetLocalAddress() const
    {
        sockaddr_storage addr_storage = {0};
        socklen_t addrlen = sizeof(addr_storage);
        sockaddr* addr = reinterpret_cast<sockaddr*>(&addr_storage);
        int result = ::getsockname(s_, addr, &addrlen);
        
        SocketAddress address;
        if (result >= 0)
        {
            SocketAddressFromSockAddrStorage(addr_storage, &address);
        }
        else
        {
            printf("GetLocalAddress: unable to get local addr, socket=%d.\n", s_);
        }
        
        return address;
    }
    
    SocketAddress PhysicalSocket::GetRemoteAddress() const
    {
        sockaddr_storage addr_storage = {0};
        socklen_t addrlen = sizeof(addr_storage);
        sockaddr* addr = reinterpret_cast<sockaddr*>(&addr_storage);
        int result = ::getpeername(s_, addr, &addrlen);
        
        SocketAddress address;
        if (result >= 0)
        {
            SocketAddressFromSockAddrStorage(addr_storage, &address);
        }
        else
        {
            printf("GetRemoteAddress: unable to get remote addr, socket=%d.\n", s_);
        }
        
        return address;
    }
    
    int PhysicalSocket::Bind(const SocketAddress& bind_addr)
    {
        sockaddr_storage addr_storage;
        size_t len = bind_addr.ToSockAddrStorage(&addr_storage);
        sockaddr* addr = reinterpret_cast<sockaddr*>(&addr_storage);
        int err = ::bind(s_, addr, static_cast<int>(len));
        if (err < 0)
        {
            UpdateLastError();
        }

        return err;
    }
    
    int PhysicalSocket::Connect(const SocketAddress& addr)
    {
        if (state_ != CS_CLOSED)
        {
            SetError(EALREADY);
            return SOCKET_ERROR;
        }
        
        return DoConnect(addr);
    }
    
    int PhysicalSocket::DoConnect(const SocketAddress& connect_addr)
    {
        if ((s_ == INVALID_SOCKET) && !Create(connect_addr.family(), SOCK_STREAM))
        {
            return SOCKET_ERROR;
        }
        
        sockaddr_storage addr_storage;
        size_t len = connect_addr.ToSockAddrStorage(&addr_storage);
        sockaddr* addr = reinterpret_cast<sockaddr*>(&addr_storage);
        
        int err = ::connect(s_, addr, static_cast<int>(len));
        UpdateLastError();
        if (err == 0)
        {
            state_ = CS_CONNECTED;
        }
        else if (IsBlockingError(GetError()))
        {
            state_ = CS_CONNECTING;
            enabled_events_ |= DE_CONNECT;
        }
        else
        {
            return SOCKET_ERROR;
        }
        
        enabled_events_ |= DE_READ | DE_WRITE;
        
        return 0;
    }
    
    int PhysicalSocket::GetError() const
    {
        CriticalSectionScoped cs(&crit_);
        return error_;
    }
    
    void PhysicalSocket::SetError(int error)
    {
        CriticalSectionScoped cs(&crit_);
        error_ = error;
    }
    
    int PhysicalSocket::GetOption(Option opt, int* value)
    {
        int slevel;
        int sopt;
        if (TranslateOption(opt, &slevel, &sopt) == -1)
        {
            return -1;
        }
        
        socklen_t optlen = sizeof(*value);
        return ::getsockopt(s_, slevel, sopt, (SockOptArg)value, &optlen);
    }

    int PhysicalSocket::SetOption(Option opt, int value)
    {
        int slevel;
        int sopt;
        if (TranslateOption(opt, &slevel, &sopt) == -1)
        {
            return -1;
        }

        return ::setsockopt(s_, slevel, sopt, (SockOptArg)&value, sizeof(value));
    }
    
    int PhysicalSocket::Send(const void* pv, size_t cb)
    {
        int sent = (int)::send(s_, reinterpret_cast<const char *>(pv), (int)cb,0);
        
        UpdateLastError();
        MaybeRemapSendError();
        if ((sent < 0) && IsBlockingError(GetError())) {
            enabled_events_ |= DE_WRITE;
        }
        
        return sent;
    }
    
    int PhysicalSocket::SendTo(const void* buffer, size_t length, const SocketAddress& addr)
    {
        sockaddr_storage saddr;
        size_t len = addr.ToSockAddrStorage(&saddr);
        int sent = (int)::sendto(s_, static_cast<const char *>(buffer), static_cast<int>(length),
                            0,
                            reinterpret_cast<sockaddr*>(&saddr), static_cast<int>(len));
        UpdateLastError();
        MaybeRemapSendError();
        if ((sent < 0) && IsBlockingError(GetError()))
        {
            enabled_events_ |= DE_WRITE;
        }
        
        return sent;
    }
    
    int PhysicalSocket::Recv(void* buffer, size_t length)
    {
        int received = (int)::recv(s_, static_cast<char*>(buffer), static_cast<int>(length), 0);
        if ((received == 0) && (length != 0))
        {
            printf("EOF from socket; deferring close event");
            enabled_events_ |= DE_READ;
            SetError(EWOULDBLOCK);
            return SOCKET_ERROR;
        }
        
        UpdateLastError();
        int error = GetError();
        bool success = (received >= 0) || IsBlockingError(error);
        if (udp_ || success) {
            enabled_events_ |= DE_READ;
        }
        
        if (!success) {
            printf("Error = %d.\n", error);
        }
        
        return received;
    }
    
    int PhysicalSocket::RecvFrom(void* buffer, size_t length, SocketAddress* out_addr)
    {
        sockaddr_storage addr_storage;
        socklen_t addr_len = sizeof(addr_storage);
        sockaddr* addr = reinterpret_cast<sockaddr*>(&addr_storage);
        
        int received = (int)::recvfrom(s_, static_cast<char*>(buffer), static_cast<int>(length), 0, addr, &addr_len);
        UpdateLastError();
        if ((received >= 0) && (out_addr != NULL))
            SocketAddressFromSockAddrStorage(addr_storage, out_addr);
        
        int error = GetError();
        
        bool success = (received >= 0) || IsBlockingError(error);
        if (udp_ || success)
        {
            enabled_events_ |= DE_READ;
        }
        
        if (!success)
        {
            printf("Error = %d.\n", error);
        }
        return received;
    }
    
    int PhysicalSocket::Listen(int backlog)
    {
        int err = ::listen(s_, backlog);
        UpdateLastError();
        if (err == 0)
        {
            state_ = CS_CONNECTING;
            enabled_events_ |= DE_ACCEPT;
        }
        
        return err;
    }
    
    AsyncSocket* PhysicalSocket::Accept(SocketAddress* out_addr)
    {
        sockaddr_storage addr_storage;
        socklen_t addr_len = sizeof(addr_storage);
        sockaddr* addr = reinterpret_cast<sockaddr*>(&addr_storage);
        SOCKET s = ::accept(s_, addr, &addr_len);
        UpdateLastError();
        if (s == INVALID_SOCKET)
            return NULL;
        enabled_events_ |= DE_ACCEPT;
        if (out_addr != NULL)
            SocketAddressFromSockAddrStorage(addr_storage, out_addr);
        
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
    
    int PhysicalSocket::Close()
    {
        if (s_ == INVALID_SOCKET)
            return 0;
        
        int err = ::closesocket(s_);
        UpdateLastError();
        s_ = INVALID_SOCKET;
        state_ = CS_CLOSED;
        enabled_events_ = 0;

        return err;
    }
    
    int PhysicalSocket::EstimateMTU(uint16* mtu)
    {
        SocketAddress addr = GetRemoteAddress();
        if (addr.IsAny()) {
            SetError(ENOTCONN);
            return -1;
        }
        
#if defined(CIO_IOS)
        SetError(EINVAL);
        return -1;
#elif defined(CIO_ANDROID)
        int value;
        socklen_t vlen = sizeof(value);
        int err = getsockopt(s_, IPPROTO_IP, IP_MTU, &value, &vlen);
        if (err < 0) {
            UpdateLastError();
            return err;
        }

        *mtu = value;
        return 0;
#endif
    }
    
    void PhysicalSocket::MaybeRemapSendError()
    {
#if defined(CIO_IOS)
        if (GetError() == ENOBUFS) {
            SetError(EWOULDBLOCK);
        }
#endif
    }
    
    int PhysicalSocket::TranslateOption(Option opt, int* slevel, int* sopt)
    {
        switch (opt) {
            case OPT_DONTFRAGMENT:
#if defined(CIO_IOS)
                printf("Socket::OPT_DONTFRAGMENT not supported.\n");
                return -1;
#elif defined(CIO_ANDROID)
                *slevel = IPPROTO_IP;
                *sopt = IP_MTU_DISCOVER;
                break;
#endif
            case OPT_RCVBUF:
                *slevel = SOL_SOCKET;
                *sopt = SO_RCVBUF;
                break;
            case OPT_SNDBUF:
                *slevel = SOL_SOCKET;
                *sopt = SO_SNDBUF;
                break;
            case OPT_NODELAY:
                *slevel = IPPROTO_TCP;
                *sopt = TCP_NODELAY;
                break;
            case OPT_DSCP:
                //LOG(LS_WARNING) << "Socket::OPT_DSCP not supported.";
                return -1;
            case OPT_RTP_SENDTIME_EXTN_ID:
                return -1;  // No logging is necessary as this not a OS socket option.
            default:
                return -1;
        }
        return 0;
    }
}
