//
// Created by chenjianjun on 15/9/18.
//

#include "org_cio_CIO.h"
#include "./net/PhysicalSocketServer.h"
#include "./net/SocketDispatcher.h"
#include "./base/Thread.h"
#include "./base/RWLock.h"
#include "JniHelpers.h"
#include "ClassreFerenceHolder.h"

#define MAX_RECV_DATA_SIZE 1*1024*1024
#define MAX_SEND_DATA_SIZE 1*1024*1024

using namespace jsbn;
using namespace cio_jni;
static JavaVM* g_jvm = nullptr;

class NetCenter:public sigslot::has_slots<sigslot::multi_threaded_local>
{
public:
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

public:
    NetCenter(JNIEnv* jni, jobject j_observer):
            _run_flg(false),
            j_observer_global_(jni, j_observer),
            j_observer_class_(jni, GetObjectClass(jni, *j_observer_global_))
    {
        loop_thread_ = std::make_shared<Thread>();
        spcket_service_ = std::make_shared<PhysicalSocketServer>();
        rw_local_ = RWLock::Create();
    }

    ~NetCenter()
    {
        NetExit();
    }

    int NetInit()
    {
        if (_run_flg)
        {
            return FUNC_SUCCESS;
        }

        spcket_service_->SetPosixSignalHandler(SIGPIPE, SIG_IGN);

        // 开启事件监听主线程
        _run_flg = true;
        if (loop_thread_->Start(&_runnable, this))
        {
            return FUNC_SUCCESS;
        }

        // 开始线程失败置运行标志
        _run_flg = false;

        return FUNC_FAILED;
    }

    int NetExit()
    {
        if (!_run_flg)
        {
            return FUNC_SUCCESS;
        }

        _run_flg = false;
        spcket_service_->WakeUp();
        loop_thread_->Stop();

        Clear();
        spcket_service_.reset();
        loop_thread_.reset();

        return FUNC_SUCCESS;
    }

    int CreateFD()
    {
        if (!_run_flg)
        {
            return -1;
        }

        SocketDispatcher* dispatcher = spcket_service_->CreateAsyncSocket(AF_INET, SOCK_STREAM);
        if (nullptr == dispatcher) {
            return -1;
        }

        dispatcher->SignalCloseEvent.connect(this, &NetCenter::OnCloseEvent);
        dispatcher->SignalConnectEvent.connect(this, &NetCenter::OnConnectEvent);
        dispatcher->SignalWriteEvent.connect(this, &NetCenter::OnWriteEvent);
        dispatcher->SignalReadEvent.connect(this, &NetCenter::OnReadEvent);

        WriteLockScoped wl(*rw_local_);

        socket_objs[dispatcher->GetDescriptor()] = new stSocketObj(dispatcher);
        spcket_service_->Add(dispatcher);
        spcket_service_->WakeUp();

        return dispatcher->GetDescriptor();
    }

    void CloseFD(int fd)
    {
        if (!_run_flg)
        {
            return;
        }

        WriteLockScoped wl(*rw_local_);
        std::map<SOCKET, struct stSocketObj*>::iterator it = socket_objs.find(fd);
        if (socket_objs.end() != it)
        {
            it->second->dispatch->SignalCloseEvent.disconnect(this);
            it->second->dispatch->SignalConnectEvent.disconnect(this);
            it->second->dispatch->SignalWriteEvent.disconnect(this);
            it->second->dispatch->SignalReadEvent.disconnect(this);

            spcket_service_->Remove(it->second->dispatch);
            spcket_service_->WakeUp();

            delete it->second;
            socket_objs.erase(it);
        }
    }

    bool ConnectServiceWhitFD(int fd , const char* host_name, int port)
    {
        if (!_run_flg)
        {
            return false;
        }

        SocketAddress addr(host_name, port);

        ReadLockScoped wl(*rw_local_);
        std::map<SOCKET, struct stSocketObj*>::iterator it = socket_objs.find(fd);
        if (socket_objs.end() == it)
        {
            return false;
        }

        if (it->second->dispatch->Connect(addr) != 0)
        {
            return false;
        }

        if (it->second->dispatch->GetState() == jsbn::AsyncSocket::CS_CONNECTED)
        {
            jmethodID m = GetMethodID(jni(), *j_observer_class_, "StatusReportWithFD", "(II)V");
            jni()->CallVoidMethod(*j_observer_global_, m, it->second->dispatch->GetDescriptor(), (int)ENE_CONNECTED);
        }

        return true;
    }

