#ifndef _COMMON_H_
#define _COMMON_H_

#include <cassert>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <pthread.h>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>  // for TCP_NODELAY
#include <arpa/inet.h>
#include <string>
#include <memory>


#define NAME_SPACE jsbn


namespace NAME_SPACE {
    
#define DISALLOW_ASSIGN(TypeName) \
void operator=(const TypeName&)
    
#define DISALLOW_COPY_AND_ASSIGN(TypeName)    \
TypeName(const TypeName&);                    \
DISALLOW_ASSIGN(TypeName)
    
#define DISALLOW_EVIL_CONSTRUCTORS(TypeName) \
DISALLOW_COPY_AND_ASSIGN(TypeName)
    
#undef DISALLOW_IMPLICIT_CONSTRUCTORS
#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
TypeName();                                    \
DISALLOW_EVIL_CONSTRUCTORS(TypeName)
    
#define LIBJINGLE_DEFINE_STATIC_LOCAL(type, name, arguments) \
static type& name = *new type arguments
    
    typedef unsigned int uint32;
    typedef int int32;
    typedef unsigned short uint16;  // NOLINT
    typedef short int16;  // NOLINT
    typedef unsigned char uint8;
    typedef char int8;
    typedef uint64_t uint64;
    typedef int64_t int64;
    
    inline void Set8(void* memory, size_t offset, uint8 v) {
        static_cast<uint8*>(memory)[offset] = v;
    }
    
    inline uint8 Get8(const void* memory, size_t offset) {
        return static_cast<const uint8*>(memory)[offset];
    }
    
    inline void SetBE16(void* memory, uint16 v) {
        Set8(memory, 0, static_cast<uint8>(v >> 8));
        Set8(memory, 1, static_cast<uint8>(v >> 0));
    }
    
    inline void SetBE32(void* memory, uint32 v) {
        Set8(memory, 0, static_cast<uint8>(v >> 24));
        Set8(memory, 1, static_cast<uint8>(v >> 16));
        Set8(memory, 2, static_cast<uint8>(v >> 8));
        Set8(memory, 3, static_cast<uint8>(v >> 0));
    }
    
    inline void SetBE64(void* memory, uint64 v) {
        Set8(memory, 0, static_cast<uint8>(v >> 56));
        Set8(memory, 1, static_cast<uint8>(v >> 48));
        Set8(memory, 2, static_cast<uint8>(v >> 40));
        Set8(memory, 3, static_cast<uint8>(v >> 32));
        Set8(memory, 4, static_cast<uint8>(v >> 24));
        Set8(memory, 5, static_cast<uint8>(v >> 16));
        Set8(memory, 6, static_cast<uint8>(v >> 8));
        Set8(memory, 7, static_cast<uint8>(v >> 0));
    }
    
    inline uint16 GetBE16(const void* memory) {
        return static_cast<uint16>((Get8(memory, 0) << 8) |
                                   (Get8(memory, 1) << 0));
    }
    
    inline uint32 GetBE32(const void* memory) {
        return (static_cast<uint32>(Get8(memory, 0)) << 24) |
        (static_cast<uint32>(Get8(memory, 1)) << 16) |
        (static_cast<uint32>(Get8(memory, 2)) << 8) |
        (static_cast<uint32>(Get8(memory, 3)) << 0);
    }
    
    inline uint64 GetBE64(const void* memory) {
        return (static_cast<uint64>(Get8(memory, 0)) << 56) |
        (static_cast<uint64>(Get8(memory, 1)) << 48) |
        (static_cast<uint64>(Get8(memory, 2)) << 40) |
        (static_cast<uint64>(Get8(memory, 3)) << 32) |
        (static_cast<uint64>(Get8(memory, 4)) << 24) |
        (static_cast<uint64>(Get8(memory, 5)) << 16) |
        (static_cast<uint64>(Get8(memory, 6)) << 8) |
        (static_cast<uint64>(Get8(memory, 7)) << 0);
    }
    
    inline void SetLE16(void* memory, uint16 v) {
        Set8(memory, 0, static_cast<uint8>(v >> 0));
        Set8(memory, 1, static_cast<uint8>(v >> 8));
    }
    
    inline void SetLE32(void* memory, uint32 v) {
        Set8(memory, 0, static_cast<uint8>(v >> 0));
        Set8(memory, 1, static_cast<uint8>(v >> 8));
        Set8(memory, 2, static_cast<uint8>(v >> 16));
        Set8(memory, 3, static_cast<uint8>(v >> 24));
    }
    
