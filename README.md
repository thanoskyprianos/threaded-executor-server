# K24 : HW2
Athanasios Kyprianos 1115202200082

### Commands

* To compile all files run: `make` or `make all`
* To clean obj/exe files run: `make clean`
* To start the server run:
```
./bin/jobExecutorServer [portNum] [bufferSize] [threadPoolSize]
```
* To use the commander do:
```
./bin/jobCommander [serverName] [portNum] [jobCommanderInputCommand]
```

For more info on command usage do `make help`

### Libraries used

* The project is coded in C/C++ and uses several STL utilites:
    * `std::initializer_list` used by some functions 
    * `std::map` used as a buffer as described below
    * `std::string` and `std::stringstream` for string manipulation
    * `std::vector` used for saving variable number of elements (e.g. job arguments)

* It also uses libraries such as:
    * `arpa/inet.h` for handling byte ordering
    * `sys/sockets.h`: C sockets for transfering data between commander and server
    * `pthread.h`: C threads for multithreading

### Files and directories

The project is organized as such:

* `bin/` contains the executable files after compilation
* `build/` contains object files after compilation (commander and server are seperated)
* `include/` contains headers files:
    * `codes.hpp` is a header file used by both the commander and the server that contains error codes and other useful definitions
    * `include/commander` contains header files used by the commander
    * `include/server` contains header files used by the server
* `src/` contains source code for commander and server
    * `src/commander` source code of commander
    * `src/server` source code of server
* `Makefile` is used for compiling, cleaning and help as described in the beggining
* `help` is the file that is "cat" when executing `make help`
* `syspro2024_hw2_completion_report.pdf` is the completion report
* `README.md` is this file

### Other choices
The project is mainly focused around the use of namespaces that keep each individual component organized (Fetching, Requesting, Responding, Executing etc.)

### Job Commander
The job commander consists of the following:
* `commander.cpp`: The main commander file that:
    * Resolves the hostname givven into an IP address
    * Connects to the server on ip:port
    * Calls functions from the `Requester` namespace based on the user's input
    * Waits for server response
* `requester.cpp` and `requester.hpp`: These files implement the `Requester` namespace that is responsible for taking the input the user gives and sending it through the socket the server opened.

The messages the commander sends follow this protocol:

* Firstly, each command calls the headers function that sends the specific code for each command (these codes are found in the `codes.hpp` header file)
* Then for each command the following happens:
    * `issueJob()`: For each argument the user gives, the Requester sends the pair `(len(arg), arg)`. As such, the server knows how many bytes the next word is going to be. In other words, the server gets the following message:

    ```
    10 (ISSUE_JOB)
    len(arg_1) arg_1
    len(arg_2) arg_2
    ...
    len(arg_n) arg_n
    (EOF)
    ```
    * `setConcurrency()`: For this command, all the Requester does is send the code and the new concurrency value given by the user:
    ```
    11 (SET_CONCURRENCY)
    N (uint32_t)
    (EOF)
    ```
    * `stop()`: The Requester sends the code, then extracts the number value from the job_xx string the user passes and sends it through the socket:
    ```
    12 (STOP_JOB)
    Id (uint32_t)
    (EOF)
    ```

    * `poll()` and `exit()`: For these final two functions, all the Requester has to do is send the code, as they don't require any more arguments:
    ```
    13 (POLL_JOBS)
    (EOF)
    ```
    ```
    14 (EXIT_SERVER)
    (EOF)
    ```

As a final note, `shutdown(sock, SHUT_WR)` is called to notify the server we are done writing (EOF).

### Job Executor Server

The job executor server consists of the following:
* `server.cpp`: The main server file responsible for the following:
    * Creates the passive communication socket and waits for commander connections
    * Creates a number of `worker threads` (specified by the user) that execute the jobs issued by the commanders
    * For each connected commander it creates a `commander thread` that uses functions from the `Fetcher`, `Executor` and `Respondent` namespaces accordingly.
    * A special case is when exit is called. At this point the `commander thread` kills the `main thread` using the `pthread_kill` function which forces the program out of the main loop and into a signal handler that does cleanup.
* `fetcher.cpp` and `fetcher.hpp`: The `Fetcher` namespace contains functions that are used to fetch the requests the commanders send. The `Fetcher::headers()` function extract the command code (codes.hpp) if the codes is 10 (issueJob), 11 (SET_CONCURRENCY) or 12 (STOP_JOB) the corresponding functions `Fetcher::issueJob()`, `Fetcher::setConcurrency()` and `Fetcher::stop()` are used to extract the extra data following the protocol described above.
* `executor.cpp` and `executor.hpp`: The `Executor` namespace is used to handle the data after fetching them i.e. issuing jobs in the buffer, setting the concurrency of the server or stopping a job. It also gives the `worker threads` the next available job and contains definitions of the following important variables and structures:
    * `struct Job`: The `Job` struct describes a job that is ready to be handled by a worker, it contains info such as its arguments, its jobId as well as the socket from which the job data was sent.
    * `internal`: The `internal` namespace contains the definition of some crucial variables that are used throughout the whole execution of the server. These are:
        * `running`: Boolean value that lets threads know the server should still be running.
        * `runningJobs`: The number of running jobs, needed by workers to know if they can execute the next job.
        * `incJobId`: The increasing job Id assigned to jobs on their insertion in the buffer.
        * `concurrencyLevel`: The number of jobs that can run at the same time. Changed by the setConcurrency command.
        * `bufferSize`: The theoretical size of the buffer (as the buffer is a map as described below).
        * `jobsBuffer`: The buffer that stores jobs until they are executed. Even though it is an `std::map`, it is also used as a queue as it keeps the ordering of incoming jobs (the jobId is always incresing). It is also useful for finding jobs quickly based on their jobId.
* `respondent.cpp` and `respondent.hpp`: The `Respondent` namespace is used after the execution of the commands to return resulting messages to the commanders.
* `sync.cpp` and `sync.hpp`: These files contain the `Mutex` and `Cond` namespaces that in turn contain mutexes and conditional variables used for synchronization between threads as described below.

### Threads and Synchronization
To synchronize the `worker` and `commander` threads and avoid race conditions, a couple of mutexes are used:
* `Mutex::runtime`: The `runtime` mutex guards the `jobBuffer`, as well as the `running` boolean variable. Specifically, the `commanders` lock it so they can access the jobBuffer to check if it has space when issuing jobs or when stoping jobs and the `workers` when trying to check if the jobBuffer is not empty and there are available jobs to execute. All of them also check if the server is still running. If not they stop their execution and wait for the main thread to do its thing. Lastly the `Respondent` locks it when polling queued jobs.
* `Mutex::concurrency`: The `concurrency` mutex is locked by worker when they wake up to see if they can run another job based on the now safe to access `runningJobs` and `concurrencyLevel` variables. It is also locked by the `Executor` when changing the `concurrencyLevel` variable.

When the `worker` or `commander` conditions are not met i.e. the jobBuffer is empty or not empty accordingly, they wait on the `Cond::runtimeWorker` and `Cond::runtimeCommander` conditional variables. When the criteria is met they are woken up and they continue their execution.