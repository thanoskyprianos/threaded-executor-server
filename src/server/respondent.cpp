#include <ios>
#include <string>
#include <sstream>

#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#include "executor.hpp"
#include "respondent.hpp"
#include "sync.hpp"

using Executor::Job;
using std::string;
using std::stringstream;

bool sendResponse(int sock, string resp) {
    int len = resp.size();
    uint32_t lenToNet = htonl(len);

    if (write(sock, &lenToNet, sizeof(uint32_t)) == -1 ||
        write(sock, resp.c_str(), len) == -1 ) {
            return false;
    }

    return true;
}

void Respondent::issueJob(int sock, Job *job) {
    stringstream stream { std::ios_base::app | std::ios_base::out };
    stream << "JOB " << job->triplet() << " SUBMITTED";
    
    sendResponse(sock, stream.str());

    // don't close socket, workers will do it
}

void Respondent::setConcurrency(int sock, uint32_t n) {
    stringstream stream { std::ios_base::app | std::ios_base::out };
    stream << "CONCURRENCY SET AT " << n;

    sendResponse(sock, stream.str());

    shutdown(sock, SHUT_RDWR);
    close(sock);
}

void Respondent::stop(int sock, uint32_t jobId, bool exists) {
    stringstream stream { std::ios_base::app | std::ios_base::out };
    stream << "JOB <job_" << jobId << (exists ? "> REMOVED" : "> NOTFOUND");

    sendResponse(sock, stream.str());

    shutdown(sock, SHUT_RDWR);
    close(sock);
}

void Respondent::poll(int sock) {
    using namespace Executor::internal;

    pthread_mutex_lock(&Mutex::jobBuffer);

    for (const auto &job : jobsBuffer) {
        string triplet = job.second->triplet();
        size_t len = triplet.size();
        uint32_t lenToNet = htonl(len);

        if (write(sock, &lenToNet, sizeof(uint32_t)) == -1 ||
            write(sock, triplet.c_str(), len) == -1) {
                break;
        }
    }

    pthread_mutex_unlock(&Mutex::jobBuffer);

    shutdown(sock, SHUT_RDWR);
    close(sock);
}