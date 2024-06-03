#pragma once

#include "executor.hpp"

using Executor::Job;

namespace Respondent {
    namespace internal {
        bool sendResponse(int sock, string resp);
    }

    void issueJob(int sock, Job *job);
    void setConcurrency(int sock, uint32_t n);
    void stop(int sock, uint32_t jobId, bool exists);
    void poll(int sock);

    void jobOutput(int sock, int pid, Job *j);
    void exit(int sock);
}