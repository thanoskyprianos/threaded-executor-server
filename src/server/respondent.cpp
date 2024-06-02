#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#include "executor.hpp"
#include "respondent.hpp"
#include "sync.hpp"

using Executor::Job;

void Respondent::issueJob(int sock, Job *j) {
    using namespace Executor::internal;

    (void)sock;
    (void)j;

    // don't close socket, workers will do it
}

void Respondent::poll(int sock) {
    using namespace Executor::internal;

    pthread_mutex_lock(&Mutex::jobBuffer);

    for (const auto &job : jobsBuffer) {
        string triplet = job.second->triplet();
        size_t len = triplet.size();
        uint32_t lenToNet = ntohl(len);

        if (write(sock, &lenToNet, sizeof(uint32_t)) == -1 ||
            write(sock, triplet.c_str(), len) == -1) {
                break;
        }
    }

    pthread_mutex_unlock(&Mutex::jobBuffer);

    shutdown(sock, SHUT_RDWR);
    close(sock);
}