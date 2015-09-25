//
//  RWLock.h
//  Frame
//
//  Created by chenjianjun on 15/9/9.
//  Copyright (c) 2015年 jsbn. All rights reserved.
//

#ifndef __Frame__RWLock__
#define __Frame__RWLock__

#include "Common.h"

namespace NAME_SPACE {
    
    class RWLock
    {
    public:
        static RWLock* Create();
        ~RWLock();
        
        void AcquireLockExclusive();
        void ReleaseLockExclusive();
        
        void AcquireLockShared();
        void ReleaseLockShared();
        
    private:
        RWLock();
        bool Init();
        
        pthread_rwlock_t lock_;
    };
    
    class ReadLockScoped
    {
    public:
        ReadLockScoped(RWLock& rw_lock):rw_lock_(rw_lock) {
            rw_lock_.AcquireLockShared();
        }
        
        ~ReadLockScoped() {
            rw_lock_.ReleaseLockShared();
        }
        
    private:
        RWLock& rw_lock_;
    };
    
    class WriteLockScoped
    {
    public:
        WriteLockScoped(RWLock& rw_lock):rw_lock_(rw_lock) {
            rw_lock_.AcquireLockExclusive();
        }
        
        ~WriteLockScoped() {
            rw_lock_.ReleaseLockExclusive();
        }
        
    private:
        RWLock& rw_lock_;
    };
}

#endif /* defined(__Frame__RWLock__) */