    int SendDataWithFD(int fd , const void* pv, int cb)
    {
        ReadLockScoped wl(*rw_local_);
        std::map<SOCKET, struct stSocketObj*>::iterator it = socket_objs.find(fd);
        if (socket_objs.end() == it)
        {
            return false;
        }

        if (it->second->outpos_ + cb + kPacketLenSize > it->second->outsize_) {
            it->second->dispatch->SetError(EMSGSIZE);
            return -1;
        }

        SetBE16(it->second->outbuf_+it->second->outpos_, cb);
        memcpy(it->second->outbuf_+it->second->outpos_ + kPacketLenSize, pv, cb);
        it->second->outpos_ += cb + kPacketLenSize;

        int res = it->second->dispatch->Send(it->second->outbuf_, it->second->outpos_);
        if (res <= 0) {
            return res;
        }
        if (static_cast<size_t>(res) <= it->second->outpos_) {
            it->second->outpos_ -= res;
        } else {
            return -1;
        }
        if (it->second->outpos_ > 0) {
            memmove(it->second->outbuf_, it->second->outbuf_ + res, it->second->outpos_);
        }

        return res;
    }

protected:

    void Clear()
    {
        WriteLockScoped wl(*rw_local_);
        std::map<SOCKET, struct stSocketObj*>::iterator it = socket_objs.begin();
        while (socket_objs.end() != it)
        {
            spcket_service_->Remove(it->second->dispatch);
            delete it->second;
            socket_objs.erase(it++);
        }
        spcket_service_->WakeUp();
    }

    void OnConnectEvent(AsyncSocket* socket)
    {
        SocketDispatcher* pSocketDispatcher = static_cast<SocketDispatcher*>(socket);
        jmethodID m = GetMethodID(jni(), *j_observer_class_, "StatusReportWithFD", "(II)V");
        jni()->CallVoidMethod(*j_observer_global_, m, pSocketDispatcher->GetDescriptor(), (int)ENE_CONNECTED);
    }

    void OnWriteEvent(AsyncSocket* socket_)
    {
        SocketDispatcher* pSocketDispatcher = static_cast<SocketDispatcher*>(socket_);
        struct stSocketObj* sst = socket_objs[pSocketDispatcher->GetDescriptor()];
        if (sst->outpos_ > 0)
        {
            int res = sst->dispatch->Send(sst->outbuf_, sst->outpos_);
            if (res <= 0) {
                return;
            }
            if (static_cast<size_t>(res) <= sst->outpos_) {
                sst->outpos_ -= res;
            } else {
                return;
            }
            if (sst->outpos_ > 0) {
                memmove(sst->outbuf_, sst->outbuf_ + res, sst->outpos_);
            }
        }
    }

    void OnReadEvent(AsyncSocket* socket_)
    {
        SocketDispatcher* pSocketDispatcher = static_cast<SocketDispatcher*>(socket_);
        struct stSocketObj* sst = socket_objs[pSocketDispatcher->GetDescriptor()];

        int len = socket_->Recv(sst->inbuf_ + sst->inpos_, sst->insize_ - sst->inpos_);
        if (len < 0)
        {
            if (!socket_->IsBlocking())
            {
                LOGE("Recv() returned error: %d.", socket_->GetError());
            }
            return;
        }

        sst->inpos_ += len;

        while (true)
        {
            if (sst->inpos_ < kPacketLenSize)
            {
                break;
            }

            PacketLength pkt_len = GetBE16(sst->inbuf_);
            if (sst->inpos_ < kPacketLenSize + pkt_len)
            {
                break;
            }

            jbyteArray array = jni()->NewByteArray(pkt_len);//定义数据变量
            jni()->SetByteArrayRegion(array, 0, pkt_len, (const jbyte*)sst->inbuf_+kPacketLenSize);
            jmethodID m = GetMethodID(jni(), *j_observer_class_, "RecvTCPDataWithFD", "(I[BI)V");
            jni()->CallVoidMethod(*j_observer_global_, m, pSocketDispatcher->GetDescriptor(), array, pkt_len);
            jni()->DeleteLocalRef(array);

            sst->inpos_ -= kPacketLenSize + pkt_len;
            if (sst->inpos_ > 0)
            {
                memmove(sst->inbuf_, sst->inbuf_ + kPacketLenSize + pkt_len, sst->inpos_);
            }
        }

        if (sst->inpos_ >= sst->insize_)
        {
            LOGE("input buffer overflow");
            sst->inpos_ = 0;
        }
    }

