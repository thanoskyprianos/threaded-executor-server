#pragma once

#define OUTPUT_DIR "/tmp/"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

using std::map;
using std::string;
using std::vector;

namespace Executor {
    struct Job {
    private:
        uint32_t jobId;      // jobId (assigned after job gets in buffer)
        vector<string> args; // cli arguments
        int sock;            // save it to send output later
    public:
        Job(void);
        Job(const vector<string> &&args, int sock);

        uint32_t getJobId(void) const;
        void setJobId(uint32_t jobId);

        int getSocket(void) const;
        
        string triplet(void) const;

        char * const *c_array(size_t &size) const; // used by execvp

        // send message to commander commander is job did not run
        void terminate(bool serverShutdown);
    };

    namespace internal {
        extern bool running;                    // server is running
        extern uint32_t runningJobs;            // number of running jobs
        extern uint32_t incJobId;               // increasing job Id
        extern uint32_t concurrencyLevel;       // concurrency level
        extern size_t bufferSize;               // size of buffer for jobs
        extern map<uint32_t, Job *> jobsBuffer; // the buffer
    }

    void issueJob(int sock, Job *job);          // issue job
    void setConcurrency(int sock, uint32_t n);  // set internal::concurencyLevel
    void stop(int sock, uint32_t jobId);        // stop queued job with jobId

    Job *next(void); // next job in buffer (fifo)
}