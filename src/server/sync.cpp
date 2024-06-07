#include <initializer_list>

#include <pthread.h>

#include "sync.hpp"

using std::initializer_list;

// intialize mutexes
pthread_mutex_t Mutex::runtime = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Mutex::concurrency = PTHREAD_MUTEX_INITIALIZER;

void Mutex::destroy_mtxs(initializer_list<pthread_mutex_t *> mtxs) {
    for (auto &mtx: mtxs) {
        pthread_mutex_destroy(mtx);
    }
}

// initialize cond variables
pthread_cond_t Cond::runtimeWorker = PTHREAD_COND_INITIALIZER;
pthread_cond_t Cond::runtimeCommander = PTHREAD_COND_INITIALIZER;

void Cond::destroy_conds(initializer_list<pthread_cond_t *> conds) {
    for (auto &mtx: conds) {
        pthread_cond_destroy(mtx);
    }
}