    void OnCloseEvent(AsyncSocket* socket, int err)
    {
        SocketDispatcher* pSocketDispatcher = static_cast<SocketDispatcher*>(socket);
        jmethodID m = GetMethodID(jni(), *j_observer_class_, "StatusReportWithFD", "(II)V");
        jni()->CallVoidMethod(*j_observer_global_, m, pSocketDispatcher->GetDescriptor(), (int)ENE_CLOSE);
    }

private:
    JNIEnv* jni() {
        return AttachCurrentThreadIfNeeded();
    }

    const ScopedGlobalRef<jobject> j_observer_global_;
    const ScopedGlobalRef<jclass> j_observer_class_;

private:
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

private:
    std::shared_ptr<Thread> loop_thread_;
    std::shared_ptr<PhysicalSocketServer> spcket_service_;
    NetRunnable _runnable;

    volatile bool _run_flg;

    // 网络连接对象
    std::map<SOCKET, struct stSocketObj*> socket_objs;
    RWLock* rw_local_;
};

static NetCenter* g_NetCenter = nullptr;

// ##############################################jni实现#############################################
#ifdef __cplusplus
extern "C" {
#endif

jint JNIEXPORT JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved)
{
    jint ret = InitGlobalJniVariables(jvm);
    if (ret < 0)
        return -1;

    LoadGlobalClassReferenceHolder();

    return ret;
}

void JNIEXPORT JNICALL JNI_OnUnLoad(JavaVM *jvm, void *reserved) {
    FreeGlobalClassReferenceHolder();
}

//static jobject JavaEnumFromIndex(JNIEnv* jni, const std::string& state_class_fragment, int index)
//{
//    const std::string state_class = "org/cio/" + state_class_fragment;
//    return JavaEnumFromIndex(jni, FindClass(jni, state_class.c_str()), state_class, index);
//}

JNIEXPORT jboolean JNICALL Java_org_cio_CIO_InitWithDelegate
        (JNIEnv *jni, jobject, jobject j_observer)
{
    g_NetCenter = new(std::nothrow) NetCenter(jni, j_observer);
    if (nullptr == g_NetCenter)
    {
        return false;
    }

    if (FUNC_SUCCESS != g_NetCenter->NetInit())
    {
        return false;
    }

    return true;
}

/*
 * Class:     org_cio_CIO
 * Method:    Terminat
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_cio_CIO_Terminat(JNIEnv *, jobject)
{
    if (nullptr == g_NetCenter)
    {
        return;
    }

    g_NetCenter->NetExit();

    delete g_NetCenter;
    g_NetCenter = nullptr;
}

/*
 * Class:     org_cio_CIO
 * Method:    OpenIOChannel
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_cio_CIO_OpenIOChannel
        (JNIEnv *, jobject)
{
    if (nullptr == g_NetCenter)
    {
        return -1;
    }

    return g_NetCenter->CreateFD();
}

/*
 * Class:     org_cio_CIO
 * Method:    ConnectServiceWhitFD
 * Signature: (ILjava/lang/String;I)Z
 */
JNIEXPORT jboolean JNICALL Java_org_cio_CIO_ConnectServiceWhitFD
        (JNIEnv *env, jobject, jint fd, jstring hostname, jint hostport)
{
    if (nullptr == g_NetCenter)
    {
        return false;
    }

    const char *hostname_ = env->GetStringUTFChars(hostname, nullptr);
    if (g_NetCenter->ConnectServiceWhitFD(fd, hostname_, hostport))
    {
        env->ReleaseStringUTFChars(hostname, hostname_);
        return true;
    }

    env->ReleaseStringUTFChars(hostname, hostname_);

    return false;
}

/*
 * Class:     org_cio_CIO
 * Method:    SendDataWithFD
 * Signature: (I[CI)I
 */
JNIEXPORT jint JNICALL Java_org_cio_CIO_SendDataWithFD
        (JNIEnv *env, jobject, jint fd, jbyteArray buffer, jint len)
{
    jbyte *array = env->GetByteArrayElements(buffer, nullptr);
    if(array == nullptr){
        return 0;
    }

    jint ret = g_NetCenter->SendDataWithFD(fd, array, len);
    env->ReleaseByteArrayElements(buffer, array, 0);

    return ret;
}

/*
 * Class:     org_cio_CIO
 * Method:    CloseIOChannelWithFD
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_org_cio_CIO_CloseIOChannelWithFD
(JNIEnv *, jobject, jint fd)
{
    if (nullptr == g_NetCenter)
    {
        return;
    }

    g_NetCenter->CloseFD(fd);
}

#ifdef __cplusplus
}
#endif

