//
// Created by chenjianjun on 15/9/25.
//

#ifndef CIO_NETCENTER_H
#define CIO_NETCENTER_H

#include "./net/PhysicalSocketServer.h"
#include "./net/SocketDispatcher.h"
#include "./base/Thread.h"
#include "./base/RWLock.h"
#include "JniHelpers.h"
#include "ClassreFerenceHolder.h"
#include <map>

#define MAX_RECV_DATA_SIZE 1*1024*1024
#define MAX_SEND_DATA_SIZE 1*1024*1024

using namespace jsbn;
using namespace cio_jni;

class NetCenter:public sigslot::has_slots<sigslot::multi_threaded_local>
{
private:
    struct stSocketObj
    {
        char* inbuf_;
        char* outbuf_;
        size_t insize_;
        size_t inpos_;
        size_t outsize_;
        size_t outpos_;
        SocketDispatcher* dispatch;

        stSocketObj(SocketDispatcher* dispatch_)
        {
            inpos_ = 0;
            outpos_ = 0;
            insize_ = MAX_RECV_DATA_SIZE;
            outsize_ = MAX_SEND_DATA_SIZE;
            inbuf_ = new char[insize_];
            outbuf_ = new char[outsize_];
            dispatch = dispatch_;
        }

        ~stSocketObj()
        {
            delete [] inbuf_;
            delete [] outbuf_;

            if (nullptr != dispatch) {
                delete dispatch;
            }
        }
    };

    enum emNetEvent {
        EVE_UNKNOWN = -1,// 未知错误
        ENE_CONNECTED = 0,// 连接建立
        ENE_HEART,// 心跳
        ENE_HEART_TIMEOUT,// 心跳检测超时
        ENE_ACCEPT_ERROR,// 监听失败
        ENE_CLOSE// 连接关闭
    };

    class NetRunnable:public Runnable
    {
    public:
        NetRunnable() {}
        ~NetRunnable() {}

    protected:
        virtual void Run(void*user_data)
        {
            NetCenter* pNetCenter = (NetCenter*)user_data;
            while (pNetCenter->_run_flg)
            {
                pNetCenter->spcket_service_->Wait(1000, true);
            }
        }
    };

    friend class NetRunnable;

public:
    NetCenter(JNIEnv* jni, jobject j_observer);

    ~NetCenter();

    int NetInit();
    int NetExit();
    int CreateFD();
    void CloseFD(int fd);
    bool ConnectServiceWhitFD(int fd , const char* host_name, int port);
    int SendDataWithFD(int fd , const void* pv, int cb);

protected:

    void Clear();
    void OnConnectEvent(AsyncSocket* socket);
    void OnWriteEvent(AsyncSocket* socket_);
    void OnReadEvent(AsyncSocket* socket_);
    void OnCloseEvent(AsyncSocket* socket, int err);

private:
    JNIEnv* jni() {
        return AttachCurrentThreadIfNeeded();
    }

    const ScopedGlobalRef<jobject> j_observer_global_;
    const ScopedGlobalRef<jclass> j_observer_class_;

private:
    std::shared_ptr<Thread> loop_thread_;
    std::shared_ptr<PhysicalSocketServer> spcket_service_;
    NetRunnable _runnable;

    volatile bool _run_flg;

    // 网络连接对象
    std::map<SOCKET, struct stSocketObj*> socket_objs;
    RWLock* rw_local_;
};


#endif //CIO_NETCENTER_H
