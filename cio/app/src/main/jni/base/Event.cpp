//
//  Event.cpp
//  Frame
//
//  Created by chenjianjun on 15/9/2.
//  Copyright (c) 2015年 jsbn. All rights reserved.
//

#include "Event.h"

namespace NAME_SPACE {
    
    Event::Event(bool manual_reset, bool initially_signaled)
    :is_manual_reset_(manual_reset), event_status_(initially_signaled) {
        pthread_mutex_init(&event_mutex_, nullptr);
        pthread_cond_init(&event_cond_, nullptr);
    }
    
    Event::~Event() {
        pthread_mutex_destroy(&event_mutex_);
        pthread_cond_destroy(&event_cond_);
    }
    
    void Event::Set() {
        pthread_mutex_lock(&event_mutex_);
        event_status_ = true;
        pthread_cond_broadcast(&event_cond_);
        pthread_mutex_unlock(&event_mutex_);
    }
    
    void Event::Reset() {
        pthread_mutex_lock(&event_mutex_);
        event_status_ = false;
        pthread_mutex_unlock(&event_mutex_);
    }
    
    bool Event::Wait(int milliseconds)
    {
        pthread_mutex_lock(&event_mutex_);
        int error = 0;
        
        if (milliseconds != kForever)
        {
            struct timespec ts;
#if HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE
            ts.tv_sec = milliseconds / 1000;
            ts.tv_nsec = (milliseconds % 1000) * 1000000;
#else
            struct timeval tv;
            gettimeofday(&tv, NULL);
            
            ts.tv_sec = tv.tv_sec + (milliseconds / 1000);
            ts.tv_nsec = tv.tv_usec * 1000 + (milliseconds % 1000) * 1000000;
            
            // Handle overflow.
            if (ts.tv_nsec >= 1000000000) {
                ts.tv_sec++;
                ts.tv_nsec -= 1000000000;
            }
#endif
            while (!event_status_ && error == 0)
            {
#if HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE
                error = pthread_cond_timedwait_relative_np(&event_cond_, &event_mutex_, &ts);
#else
                error = pthread_cond_timedwait(&event_cond_, &event_mutex_, &ts);
#endif
            }
        }
        else
        {
            while (!event_status_ && error == 0)
            {
                error = pthread_cond_wait(&event_cond_, &event_mutex_);
            }
        }
        
        if (error == 0 && !is_manual_reset_)
            event_status_ = false;
        
        pthread_mutex_unlock(&event_mutex_);
        
        return (error == 0);
    }
    
}
