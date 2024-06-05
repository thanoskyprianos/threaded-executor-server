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

static pthread_t *workers;

// used in termination
static pthread_t main_thread;
static int threadPool = 0;
static int exitSock = -1;

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
    
    while (true) {
        pthread_mutex_lock(&Mutex::runtime);

        while (jobsBuffer.empty() && running) {
            pthread_cond_wait(&Cond::runtimeWorker, &Mutex::runtime);
        }

        if (!running) {
            pthread_mutex_unlock(&Mutex::runtime);
            break;
        }
        
        pthread_mutex_lock(&Mutex::concurrency);

        if (runningJobs == concurrencyLevel) {
            pthread_mutex_unlock(&Mutex::runtime);
            pthread_mutex_unlock(&Mutex::concurrency);
            continue; // try again
        }

        j = Executor::next();

        pthread_mutex_unlock(&Mutex::runtime);
        pthread_mutex_unlock(&Mutex::concurrency);
        
        if (j == nullptr) {
            continue;
        }

        // let commanders know there's space
        pthread_cond_broadcast(&Cond::runtimeCommander);
        
        pid_t pid = fork();
        if (pid == -1) {
            close(j->getSocket());
        } else if (pid == 0) {
            { // handles memory leaks
                int fd;
                char * const* argv; size_t size;
                stringstream stream { std::ios_base::app | std::ios_base::out };
                stream << OUTPUT_DIR << getpid() << ".output";
                fd = open(stream.str().c_str(), O_WRONLY | O_CREAT, 0644);
                dup2(fd, STDOUT_FILENO);
                close(fd);

                argv = j->c_array(size);

                if (execvp(argv[0], argv) == -1) {
                    j->terminate(false);
                }

                cerr << endl;
                cerr << "size:" << size << endl;
                cerr << endl;

                delete j;
                while(size--) delete[] argv[size];
                delete[] argv;
                delete[] workers;
            }

            exit(EXEC_ERROR);
        } else {
            int stat;
            stringstream stream { std::ios_base::app | std::ios_base::out };

            pthread_mutex_lock(&Mutex::concurrency);
            runningJobs++;
            pthread_mutex_unlock(&Mutex::concurrency);
            
            waitpid(pid, &stat, 0);
            
            pthread_mutex_lock(&Mutex::concurrency);
            runningJobs--;
            pthread_mutex_unlock(&Mutex::concurrency);

            // send output
            if (WEXITSTATUS(stat) != EXEC_ERROR)
                Respondent::jobOutput(j->getSocket(), pid, j);

            stream.str("");
            stream << OUTPUT_DIR << pid << ".output";
            unlink(stream.str().c_str());
        }

        delete j;        
    }

    return nullptr;
}

void *commanderRoutine(void *arg) {
    int sock = *(int *)arg;
    
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
            pthread_mutex_lock(&Mutex::runtime);
            Executor::internal::running = false;
            if (exitSock == -1) {
                exitSock = sock;
            }
            pthread_mutex_unlock(&Mutex::runtime);

            pthread_cond_broadcast(&Cond::runtimeWorker);
            pthread_cond_broadcast(&Cond::runtimeCommander);

            pthread_kill(main_thread, SIGUSR1);
            break;

        case ERROR_COMMAND:
            close(sock);
            return nullptr;
            break;
    }

    return nullptr;
}

void terminate (int sig) {
    // handle exits yourself
    if (sig != SIGUSR1) {
        pthread_mutex_lock(&Mutex::runtime);
        Executor::internal::running = false;
        pthread_mutex_unlock(&Mutex::runtime);

        pthread_cond_broadcast(&Cond::runtimeWorker);
        pthread_cond_broadcast(&Cond::runtimeCommander);
    }

    // join worker threads
    for (int i = 0; i < threadPool; i++) {
        int errorCode;
        
        if((errorCode = pthread_join(workers[i], nullptr)) != 0) {
            terror((char *)"Error while joining thread", errorCode);
            exit(THREAD_ERROR);
        }
    }

    Mutex::destroy_mtxs({&Mutex::runtime, &Mutex::concurrency});
    Cond::destroy_conds({&Cond::runtimeWorker, &Cond::runtimeCommander});

    // handle remaining queued jobs
    for (auto &job : Executor::internal::jobsBuffer) {
        job.second->terminate(true);
    }

    delete[] workers;

    if (sig == SIGUSR1) // called by commander
        Respondent::exit(exitSock);

    exit(0);
}

int main(int argc, char *argv[]) {
    int port = 0, passiveEndpoint;

    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " [portNum] [bufferSize] [threadPoolSize]" << endl;
        exit(USAGE_ERROR);
    }
    
    if ((port = atoi(argv[1])) <= 0) {
        cerr << "Port should be a positive integer. Port: " << argv[1] << endl;
        exit(VALUE_ERROR);
    }

    if ((Executor::internal::bufferSize = atoi(argv[2])) <= 0) {
        cerr << "Buffer size should be a positive integer. Buffer size: " << argv[2] << endl;
        exit(VALUE_ERROR);
    }

    if ((threadPool = atoi(argv[3])) <= 0) {
        cerr << "Thread pool size should be a positive integer. Thread pool size: " << argv[3] << endl;
        exit(VALUE_ERROR);
    }

    // exit handling signals
    struct sigaction act;
    bzero(&act, sizeof(act));
    act.sa_handler = terminate;
    sigfillset(&act.sa_mask);       // ignore all signals when handling

    sigaction(SIGUSR1, &act, NULL); // called by exit
    sigaction(SIGINT, &act, NULL);  // ctrl + c

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
    
    // detachable thread attribute
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    main_thread = pthread_self();

    while (true) {
        int commEndpoint;
        sockaddr_in client;
        socklen_t clientLen = 0;
        pthread_t commander;
        int errorCode;

        if ((commEndpoint = accept(passiveEndpoint, (sockaddr *)&client, &clientLen)) == -1) {
            cerr << "Error while accepting connection" << endl;
            continue; // wait for other clients
        }
        
        if((errorCode = pthread_create(&commander, &attr, &commanderRoutine, (void *)&commEndpoint)) != 0) {
            terror((char *)"Error while creating commander thread", errorCode);
            exit(THREAD_ERROR);
        }
    }

    return 0;
}