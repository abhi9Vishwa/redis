#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "infoClass.hpp"
#include "helperFunc.hpp"
#include "handleClient.hpp"
#include "slaveConn.hpp"
#include "dataStructs.hpp"


using namespace std;

using ll = long long;

int server_fd = socket(AF_INET, SOCK_STREAM, 0);

void handle_sigint(int) {
    close(server_fd);
    exit(0);
}


int main(int argc, char** argv) {
    string role = "master";
    RedisInfo redisInfo(role);
    RedisAllData redisDB;
    signal(SIGINT, handle_sigint);
    int port = 6379;
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    if (server_fd < 0) {
        std::cerr << "Failed to create server socket\n";
        return 1;
    }

    // get Port number 
    if (argc >= 3) {
        string portFlag = argv[1];
        if (portFlag == "-p" || portFlag == "--port") {
            try {
                port = stoi(argv[2]);
            }
            catch (const std::exception& e) {
                cerr << "Invalid port provided" << endl;
                std::cerr << e.what() << '\n';
            }
        }
    }
    bool isReplicaFlag = false;
    string masterHost;
    int masterPort;
    if (argc == 5) {
        string isReplica = argv[3];
        string masterNode = argv[4];
        if (isReplica == "--replicaof") {
            isReplicaFlag = true;
            role = "slave";
            redisInfo.setRole(role);

            stringstream ss(masterNode);
            ss >> masterHost >> masterPort;
            redisInfo.setMasterHost(masterHost);
            redisInfo.setMasterPort(masterPort);
        }

    }

    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) <
        0) {
        std::cerr << "setsockopt failed\n";
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) !=
        0) {
        std::cerr << "Failed to bind to port " << port << "\n";
        return 1;
    }

    // If server is a replica then connect to master server
    if (isReplicaFlag) {
        thread([&]() {
            int fd = -1;
            while (true) {
                fd = tcpConnToMaster(masterHost, masterPort);
                if (fd >= 0) {
                    cout << "Connected to master" << masterHost << ":" << masterPort << endl;
                    performHandshake(fd, port);
                    break;
                }
                cerr << "Retrying master connection in 1 second...\n";
                this_thread::sleep_for(chrono::seconds(1));
            }
            // Handle handshake
            close(fd);
            }).detach();
    }

    int connection_backlog = 5;
    if (listen(server_fd, connection_backlog) != 0) {
        std::cerr << "listen failed\n";
        return 1;
    }

    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    std::cout << "Waiting for a client to connect on port: " << port << "\n";

    while (true) {
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, (socklen_t*)&client_addr_len);
        std::cout << "Client connected\n";
        thread(handle_client, client_fd, redisDB, redisInfo).detach();
    }

    close(server_fd);

    return 0;
}
