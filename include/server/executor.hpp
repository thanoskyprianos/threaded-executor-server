#pragma once

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
        uint32_t jobId;
        vector<string> args;
        int sock;
    public:
        Job(void);
        Job(const vector<string> &&args, int sock);

        uint32_t getJobId (void) const;
        void setJobId (uint32_t jobId);

        int getSocket (void) const;
        
        string triplet(void) const;
    };

    namespace internal {
        extern uint32_t incJobId;
        extern uint32_t concurrencyLevel;
        extern size_t bufferSize;
        extern map<uint32_t, Job *> jobsBuffer;
    }

    void issueJob(int sock, Job *job);
    void setConcurrency(int sock, uint32_t n);
    void stop(int sock, uint32_t jobId);
}