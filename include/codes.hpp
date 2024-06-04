// ERROR CODES

#pragma once

#define USAGE_ERROR     1   // usage error
#define HOSTNAME_ERROR  2   // error when resolving hostname
#define VALUE_ERROR     3   // error when converting values
#define NETWORK_ERROR   4   // network related errors
#define PROCCESS_ERROR  5   // processing errors (read/write)
#define THREAD_ERROR    6   // thread related errors
#define SOCKET_ERROR    7   // socket related errors
#define EXEC_ERROR      8   // fork/exec error

#define ISSUE_JOB       10
#define SET_CONCURRENCY 11
#define STOP_JOB        12
#define POLL_JOBS       13
#define EXIT_SERVER     14
#define ERROR_COMMAND   15