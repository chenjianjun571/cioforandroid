//
// Created by chenjianjun on 15/9/25.
//

#include "NetCenter.h"

NetCenter::NetCenter(JNIEnv* jni, jobject j_observer)
        :_run_flg(false),
         j_observer_global_(jni, j_observer),
         j_observer_class_(jni, GetObjectClass(jni, *j_observer_global_))
{
    loop_thread_ = std::make_shared<Thread>();
    spcket_service_ = std::make_shared<PhysicalSocketServer>();
    rw_local_ = RWLock::Create();
}

NetCenter::~NetCenter()
{
    NetExit();
    delete rw_local_;
}

int NetCenter::NetInit()
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

int NetCenter::NetExit()
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

int NetCenter::CreateFD()
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

void NetCenter::CloseFD(int fd)
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

bool NetCenter::ConnectServiceWhitFD(int fd , const char* host_name, int port)
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

int NetCenter::SendDataWithFD(int fd , const void* pv, int cb)
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

void NetCenter::Clear()
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

void NetCenter::OnConnectEvent(AsyncSocket* socket)
{
    SocketDispatcher* pSocketDispatcher = static_cast<SocketDispatcher*>(socket);
    jmethodID m = GetMethodID(jni(), *j_observer_class_, "StatusReportWithFD", "(II)V");
    jni()->CallVoidMethod(*j_observer_global_, m, pSocketDispatcher->GetDescriptor(), (int)ENE_CONNECTED);
}

void NetCenter::OnWriteEvent(AsyncSocket* socket_)
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

void NetCenter::OnReadEvent(AsyncSocket* socket_)
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

void NetCenter::OnCloseEvent(AsyncSocket* socket, int err)
{
    SocketDispatcher* pSocketDispatcher = static_cast<SocketDispatcher*>(socket);
    jmethodID m = GetMethodID(jni(), *j_observer_class_, "StatusReportWithFD", "(II)V");
    jni()->CallVoidMethod(*j_observer_global_, m, pSocketDispatcher->GetDescriptor(), (int)ENE_CLOSE);
}
