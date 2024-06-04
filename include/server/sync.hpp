#pragma once

#include <initializer_list>

#include <pthread.h>

using std::initializer_list;

namespace Mutex {
    // used for buffer checking and if the server is still running
    extern pthread_mutex_t runtime;

    // used to check concurrency safely
    extern pthread_mutex_t concurrency;

    void destroy_mtxs(initializer_list<pthread_mutex_t *> mtxs);
}

namespace Cond {
    extern pthread_cond_t runtimeWorker;
    extern pthread_cond_t runtimeCommander;

    void destroy_conds(initializer_list<pthread_cond_t *> conds);
}