//
//  Thread.cpp
//  Frame
//
//  Created by chenjianjun on 15/9/2.
//  Copyright (c) 2015年 jsbn. All rights reserved.
//

#include "Thread.h"
#include <time.h>

namespace NAME_SPACE {
    
    ThreadManager* ThreadManager::Instance() {
        LIBJINGLE_DEFINE_STATIC_LOCAL(ThreadManager, thread_manager, ());
        return &thread_manager;
    }
    
    ThreadManager::ThreadManager() {
        pthread_key_create(&key_, nullptr);
    }
    
    ThreadManager::~ThreadManager() {
        pthread_key_delete(key_);
    }
    
    Thread *ThreadManager::CurrentThread() {
        return static_cast<Thread *>(pthread_getspecific(key_));
    }
    
    void ThreadManager::SetCurrentThread(Thread *thread) {
        pthread_setspecific(key_, thread);
    }
    
    struct ThreadInit {
        Thread* thread;
        Runnable* runnable;
        void* userdata;
    };
    
    Thread* Thread::Current() {
        return ThreadManager::Instance()->CurrentThread();
    }
    
    Thread::Thread():priority_(PRIORITY_NORMAL),running_(true, false),owned_(true) {
        SetName("Thread", this);
    }
    
    bool Thread::SleepMs(int milliseconds) {
        
        // POSIX has both a usleep() and a nanosleep(), but the former is deprecated,
        // so we use nanosleep() even though it has greater precision than necessary.
        struct timespec ts;
        ts.tv_sec = milliseconds / 1000;
        ts.tv_nsec = (milliseconds % 1000) * 1000000;
        int ret = nanosleep(&ts, nullptr);
        if (ret != 0) {
            return false;
        }
        return true;
    }
    
    bool Thread::SetName(const std::string& name, const void* obj) {
        if (running()) return false;
        name_ = name;
        if (obj) {
            char buf[16];
            snprintf(buf, sizeof(buf), " 0x%p", obj);
            name_ += buf;
        }
        return true;
    }
    
    bool Thread::SetPriority(ThreadPriority priority) {
        if (running()) return false;
        priority_ = priority;
        return true;
    }
    
    bool Thread::Start(Runnable* runnable, void* userdata) {
        if (!runnable) return false;
        if (!owned_) return false;
        if (running()) return false;
        
        ThreadManager::Instance();
        
        ThreadInit* init = new ThreadInit;
        init->thread = this;
        init->runnable = runnable;
        init->userdata = userdata;
        
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        
        if (priority_ != PRIORITY_NORMAL) {
            if (priority_ == PRIORITY_IDLE) {
                //LOG("PRIORITY_IDLE not supported");
            } else {
                if (pthread_attr_setschedpolicy(&attr, SCHED_RR) != 0) {
                    //LOG(ERROR) << "pthread_attr_setschedpolicy";
                }
                struct sched_param param;
                if (pthread_attr_getschedparam(&attr, &param) != 0) {
                    //LOG(ERROR) << "pthread_attr_getschedparam";
                } else {
                    if (priority_ == PRIORITY_HIGH) {
                        param.sched_priority = 6;// 6 = HIGH
                    } else {
                        assert(priority_ == PRIORITY_ABOVE_NORMAL);
                        param.sched_priority = 4;// 4 = ABOVE_NORMAL
                    }
                    if (pthread_attr_setschedparam(&attr, &param) != 0) {
                        //LOG(ERROR) << "pthread_attr_setschedparam";
                    }
                }
            }
        }
        
        int error_code = pthread_create(&thread_, &attr, PreRun, init);
        if (0 != error_code) {
            //LOG(ERROR) << "创建线程失败, error code: " << error_code;
            return false;
        }
        running_.Set();
        
        return true;
    }
    
    void Thread::Join() {
        if (running()) {
            assert(!IsCurrent());
            void *pv;
            pthread_join(thread_, &pv);
            running_.Reset();
        }
    }
    
    void* Thread::PreRun(void* pv) {
        ThreadInit* init = static_cast<ThreadInit*>(pv);
        ThreadManager::Instance()->SetCurrentThread(init->thread);
        
        init->runnable->Run(init->userdata);
        
        delete init;
        return nullptr;
    }
    
    bool Thread::IsOwned() {
        return owned_;
    }
    
    void Thread::Stop() {
        Join();
    }

}
