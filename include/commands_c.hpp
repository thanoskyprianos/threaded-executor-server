#pragma once

#include <cstdint>

namespace Commands {
    namespace internal {
        bool headers(int sock, uint32_t headerCode);
    }

    // false -> failure
    // true -> success
    bool issueJob(int sock, int argc, char *argv[]);
    bool setConcurrency(int sock, uint32_t n);
    bool stop(int sock, char *jobId);
    bool poll(int sock);
    bool exit(int sock);
}