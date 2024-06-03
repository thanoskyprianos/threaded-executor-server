#include <initializer_list>

#include <pthread.h>

#include "sync.hpp"

using std::initializer_list;

pthread_mutex_t Mutex::jobBuffer;
pthread_mutex_t Mutex::concurrency;
pthread_mutex_t Mutex::running;

void Mutex::init_mtxs(initializer_list<pthread_mutex_t *> mtxs) {
    for (auto &mtx: mtxs) {
        pthread_mutex_init(mtx, nullptr);
    }
}

void Mutex::destroy_mtxs(initializer_list<pthread_mutex_t *> mtxs) {
    for (auto &mtx: mtxs) {
        pthread_mutex_destroy(mtx);
    }
}

pthread_cond_t Cond::jobBuffer;

void Cond::init_conds(initializer_list<pthread_cond_t *> conds) {
    for (auto &mtx: conds) {
        pthread_cond_init(mtx, nullptr);
    }
}

void Cond::destroy_conds(initializer_list<pthread_cond_t *> conds) {
    for (auto &mtx: conds) {
        pthread_cond_destroy(mtx);
    }
}