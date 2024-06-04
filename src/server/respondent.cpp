#include <ios>
#include <cstring>
#include <string>
#include <sstream>

#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "executor.hpp"
#include "respondent.hpp"
#include "sync.hpp"

using Executor::Job;
using std::string;
using std::stringstream;

bool Respondent::internal::sendResponse(int sock, string resp) {
    int len = resp.size();
    uint32_t lenToNet = htonl(len);

    if (write(sock, &lenToNet, sizeof(uint32_t)) == -1 ||
        write(sock, resp.c_str(), len) == -1 ) {
            return false;
    }

    return true;
}

void Respondent::issueJob(int sock, Job *job) {
    using namespace Respondent::internal;

    stringstream stream { std::ios_base::app | std::ios_base::out };
    stream << "JOB " << job->triplet() << " SUBMITTED\n";
    
    sendResponse(sock, stream.str());

    // don't close socket, workers will do it
}

void Respondent::setConcurrency(int sock, uint32_t n) {
    using namespace Respondent::internal;
    
    stringstream stream { std::ios_base::app | std::ios_base::out };
    stream << "CONCURRENCY SET AT " << n << '\n';

    sendResponse(sock, stream.str());

    shutdown(sock, SHUT_RDWR);
    close(sock);
}

void Respondent::stop(int sock, uint32_t jobId, bool exists) {
    using namespace Respondent::internal; 

    stringstream stream { std::ios_base::app | std::ios_base::out };
    stream << "JOB <job_" << jobId << (exists ? "> REMOVED\n" : "> NOTFOUND\n");

    sendResponse(sock, stream.str());

    shutdown(sock, SHUT_RDWR);
    close(sock);
}

void Respondent::poll(int sock) {
    using namespace Executor::internal;

    pthread_mutex_lock(&Mutex::runtime);

    for (const auto &job : jobsBuffer) {
        string triplet = job.second->triplet() + '\n';
        size_t len = triplet.size();
        uint32_t lenToNet = htonl(len);

        if (write(sock, &lenToNet, sizeof(uint32_t)) == -1 ||
            write(sock, triplet.c_str(), len) == -1) {
                break;
        }
    }

    pthread_mutex_unlock(&Mutex::runtime);

    shutdown(sock, SHUT_RDWR);
    close(sock);
}
    #include <iostream>

void Respondent::jobOutput(int sock, int pid, Job *j) {
    using namespace Respondent::internal;

    stringstream stream { std::ios_base::app | std::ios_base::out };
    stream << OUTPUT_DIR << pid << ".output";

    int fd = open(stream.str().c_str(), O_RDONLY);
    if (fd != -1) {
        stream.str("");
        stream << "-----job_" << j->getJobId() << " output start------\n\n";

        if(sendResponse(sock, stream.str())) {
            char *buf = new char[64]();
            ssize_t bytes;

            while((bytes = read(fd, buf, 64 - 1)) > 0) {
                if(!sendResponse(sock, string { buf })) {
                    break;
                }

                memset(buf, 0, 64);
            }

            stream.str("");
            stream << "\n-----job_" << j->getJobId() << " output end------\n";
        }

        sendResponse(sock, stream.str());
    }

    shutdown(sock, SHUT_RDWR);
    close(sock);
}

void Respondent::exit(int sock) {
    using namespace Respondent::internal;

    string resp { "SERVER TERMINATED\n" };
    sendResponse(sock, resp);

    shutdown(sock, SHUT_RDWR);
    close(sock);
}