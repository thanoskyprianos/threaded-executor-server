#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "codes.hpp"
#include "executor.hpp"
#include "fetcher.hpp"
#include "sync.hpp"

using std::cout;
using std::cerr;
using std::endl;
using Executor::Job;

static bool running = true;
pthread_t *workers;

inline void terror(char *errorStr, int errorCode) {
    cerr << errorStr << ((errorStr != nullptr) ? ":" : "") << strerror(errorCode) << endl;
}

// returns passive endpoint communication socket
int createPassiveEndpoint(uint16_t port) {
    int sock;
    struct sockaddr_in allAddr;

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error while creating socket");
        exit(NETWORK_ERROR);
    }

    allAddr.sin_family = AF_INET;
    allAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    allAddr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *)&allAddr, sizeof(allAddr)) == -1) {
        perror("Error while binding socket");
        exit(NETWORK_ERROR);
    }

    if (listen(sock, SOMAXCONN) == -1) {
        perror("Error while making socket passive");
        exit(NETWORK_ERROR);
    }

    return sock;
}

void *workerRoutine(void *arg) {
    (void)arg;
    // do worker stuff

    return nullptr;
}

void *commanderRoutine(void *arg) {
    int sock = *(int *)arg;
    delete (int *)arg;
    
    Job *j;
    uint32_t conc, jobId;

    uint32_t command = Fetcher::headers(sock);
    switch(command) {
        case ISSUE_JOB:
            if ((j = Fetcher::issueJob(sock)) == nullptr) {
                close(sock);
                break;
            }

            Executor::issueJob(sock, j);
            break;
        case SET_CONCURRENCY:
            if((conc = Fetcher::setConcurrency(sock)) == 0) {
                close(sock);
                break;
            }

            Executor::setConcurrency(sock, conc);

            //DEL_THIS
            cerr << "Set concurrency to: " << Executor::internal::concurrencyLevel << endl;

            break;
        case STOP_JOB:
            if ((jobId = Fetcher::stop(sock)) == 0) {
                close(sock);
                break;
            }

            Executor::stop(sock, jobId);
            break;
        case POLL_JOBS:
            // Jobs::poll(sock);

            break;

        case EXIT_SERVER:
            break;

        case ERROR_COMMAND:
            close(sock);
            return nullptr;
            break;
    }

    //DEL_THIS
    cout << "Thread " << pthread_self() << " finished execution" << endl;
    return nullptr;
}

int main(int argc, char *argv[]) {
    int port = 0, bufferSize = 0, threadPool = 0, passiveEndpoint;

    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " [portNum] [bufferSize] [threadPoolSize]" << endl;
        exit(USAGE_ERROR);
    }
    
    if ((port = atoi(argv[1])) <= 0) {
        cerr << "Port should be a positive integer. Port: " << argv[1] << endl;
        exit(VALUE_ERROR);
    }

    if ((bufferSize = atoi(argv[2])) <= 0) {
        cerr << "Buffer size should be a positive integer. Buffer size: " << argv[2] << endl;
        exit(VALUE_ERROR);
    }

    Executor::internal::bufferSize = bufferSize;

    if ((threadPool = atoi(argv[3])) <= 0) {
        cerr << "Thread pool size should be a positive integer. Thread pool size: " << argv[3] << endl;
        exit(VALUE_ERROR);
    }

    passiveEndpoint = createPassiveEndpoint(port);

    // create worker threads
    workers = new pthread_t[threadPool];
    for (int i = 0; i < threadPool; i++) {
        int errorCode;
        if ((errorCode = pthread_create(workers + i, NULL, &workerRoutine, NULL)) != 0) {
            terror((char *)"Error while creating worker thread", errorCode);
            exit(THREAD_ERROR);
        }
    }

    // init sync
    Mutex::init_mtxs({&Mutex::jobBuffer, &Mutex::jobBufferSize, &Mutex::jobId, &Mutex::concurrency});
    Cond::init_conds({&Cond::jobBuffer, &Cond::jobBufferSize});
    
    // detachable thread attribute
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    while (running) {
        int *commEndpoint = new int();
        sockaddr_in client;
        socklen_t clientLen = 0;
        pthread_t commander;
        int errorCode;

        if ((*commEndpoint = accept(passiveEndpoint, (sockaddr *)&client, &clientLen)) == -1) {
            cerr << "Error while accepting connection" << endl;
            continue; // wait for other clients
        }
        
        if((errorCode = pthread_create(&commander, &attr, &commanderRoutine, (void *)commEndpoint)) != 0) {
            terror((char *)"Error while creating commander thread", errorCode);
            exit(THREAD_ERROR);
        }
    }

    // join worker threads
    for (int i = 0; i < threadPool; i++) {
        int errorCode;
        
        if((errorCode = pthread_join(workers[i], nullptr)) != 0) {
            terror((char *)"Error while joining thread", errorCode);
            exit(THREAD_ERROR);
        }
    }

    Mutex::destroy_mtxs({&Mutex::jobBuffer, &Mutex::jobBufferSize, &Mutex::jobId, &Mutex::concurrency});
    Cond::destroy_conds({&Cond::jobBuffer, &Cond::jobBufferSize});

    return 0;
}