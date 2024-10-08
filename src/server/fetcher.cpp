#include <cstdint>
#include <string>
#include <vector>

#include <arpa/inet.h>
#include <unistd.h>

#include "codes.hpp"
#include "executor.hpp"
#include "fetcher.hpp"

using Executor::Job;
using std::string;
using std::vector;

// returns ERROR_COMMAND on failure
// `COMMAND` on success
uint32_t Fetcher::headers(int sock) {
    uint32_t comm = ERROR_COMMAND;
    if (read(sock, &comm, sizeof(uint32_t)) == -1) {
        return ERROR_COMMAND;
    }

    comm = ntohl(comm);
    return comm;
}

// nullptr on error
// new Job on success
Job *Fetcher::issueJob(int sock) {
    ssize_t bytes;
    uint32_t len;
    char *buf;

    Job *j;
    vector<string> args;

    while (true) {
        if ((bytes = read(sock, &len, sizeof(uint32_t))) == -1) {
            return nullptr;
        } else if (bytes == 0) { break; }

        len = ntohl(len);
        buf = new char[len + 1](); // \0

        if (read(sock, buf, len) == -1) {
            return nullptr;
        }

        string arg { buf };
        args.push_back(arg);

        delete[] buf;
    }

    j = new Job {std::move(args), sock};
    return j;
}

// 0 on failure
// new concurrency on success
uint32_t Fetcher::setConcurrency(int sock) {
    uint32_t conc = 0;

    if (read(sock, &conc, sizeof(uint32_t)) == -1) {
        return 0;
    }

    conc = ntohl(conc);
    return conc;
}

// 0 on failure
// job Id on success
uint32_t Fetcher::stop(int sock) {
    uint32_t jobId = 0;

    if (read(sock, &jobId, sizeof(uint32_t)) == -1) {
        return 0;
    }

    jobId = ntohl(jobId);
    return jobId;
}