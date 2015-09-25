//
//  RWLock.cpp
//  Frame
//
//  Created by chenjianjun on 15/9/9.
//  Copyright (c) 2015年 jsbn. All rights reserved.
//

#include "RWLock.h"

namespace NAME_SPACE {
    
    RWLock::RWLock():lock_() {
    }
    
    RWLock::~RWLock() {
        pthread_rwlock_destroy(&lock_);
    }
    
    RWLock* RWLock::Create() {
        RWLock* ret_val = new RWLock();
        if (!ret_val->Init()) {
            delete ret_val;
            return nullptr;
        }
        return ret_val;
    }
    
    bool RWLock::Init() {
        return pthread_rwlock_init(&lock_, 0) == 0;
    }
    
    void RWLock::AcquireLockExclusive() {
        pthread_rwlock_wrlock(&lock_);
    }
    
    void RWLock::ReleaseLockExclusive() {
        pthread_rwlock_unlock(&lock_);
    }
    
    void RWLock::AcquireLockShared() {
        pthread_rwlock_rdlock(&lock_);
    }
    
    void RWLock::ReleaseLockShared() {
        pthread_rwlock_unlock(&lock_);
    }
    
}
