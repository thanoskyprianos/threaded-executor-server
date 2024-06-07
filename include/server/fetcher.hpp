#pragma once

#include <cstdint>

#include "executor.hpp"

using Executor::Job;

namespace Fetcher {
    uint32_t headers(int sock);         // fetches commander command (specified in codes.hpp)
    
    Job *issueJob(int sock);            // fetches job args if headers = ISSUE_JOB
    uint32_t setConcurrency(int sock);  // fetches new concurrency if headers = SET_CONCURRENCY
    uint32_t stop(int sock);            // fetches jobId if headers = STOP_JOB
}