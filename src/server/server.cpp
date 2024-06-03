#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <ios>
#include <iostream>
#include <sstream>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "codes.hpp"
#include "executor.hpp"
#include "fetcher.hpp"
#include "respondent.hpp"
#include "sync.hpp"

using Executor::Job;
using std::cout;
using std::cerr;
using std::endl;
using std::stringstream;

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
    using namespace Executor::internal;
    (void)arg;

    Job *j;
    
    while (running) {
        // DEL_THIS
        cerr << "Thread: " << pthread_self() << " running again." << endl;

        pthread_mutex_lock(&Mutex::jobBuffer);
        while (jobsBuffer.empty()) {
            pthread_cond_wait(&Cond::jobBuffer, &Mutex::jobBuffer);
        }
        
        pthread_mutex_lock(&Mutex::concurrency);

        if (runningJobs >= concurrencyLevel) {
            pthread_mutex_unlock(&Mutex::jobBuffer);
            pthread_mutex_unlock(&Mutex::concurrency);
            continue; // try again
        }

        pthread_mutex_unlock(&Mutex::concurrency);

        j = Executor::next();
        pthread_mutex_unlock(&Mutex::jobBuffer);
        
        if (j == nullptr) {
            continue;
        }
        
        // let some commander know there's space
        pthread_cond_broadcast(&Cond::jobBuffer);

        pid_t pid; 
        int fd;
        stringstream stream { std::ios_base::app | std::ios_base::out };
        char * const* argv;
        
        switch (pid = fork()) {
            case -1:
                close(j->getSocket());
                break;
            case 0:
                stream << OUTPUT_DIR << getpid() << ".output";

                fd = open(stream.str().c_str(), O_WRONLY | O_CREAT, 0644);
                dup2(fd, STDOUT_FILENO);
                close(fd);

                argv = j->c_array();

                cerr << "Executing: " << j->triplet() << endl;

                if (execvp(argv[0], argv) == -1) {
                    close(j->getSocket());
                }

                break;
            default:
                pthread_mutex_lock(&Mutex::concurrency);
                runningJobs++;
                pthread_mutex_unlock(&Mutex::concurrency);
                
                wait(NULL);

                pthread_mutex_lock(&Mutex::concurrency);
                runningJobs--;
                pthread_mutex_unlock(&Mutex::concurrency);

                // send output
                Respondent::jobOutput(j->getSocket(), pid, j);

                stream.str("");
                stream << OUTPUT_DIR << pid << ".output";
                remove(stream.str().c_str());

                break;
        }

        delete j;        
    }

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

            break;
        case STOP_JOB:
            if ((jobId = Fetcher::stop(sock)) == 0) {
                close(sock);
                break;
            }

            Executor::stop(sock, jobId);
            break;
        case POLL_JOBS:
            Respondent::poll(sock);
            break;

        case EXIT_SERVER:
            break;

        case ERROR_COMMAND:
            close(sock);
            return nullptr;
            break;
    }

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
    Mutex::init_mtxs({&Mutex::jobBuffer, &Mutex::concurrency});
    Cond::init_conds({&Cond::jobBuffer});
    
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

    Mutex::destroy_mtxs({&Mutex::jobBuffer, &Mutex::concurrency});
    Cond::destroy_conds({&Cond::jobBuffer});

    return 0;
}