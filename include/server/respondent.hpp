#pragma once

#include "executor.hpp"

using Executor::Job;

namespace Respondent {
    namespace internal {
        bool sendResponse(int sock, string resp);   // sends: resp.size(), resp
    }

    // following are called by Executor functions
    void issueJob(int sock, Job *job);
    void setConcurrency(int sock, uint32_t n);
    void stop(int sock, uint32_t jobId, bool exists);

    // standalone
    void poll(int sock);

    // called by worker after j finishes
    void jobOutput(int sock, int pid, Job *j);

    // standalone
    void exit(int sock);
}