    inline void SetLE64(void* memory, uint64 v) {
        Set8(memory, 0, static_cast<uint8>(v >> 0));
        Set8(memory, 1, static_cast<uint8>(v >> 8));
        Set8(memory, 2, static_cast<uint8>(v >> 16));
        Set8(memory, 3, static_cast<uint8>(v >> 24));
        Set8(memory, 4, static_cast<uint8>(v >> 32));
        Set8(memory, 5, static_cast<uint8>(v >> 40));
        Set8(memory, 6, static_cast<uint8>(v >> 48));
        Set8(memory, 7, static_cast<uint8>(v >> 56));
    }
    
    inline uint16 GetLE16(const void* memory) {
        return static_cast<uint16>((Get8(memory, 0) << 0) |
                                   (Get8(memory, 1) << 8));
    }
    
    inline uint32 GetLE32(const void* memory) {
        return (static_cast<uint32>(Get8(memory, 0)) << 0) |
        (static_cast<uint32>(Get8(memory, 1)) << 8) |
        (static_cast<uint32>(Get8(memory, 2)) << 16) |
        (static_cast<uint32>(Get8(memory, 3)) << 24);
    }
    
    inline uint64 GetLE64(const void* memory) {
        return (static_cast<uint64>(Get8(memory, 0)) << 0) |
        (static_cast<uint64>(Get8(memory, 1)) << 8) |
        (static_cast<uint64>(Get8(memory, 2)) << 16) |
        (static_cast<uint64>(Get8(memory, 3)) << 24) |
        (static_cast<uint64>(Get8(memory, 4)) << 32) |
        (static_cast<uint64>(Get8(memory, 5)) << 40) |
        (static_cast<uint64>(Get8(memory, 6)) << 48) |
        (static_cast<uint64>(Get8(memory, 7)) << 56);
    }
    
    inline bool IsHostBigEndian() {
        static const int number = 1;
        return 0 == *reinterpret_cast<const char*>(&number);
    }
    
    inline uint16 HostToNetwork16(uint16 n) {
        uint16 result;
        SetBE16(&result, n);
        return result;
    }
    
    inline uint32 HostToNetwork32(uint32 n) {
        uint32 result;
        SetBE32(&result, n);
        return result;
    }
    
    inline uint64 HostToNetwork64(uint64 n) {
        uint64 result;
        SetBE64(&result, n);
        return result;
    }
    
    inline uint16 NetworkToHost16(uint16 n) {
        return GetBE16(&n);
    }
    
    inline uint32 NetworkToHost32(uint32 n) {
        return GetBE32(&n);
    }
    
    inline uint64 NetworkToHost64(uint64 n) {
        return GetBE64(&n);
    }
    
#define ARRAY_SIZE(x) (static_cast<int>(sizeof(x) / sizeof(x[0])))
    
    // 函数返回值
#ifndef FUNC_SUCCESS
#define FUNC_SUCCESS 0
#endif
    
#ifndef FUNC_FAILED
#define FUNC_FAILED -1
#endif
    
#define SOCKET int
#define IP_MTU 14 // Until this is integrated from linux/in.h to netinet/in.h
    typedef void* SockOptArg;
    
#define closesocket close
    typedef int NativeSocket;
    
#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif
    
#ifndef INVALID_SOCKET
#define INVALID_SOCKET  static_cast<NativeSocket>(-1)
#endif
    
typedef uint16 PacketLength;
static const size_t kPacketLenSize = sizeof(PacketLength);
    
    inline bool ImplicitCastToBool(bool result) { return result; }
    
#ifndef VERIFY
#define VERIFY(x) ImplicitCastToBool(x)
#endif
    
#define LAST_SYSTEM_ERROR (errno)
    
    //
    //#define RECV_DATA_MAX_PACKET_SIZE 4096
    //#define SEND_DATA_MAX_PACKET_SIZE 4096
    //    typedef uint16 PacketLength;
    //    static const size_t kPacketLenSize = sizeof(PacketLength);
    //
    //    typedef struct stSendData
    //    {
    //        /// 发送数据缓冲区
    //        unsigned char send_buf[SEND_DATA_MAX_PACKET_SIZE];
    //        /// 发送数据长度
    //        unsigned int send_len;
    //        /// 连接句柄
    //        int sock_handle;
    //
    //        void Copy(const unsigned char* buf, unsigned int len) {
    //            // 贴上包头
    //            SetBE16(send_buf, len);
    //            // 贴上包体
    //            ::memcpy(send_buf+kPacketLenSize, buf, (len>SEND_DATA_MAX_PACKET_SIZE) ? SEND_DATA_MAX_PACKET_SIZE : len);
    //
    //            send_len = kPacketLenSize + ((len>SEND_DATA_MAX_PACKET_SIZE) ? SEND_DATA_MAX_PACKET_SIZE : len);
    //        }
    //    }TSendData;
    //    // 发送数据智能指针
    //    typedef std::shared_ptr<TSendData> sSendDataPage_ptr;
    //
}

#endif
