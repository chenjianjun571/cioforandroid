//
//  ConditionVariable.h
//  Frame
//
//  Created by chenjianjun on 15/9/9.
//  Copyright (c) 2015年 jsbn. All rights reserved.
//

#ifndef __Frame__ConditionVariable__
#define __Frame__ConditionVariable__

#include <pthread.h>

#include "Common.h"
#include "CriticalSection.h"

namespace NAME_SPACE {
    
    class ConditionVariable {
        
    public:
        static ConditionVariable* Create();
        ~ConditionVariable();
        
        void SleepCS(CriticalSection& crit_sect);
        bool SleepCS(CriticalSection& crit_sect, unsigned long max_time_in_ms);
        void Wake();
        void WakeAll();
        
    private:
        ConditionVariable();
        int Construct();
        
    private:
        pthread_cond_t cond_;
    };
    
}

#endif /* defined(__Frame__ConditionVariable__) */
