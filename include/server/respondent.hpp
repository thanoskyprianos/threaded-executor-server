#pragma once

#include "executor.hpp"

using Executor::Job;

namespace Respondent {
    void issueJob(int sock, Job *job);
    void setConcurrency(int sock, uint32_t n);
    void stop(int sock, uint32_t jobId, bool exists);
    void poll(int sock);
}