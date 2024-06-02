#pragma once

#include <cstdint>

#include "executor.hpp"

using Executor::Job;

namespace Fetcher {
    uint32_t headers(int sock);
    
    Job *issueJob(int sock);
    uint32_t setConcurrency(int sock);
    uint32_t stop(int sock);
}