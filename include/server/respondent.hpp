#pragma once

#include "executor.hpp"

using Executor::Job;

namespace Respondent {
    void issueJob(int sock, Job *j);
    void setConcurrency(int sock, uint32_t n);
    void stop(int sock, Job *j);
    void poll(int sock);
}