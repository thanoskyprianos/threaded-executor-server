#include <cerrno>
#include <cstring>
#include <iostream>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "codes.hpp"
#include "commands_c.hpp"

using std::cout;
using std::cerr;
using std::endl;

bool Commands::internal::headers(int sock, uint32_t headerCode) {
    uint32_t toNet = htonl(headerCode);
    if (write(sock, &toNet, sizeof(uint32_t)) == -1) {
        return false;
    }

    return true;
}

bool Commands::issueJob(int sock, int argc, char *argv[]) {
    using namespace Commands::internal;
    
    if (!headers(sock, ISSUE_JOB)) {
        return false;
    }

    for (int i = 0; i < argc; i++) {
        size_t len = strlen(argv[i]);
        uint32_t lenToN = htonl(len);

        if (write(sock, &lenToN, sizeof(uint32_t)) == -1 ||
            write(sock, argv[i], len) == -1) {
                return false;
        }
    }

    return true;
}

bool Commands::setConcurrency(int sock, uint32_t n) {
    using namespace Commands::internal;
    
    if (!headers(sock, SET_CONCURRENCY)) {
        return false;
    }

    uint32_t nToNet = htonl(n);

    if (write(sock, &nToNet, sizeof(uint32_t)) == -1) {
        return false;
    }

    return true;
}

bool Commands::stop(int sock, char *jobId) {
    using namespace Commands::internal;
    
    if (!headers(sock, STOP_JOB)) {
        return false;
    }

    int jobIdNum = atoi(jobId + 4);
    if(jobIdNum <= 0) {
        return false;
    }

    uint32_t jobIdToNet = htonl(jobIdNum);

    if (write(sock, &jobIdToNet, sizeof(uint32_t)) == -1) {
        return false;
    }

    return true;
}

bool Commands::poll(int sock) {
    using namespace Commands::internal;
    
    if (!headers(sock, POLL_JOBS)) {
        return false;
    }

    return true;
}

bool Commands::exit(int sock) {
    using namespace Commands::internal;
    
    if (!headers(sock, EXIT_SERVER)) {
        return false;
    }

    return true;
}