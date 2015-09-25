//
//  Thread.h
//  Frame
//
//  Created by chenjianjun on 15/9/2.
//  Copyright (c) 2015年 jsbn. All rights reserved.
//

#ifndef __THREAD_H_
#define __THREAD_H_

#include <pthread.h>
#include <list>
#include <string>
#include <vector>

#include "Common.h"
#include "Event.h"

namespace NAME_SPACE {
    
    class Thread;
    
    class ThreadManager {
    public:
        static const int kForever = -1;
        
        ThreadManager();
        ~ThreadManager();
        
        static ThreadManager* Instance();
        
        Thread* CurrentThread();
        void SetCurrentThread(Thread* thread);
        
    private:
        pthread_key_t key_;
        DISALLOW_COPY_AND_ASSIGN(ThreadManager);
    };
    
    enum ThreadPriority {
        PRIORITY_IDLE = -1,
        PRIORITY_NORMAL = 0,
        PRIORITY_ABOVE_NORMAL = 1,
        PRIORITY_HIGH = 2,
    };
    
    class Runnable {
    public:
        virtual ~Runnable() {}
        virtual void Run(void*) = 0;
        
    protected:
        Runnable() {}
        
    private:
        DISALLOW_COPY_AND_ASSIGN(Runnable);
    };
    
    class Thread {
    public:
        Thread();
        virtual ~Thread() { Stop(); }
        static Thread* Current();
        
        bool IsCurrent() const { return Current() == this;}
        
        /**
         *	@brief	毫秒级的休眠
         *
         *	@param 	millis 	毫秒数
         *
         *	@return	返回false是信号中断
         */
        static bool SleepMs(int millis);
        
        // 可以设置线程的名称用于调试. 必须在Start()之前调用.
        // 如果obj非空的话, 它的值会被追缴到name后面.
        const std::string& name() const { return name_;}
        bool SetName(const std::string& name, const void* obj);
        
        // 设置线程的优先级. 必须在Start()之前调用.
        ThreadPriority priority() const { return priority_; }
        bool SetPriority(ThreadPriority priority);
        
        // 启动线程.
        bool Start(Runnable* runnable,void* userdata=nullptr);
        virtual void Stop();
        
        bool IsOwned();
        
        pthread_t GetPThread() { return thread_; }
        
        bool WrapCurrent();
        void UnwrapCurrent();
        
    protected:
        void Join();
        
    private:
        static void *PreRun(void *pv);
        
        // ThreadManager calls this instead WrapCurrent() because
        // ThreadManager::Instance() cannot be used while ThreadManager is
        // being created.
        // The method tries to get synchronization rights of the thread on Windows if
        // |need_synchronize_access| is true.
        bool WrapCurrentWithThreadManager(ThreadManager* thread_manager);
        
        // Return true if the thread was started and hasn't yet stopped.
        bool running() { return running_.Wait(0); }
        
    private:
        std::string name_;
        ThreadPriority priority_;
        Event running_;  // Signalled means running.
        
        pthread_t thread_;
        bool owned_;
        friend class ThreadManager;
        DISALLOW_COPY_AND_ASSIGN(Thread);
    };
}

#endif /* defined(__THREAD_H_) */
