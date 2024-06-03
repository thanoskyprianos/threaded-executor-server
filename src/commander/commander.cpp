#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "codes.hpp"
#include "requester.hpp"

using std::cout;
using std::cerr;
using std::endl;

// exit closing socket
void exit_s(int sock, int errorCode) {
    close(sock);
    exit(errorCode);
}

// returns socket connected to server
int connectToServer(char *hostname, uint16_t port) {
    int sock;
    hostent *serverEntry;
    sockaddr_in serverSocketAddr;

    // resovling hostname
    if ((serverEntry = gethostbyname(hostname)) == nullptr) {
        herror("Error during hostname resovling");
        exit(HOSTNAME_ERROR);
    }

    // getting TCP socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error while creating socket");
        exit(NETWORK_ERROR);
    }

    serverSocketAddr.sin_family = AF_INET;
    memcpy(&serverSocketAddr.sin_addr, serverEntry->h_addr, serverEntry->h_length);
    serverSocketAddr.sin_port = htons(port);

    // connecting to server
    if(connect(sock, (sockaddr *)&serverSocketAddr, sizeof(serverSocketAddr)) == -1) {
        perror("Error while connect to server");
        exit_s(sock, NETWORK_ERROR);
    }

    return sock;
}

void readFromServer(int sock) {
    uint32_t len;
    char *buf;

    while (true) {
        ssize_t bytes = 0;
        
        if ((bytes = read(sock, &len, sizeof(uint32_t))) < 0) {
            cerr << "Error while reading server response" << endl;
            exit_s(sock, PROCCESS_ERROR);
        }
        else if (bytes == 0) { break; } // server sent shutdown

        len = ntohl(len); // convert to host order
        buf = new char[len + 1](); // \0

        if (read(sock, buf, len) == -1) {
            cerr << "Error while reading server response" << endl;
            exit_s(sock, PROCCESS_ERROR);
        }

        cout << buf;
        delete[] buf;
    }
}

int main(int argc, char *argv[]) {
    uint16_t port = 0;
    int sock = -1;
    char *address = nullptr;

    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " [serverName] [portNum] [jobCommanderInputCommand]" << endl;
        exit(USAGE_ERROR);
    }

    address = argv[1];

    if ((port = atoi(argv[2])) <= 0) {
        cerr << "Port should be a positive integer. Port: " << argv[2] << endl;
        exit(VALUE_ERROR);
    }

    if (!strcmp(argv[3], "issueJob")) {
        if (argc < 5) {
            cerr << "Job arguments missing" << endl;
            exit(USAGE_ERROR);
        }

        sock = connectToServer(address, port);

        if(!Requester::issueJob(sock, argc - 4, argv + 4)) {
            cerr << "Error while issuing job" << endl;
            exit_s(sock, PROCCESS_ERROR);
        }
    }
    else if (!strcmp(argv[3], "setConcurrency")) {
        if (argc < 5) {
            cerr << "Concurrency value is missing" << endl;
            exit(USAGE_ERROR);
        }

        int concurrency = atoi(argv[4]);
        if (concurrency <= 0) {
            cerr << "Consurrency should be a positive integer. Conncurrency: " << argv[5] << endl;
            exit(VALUE_ERROR);
        }

        sock = connectToServer(address, port);

        if (!Requester::setConcurrency(sock, concurrency)) {
            cerr << "Error while sending new concurrency" << endl;
            exit_s(sock, PROCCESS_ERROR);
        } 
    }
    else if (!strcmp(argv[3], "stop")) {
        if (argc < 5) {
            cerr << "Job ID is missing" << endl;
            exit(USAGE_ERROR);
        }

        sock = connectToServer(address, port);

        if(!Requester::stop(sock, argv[4])) {
            cerr << "Error while sending job ID" << endl;
            exit_s(sock, PROCCESS_ERROR);
        }
    }
    else if (!strcmp(argv[3], "poll")) {
        sock = connectToServer(address, port);

        if(!Requester::poll(sock)) {
            cerr << "Error while sending poll command" << endl;
            exit_s(sock, PROCCESS_ERROR);
        }
    }
    else if (!strcmp(argv[3], "exit")) {
        sock = connectToServer(address, port);

        if(!Requester::exit(sock)) {
            cerr << "Error while sending exit command" << endl;
            exit_s(sock, PROCCESS_ERROR);
        }
    }
    else {
        cerr << "Invalid command: " << argv[3] << endl;
        exit(USAGE_ERROR);
    }

    if (shutdown(sock, SHUT_WR) == -1) {
        perror("Error while shuting down write end of socket");
        exit_s(sock, SOCKET_ERROR);
    }

    readFromServer(sock);

    close(sock);
    return 0;
}