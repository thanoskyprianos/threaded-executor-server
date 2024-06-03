#pragma once

#include <initializer_list>

#include <pthread.h>

using std::initializer_list;

namespace Mutex {
    extern pthread_mutex_t jobBuffer;
    extern pthread_mutex_t concurrency;
    extern pthread_mutex_t running;

    void init_mtxs(initializer_list<pthread_mutex_t *> mtxs);
    void destroy_mtxs(initializer_list<pthread_mutex_t *> mtxs);
}

namespace Cond {
    extern pthread_cond_t jobBuffer;

    void init_conds(initializer_list<pthread_cond_t *> conds);
    void destroy_conds(initializer_list<pthread_cond_t *> conds);
}