//
//  CriticalSection.h
//  Frame
//
//  Created by chenjianjun on 15/8/31.
//  Copyright (c) 2015年 jsbn. All rights reserved.
//

#ifndef __CRITICAL_SEXTION_H_
#define __CRITICAL_SEXTION_H_

#include "Common.h"

namespace NAME_SPACE {
    
    class CriticalSection {
    public:
        CriticalSection();
        ~CriticalSection();
        
        void Enter();
        void Leave();
        
    private:
        pthread_mutex_t mutex_;
        friend class ConditionVariable;
    };

    class CriticalSectionScoped {
    public:
        explicit CriticalSectionScoped(CriticalSection* critsec)
        : ptr_crit_sec_(critsec) {
            ptr_crit_sec_->Enter();
        }
        
        ~CriticalSectionScoped() { ptr_crit_sec_->Leave(); }
        
    private:
        CriticalSection* ptr_crit_sec_;
    };
}

#endif /* defined(__Frame__CriticalSection__) */
