//
//  Sigslot.cpp
//  Frame
//
//  Created by chenjianjun on 15/9/1.
//  Copyright (c) 2015年 jsbn. All rights reserved.
//

#include "Sigslot.h"

namespace sigslot {
    
    multi_threaded_global::multi_threaded_global() {
        pthread_mutex_init(get_mutex(), nullptr);
    }
    
    multi_threaded_global::multi_threaded_global(const multi_threaded_global&) {
    }
    
    multi_threaded_global::~multi_threaded_global() = default;
    
    void multi_threaded_global::lock() {
        pthread_mutex_lock(get_mutex());
    }
    
    void multi_threaded_global::unlock() {
        pthread_mutex_unlock(get_mutex());
    }
    
    multi_threaded_local::multi_threaded_local() {
        pthread_mutex_init(&m_mutex, nullptr);
    }
    
    multi_threaded_local::multi_threaded_local(const multi_threaded_local&) {
        pthread_mutex_init(&m_mutex, nullptr);
    }
    
    multi_threaded_local::~multi_threaded_local() {
        pthread_mutex_destroy(&m_mutex);
    }
    
    void multi_threaded_local::lock() {
        pthread_mutex_lock(&m_mutex);
    }
    
    void multi_threaded_local::unlock() {
        pthread_mutex_unlock(&m_mutex);
    }
}
