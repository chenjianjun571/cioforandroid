//
//  IPAddress.hpp
//  cio
//
//  Created by chenjianjun on 15/9/22.
//  Copyright © 2015年 cio. All rights reserved.
//

#ifndef __IPAddress_h_
#define __IPAddress_h_

#include "../base/Common.h"

namespace NAME_SPACE {
    
    enum IPv6AddressFlag
    {
        IPV6_ADDRESS_FLAG_NONE = 0x00,
        // Temporary address is dynamic by nature and will not carry MAC
        // address.
        IPV6_ADDRESS_FLAG_TEMPORARY = 1 << 0,
        // Temporary address could become deprecated once the preferred
        // lifetime is reached. It is still valid but just shouldn't be used
        // to create new connection.
        IPV6_ADDRESS_FLAG_DEPRECATED = 1 << 1,
    };
 
    class IPAddress
    {
    public:
        IPAddress():family_(AF_UNSPEC)
        {
            ::memset(&u_, 0, sizeof(u_));
        }
        
        explicit IPAddress(const in_addr& ip4):family_(AF_INET)
        {
            ::memset(&u_, 0, sizeof(u_));
            u_.ip4 = ip4;
        }
        
        explicit IPAddress(const in6_addr& ip6):family_(AF_INET6)
        {
            ::memset(&u_, 0, sizeof(u_));
            u_.ip6 = ip6;
        }
        
        explicit IPAddress(uint32 ip_in_host_byte_order):family_(AF_INET) {
            memset(&u_, 0, sizeof(u_));
            u_.ip4.s_addr = HostToNetwork32(ip_in_host_byte_order);
        }
        
        IPAddress(const IPAddress& other):family_(other.family_)
        {
            ::memcpy(&u_, &other.u_, sizeof(u_));
        }
        
        const IPAddress& operator=(const IPAddress& other) {
            family_ = other.family_;
            ::memcpy(&u_, &other.u_, sizeof(u_));
            return *this;
        }
        
        virtual ~IPAddress() {}
        
        bool operator==(const IPAddress& other) const;
        bool operator!=(const IPAddress& other) const;
        bool operator <(const IPAddress& other) const;
        bool operator >(const IPAddress& other) const;
        
        int family() const { return family_; }
        in_addr ipv4_address() const;
        in6_addr ipv6_address() const;
        
        // Returns the number of bytes needed to store the raw address.
        size_t Size() const;
        
        // Wraps inet_ntop.
        std::string ToString() const;
        
        // Returns this address as an IPv6 address.
        // Maps v4 addresses (as ::ffff:a.b.c.d), returns v6 addresses unchanged.
        IPAddress AsIPv6Address() const;
        
        // 对于SocketAddress的地址。返回主机字节顺序的字节.
        uint32 v4AddressAsHostOrderInteger() const;
        
        static void set_strip_sensitive(bool enable);
        
    private:
        int family_;
        union {
            in_addr ip4;
            in6_addr ip6;
        } u_;
        
        static bool strip_sensitive_;
    };
    
    bool IPFromString(const std::string& str, IPAddress* out);
    bool IPIsAny(const IPAddress& ip);
    bool IPIsUnspec(const IPAddress& ip);
}

#endif /* __IPAddress_h_ */
