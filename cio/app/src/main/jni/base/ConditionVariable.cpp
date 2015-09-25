//
//  ConditionVariable.cpp
//  Frame
//
//  Created by chenjianjun on 15/9/9.
//  Copyright (c) 2015年 jsbn. All rights reserved.
//
#include "ConditionVariable.h"

namespace NAME_SPACE {
    
    ConditionVariable* ConditionVariable::Create()
    {
        ConditionVariable* ptr = new ConditionVariable;
        if (!ptr) {
            return nullptr;
        }
        
        const int error = ptr->Construct();
        if (error) {
            delete ptr;
            return nullptr;
        }
        
        return ptr;
    }
    
    ConditionVariable::ConditionVariable()
    {}
    
    int ConditionVariable::Construct()
    {
#if defined(CIO_IOS)
        pthread_cond_init(&cond_, nullptr);
#elif defined(CIO_ANDROID)
        int result = 0;
        pthread_condattr_t cond_attr;
        result = pthread_condattr_init(&cond_attr);
        if (result != 0) {
            return -1;
        }
        result = pthread_condattr_setclock(&cond_attr, CLOCK_MONOTONIC);
        if (result != 0) {
            return -1;
        }
        result = pthread_cond_init(&cond_, &cond_attr);
        if (result != 0) {
            return -1;
        }
        result = pthread_condattr_destroy(&cond_attr);
        if (result != 0) {
            return -1;
        }
#endif
        
        return 0;
    }
    
    ConditionVariable::~ConditionVariable() {
        pthread_cond_destroy(&cond_);
    }
    
    void ConditionVariable::SleepCS(CriticalSection& crit_sect) {
        CriticalSection* cs = reinterpret_cast<CriticalSection*>(&crit_sect);
        pthread_cond_wait(&cond_, &cs->mutex_);
    }
    
    bool ConditionVariable::SleepCS(CriticalSection& crit_sect, unsigned long max_time_inMS)
    {
        const unsigned long INFINITE =  0xFFFFFFFF;
        const int MILLISECONDS_PER_SECOND = 1000;
#if defined(CIO_IOS)
        const int MICROSECONDS_PER_MILLISECOND = 1000;
#endif
        const int NANOSECONDS_PER_SECOND = 1000000000;
        const int NANOSECONDS_PER_MILLISECOND  = 1000000;
        
        CriticalSection* cs = reinterpret_cast<CriticalSection*>(&crit_sect);
        if (max_time_inMS != INFINITE)
        {
            timespec ts;
#if defined(CIO_ANDROID)
            clock_gettime(CLOCK_MONOTONIC, &ts);
#elif defined(CIO_IOS)
            struct timeval tv;
            gettimeofday(&tv, 0);
            ts.tv_sec  = tv.tv_sec;
            ts.tv_nsec = tv.tv_usec * MICROSECONDS_PER_MILLISECOND;
#endif

            ts.tv_sec += max_time_inMS / MILLISECONDS_PER_SECOND;
            ts.tv_nsec +=
            (max_time_inMS
             - ((max_time_inMS / MILLISECONDS_PER_SECOND) * MILLISECONDS_PER_SECOND))
            * NANOSECONDS_PER_MILLISECOND;
            
            if (ts.tv_nsec >= NANOSECONDS_PER_SECOND) {
                ts.tv_sec += ts.tv_nsec / NANOSECONDS_PER_SECOND;
                ts.tv_nsec %= NANOSECONDS_PER_SECOND;
            }
            const int res = pthread_cond_timedwait(&cond_, &cs->mutex_, &ts);
            return (res == ETIMEDOUT) ? false : true;
        } else {
            pthread_cond_wait(&cond_, &cs->mutex_);
            return true;
        }

    }
    
    void ConditionVariable::Wake() {
        pthread_cond_signal(&cond_);
    }
    
    void ConditionVariable::WakeAll() {
        pthread_cond_broadcast(&cond_);
    }
    
}
