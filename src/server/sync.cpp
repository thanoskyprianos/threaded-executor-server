#include <initializer_list>

#include <pthread.h>

#include "sync.hpp"

using std::initializer_list;

pthread_mutex_t Mutex::jobBuffer = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Mutex::concurrency = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Mutex::running = PTHREAD_MUTEX_INITIALIZER;

void Mutex::destroy_mtxs(initializer_list<pthread_mutex_t *> mtxs) {
    for (auto &mtx: mtxs) {
        pthread_mutex_destroy(mtx);
    }
}

pthread_cond_t Cond::jobBufferWorker = PTHREAD_COND_INITIALIZER;
pthread_cond_t Cond::jobBufferCommander = PTHREAD_COND_INITIALIZER;

void Cond::destroy_conds(initializer_list<pthread_cond_t *> conds) {
    for (auto &mtx: conds) {
        pthread_cond_destroy(mtx);
    }
}