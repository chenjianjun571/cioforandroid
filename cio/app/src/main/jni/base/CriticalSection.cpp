//
//  CriticalSection.cpp
//  Frame
//
//  Created by chenjianjun on 15/8/31.
//  Copyright (c) 2015年 jsbn. All rights reserved.
//

#include "CriticalSection.h"

namespace NAME_SPACE {
    
    CriticalSection::CriticalSection() {
        pthread_mutexattr_t attr;
        (void) pthread_mutexattr_init(&attr);
        (void) pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        (void) pthread_mutex_init(&mutex_, &attr);
    }
    
    CriticalSection::~CriticalSection() {
        (void) pthread_mutex_destroy(&mutex_);
    }
    
    void CriticalSection::Enter() {
        (void) pthread_mutex_lock(&mutex_);
    }
    
    void CriticalSection::Leave() {
        (void) pthread_mutex_unlock(&mutex_);
    }
}
