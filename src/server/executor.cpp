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

#include "executor.hpp"
#include "respondent.hpp"
#include "sync.hpp"

using std::map;
using std::ostream_iterator;
using std::string;
using std::stringstream;
using std::vector;
using Executor::Job;

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

uint32_t Executor::internal::incJobId = 1;
uint32_t Executor::internal::concurrencyLevel = 1;
size_t Executor::internal::bufferSize = 0; // re-assign it on main !
map<uint32_t, Job *> Executor::internal::jobsBuffer;

// returns job with modified job Id back
// calls respondent to avoid races from workers
void Executor::issueJob(int sock, Job *job) {
    using namespace Executor::internal;

    pthread_mutex_lock(&Mutex::jobBufferSize);
        
    while (jobsBuffer.size() >= bufferSize) {
        // DELETE LATER
        cerr << "Waiting for space..." << endl;
        pthread_cond_wait(&Cond::jobBufferSize, &Mutex::jobBufferSize);
    }

    pthread_mutex_lock(&Mutex::jobId);
    pthread_mutex_lock(&Mutex::jobBuffer);

    job->setJobId(incJobId);
    Executor::internal::jobsBuffer[incJobId] = job;
    incJobId++;

    Respondent::issueJob(sock, job);
    
    pthread_mutex_unlock(&Mutex::jobId);
    pthread_mutex_unlock(&Mutex::jobBuffer);

    pthread_mutex_unlock(&Mutex::jobBufferSize);
}


// returns n back
void Executor::setConcurrency(int sock, uint32_t n) {
    using namespace Executor::internal;

    pthread_mutex_lock(&Mutex::concurrency);

    concurrencyLevel = n;
    Respondent::setConcurrency(sock, n);

    pthread_mutex_unlock(&Mutex::concurrency);
    
    // broadcast to workers...?
}

// nullptr on failure
// removed Job on success
void Executor::stop(int sock, uint32_t jobId) {
    using namespace Executor::internal;
    Job *removed;

    pthread_mutex_lock(&Mutex::jobBuffer);

    auto ptr = jobsBuffer.find(jobId);
    if (ptr == jobsBuffer.end()) {
        Respondent::stop(sock, jobId, false);
    } else {
        // move job before removing it from map
        removed = new Job { std::move(*ptr->second) };
        jobsBuffer.erase(ptr);

        Respondent::stop(sock, jobId, true);

        delete removed;
    }

    pthread_mutex_unlock(&Mutex::jobBuffer);
        
    // broadcast to commanders...?
}