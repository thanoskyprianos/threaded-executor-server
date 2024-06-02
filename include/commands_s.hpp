#pragma once

#include <cstdint>

#include "jobs.hpp"

using Jobs::Job;

namespace Commands {
    uint32_t headers(int sock);
    
    Job *issueJob(int sock);
    uint32_t setConcurrency(int sock);
    uint32_t stop(int sock);
}