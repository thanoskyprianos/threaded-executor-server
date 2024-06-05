#include <cstdint>
#include <cstring>
#include <ios>
#include <iterator>
#include <map>
#include <string>
#include <sstream>
#include <vector>

#include <arpa/inet.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
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

// sets size on return
char * const *Job::c_array(size_t &size) const {
    char **carr = new char*[args.size() + 1];
    size = 0;

    for (const auto &str : args) {
        char *buf = new char[str.size() + 1];
        strcpy(buf, str.c_str());
        carr[size++] = buf;
    }

    carr[size] = NULL; // sentinel

    return const_cast<char * const *>(carr);
}

void Job::terminate(bool serverShutdown) {
    string resp;
    
    if (serverShutdown)
        resp = "SERVER TERMINATED BEFORE EXECUTION\n";
    else
        resp = "JOB STOPPED BEFORE EXECUTION\n";

    Respondent::internal::sendResponse(this->sock, resp);
    
    shutdown(this->sock, SHUT_RDWR);
    close(this->sock);
}

bool Executor::internal::running = true;
uint32_t Executor::internal::runningJobs = 0;
uint32_t Executor::internal::incJobId = 1;
uint32_t Executor::internal::concurrencyLevel = 1;
size_t Executor::internal::bufferSize = 0; // re-assign it on main !
map<uint32_t, Job *> Executor::internal::jobsBuffer;


// returns job with modified job Id back
// calls respondent to avoid races from workers
void Executor::issueJob(int sock, Job *job) {
    using namespace Executor::internal;

    // del
    pthread_mutex_lock(&Mutex::runtime);
        
    while (jobsBuffer.size() == bufferSize && running) {
        pthread_cond_wait(&Cond::runtimeCommander, &Mutex::runtime);
    }

    if (!running) {
        job->terminate(true);
        pthread_mutex_unlock(&Mutex::runtime);
        return;
    }

    job->setJobId(incJobId);
    Executor::internal::jobsBuffer[incJobId] = job;
    incJobId++;

    Respondent::issueJob(sock, job);
    
    pthread_mutex_unlock(&Mutex::runtime);

    // let workers know they can access the buffer
    pthread_cond_broadcast(&Cond::runtimeWorker);
}


// returns n back
void Executor::setConcurrency(int sock, uint32_t n) {
    using namespace Executor::internal;

    pthread_mutex_lock(&Mutex::concurrency);

    concurrencyLevel = n;
    Respondent::setConcurrency(sock, n);

    pthread_mutex_unlock(&Mutex::concurrency);
}

// nullptr on failure
// removed Job on success
void Executor::stop(int sock, uint32_t jobId) {
    using namespace Executor::internal;
    Job *removed;

    pthread_mutex_lock(&Mutex::runtime);

    auto ptr = jobsBuffer.find(jobId);
    if (ptr == jobsBuffer.end()) {
        Respondent::stop(sock, jobId, false);
        pthread_mutex_unlock(&Mutex::runtime);
    } else {
        removed = ptr->second;
        jobsBuffer.erase(ptr);

        pthread_mutex_unlock(&Mutex::runtime);

        // let commanders know there is space
        pthread_cond_broadcast(&Cond::runtimeCommander);

        removed->terminate(false);

        Respondent::stop(sock, jobId, true);

        delete removed;
    }
}

Job *Executor::next(void) {
    using namespace Executor::internal;

    Job *j;

    auto ptr = jobsBuffer.begin();
    if (ptr == jobsBuffer.end()) {
        return nullptr;
    }

    j = ptr->second;
    jobsBuffer.erase(ptr);

    return j;
}