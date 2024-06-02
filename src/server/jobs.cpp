#include <cstdint>
#include <ios>
#include <iterator>
#include <map>
#include <string>
#include <sstream>
#include <vector>

#include <arpa/inet.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>

#include "jobs.hpp"
#include "sync.hpp"

using std::map;
using std::ostream_iterator;
using std::string;
using std::stringstream;
using std::vector;
using Jobs::Job;

// DELETE LATER
#include <iostream>
using std::cerr;
using std::endl;
// DELETE LATER


Job::Job() : jobId(-1), sock(-1) { }
Job::Job(const vector<string> &&args, int sock) 
    : args(args), sock(sock) { }

uint32_t Job::getJobId(void) const { return this->jobId; }
void Job::setJobId(uint32_t jobId) { this->jobId = jobId; }

int Job::getSocket(void) const { return this->sock; }

string Job::triplet() const {
    stringstream res { std::ios_base::app | std::ios_base::out };
    res << "<job_" << std::to_string(jobId) << ", ";

    // convert args vector to space seperated string
    stringstream args;
    std::copy(this->args.begin(), this->args.end(), ostream_iterator<string>(args, " "));
    string argsStr = args.str();

    res << argsStr.substr(0, argsStr.size() - 1) << ">";

    return res.str();
}

uint32_t Jobs::internal::incJobId = 1;
uint32_t Jobs::internal::concurrencyLevel = 1;
size_t Jobs::internal::bufferSize = 0; // re-assign it on main !
map<uint32_t, Job *> Jobs::internal::jobsBuffer;

void Jobs::issueJob(Job *job) {
    using namespace Jobs::internal;

    pthread_mutex_lock(&Mutex::jobBufferSize);
        
    while (jobsBuffer.size() >= bufferSize) {
        // DELETE LATER
        cerr << "Waiting for space..." << endl;
        pthread_cond_wait(&Cond::jobBufferSize, &Mutex::jobBufferSize);
    }

    pthread_mutex_lock(&Mutex::jobId);
    pthread_mutex_lock(&Mutex::jobBuffer);

    job->setJobId(incJobId);
    Jobs::internal::jobsBuffer[incJobId] = job;
    incJobId++;
    
    pthread_mutex_unlock(&Mutex::jobId);
    pthread_mutex_unlock(&Mutex::jobBuffer);

    pthread_mutex_unlock(&Mutex::jobBufferSize);
}

void Jobs::setConcurrency(uint32_t n) {
    using namespace Jobs::internal;

    pthread_mutex_lock(&Mutex::concurrency);

    concurrencyLevel = n;
    // maybe broadcast to workers...?

    pthread_mutex_unlock(&Mutex::concurrency);
}

// // false -> not found
// // true -> found and removed
// bool Jobs::stop(int jobId) {
//     using namespace Jobs::internal;

//     auto ptr = jobsBuffer.find(jobId);
//     if (ptr != jobsBuffer.end()) {
//         jobsBuffer.erase(ptr);
//         return true;
//     }

//     return false;
// }

// // false -> error while writing
// // true -> success
// bool Jobs::poll(int fd) {
//     using namespace Jobs::internal;

//     for (const auto &ptr : jobsBuffer) {
//         string triplet = ptr.second.triplet();
//         uint32_t len = htonl(triplet.size());

//         if (write(fd, &len, sizeof(uint32_t)) == -1 ||
//             write(fd, triplet.c_str(), len) == -1) {
//                 return false;
//         }
//     }

//     return true;
// }