//
//  Event.h
//  Frame
//
//  Created by chenjianjun on 15/9/2.
//  Copyright (c) 2015年 jsbn. All rights reserved.
//

#ifndef __EVENT_H_
#define __EVENT_H_

#include "Common.h"

namespace NAME_SPACE {
    class Event {
    public:
        static const int kForever = -1;
        
        Event(bool manual_reset, bool initially_signaled);
        ~Event();
        
        void Set();
        void Reset();
        
        // Wait for the event to become signaled, for the specified number of
        // |milliseconds|.  To wait indefinetly, pass kForever.
        bool Wait(int milliseconds);
        
    private:
        pthread_mutex_t event_mutex_;
        pthread_cond_t event_cond_;
        const bool is_manual_reset_;
        bool event_status_;
    };
}

#endif /* defined(__EVENT_H_) */
