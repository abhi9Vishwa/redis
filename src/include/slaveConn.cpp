#include "slaveConn.hpp"
#include "helperFunc.hpp"

#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

int tcpConnToMaster(std::string& mHost, int& mPort)
{
    // Step 1: Create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Socket creation failed\n";
        return -1;
    }

    // Step 2: Resolve hostname
    struct hostent* server = gethostbyname(mHost.c_str());
    if (!server) {
        std::cerr << "Host resolution failed: " << mHost << "\n";
        close(sockfd);
        return -1;
    }

    // Step 3: Configure address
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(mPort);
    memcpy(&serv_addr.sin_addr.s_addr,
           server->h_addr,
           server->h_length);

    // Step 4: Connect
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection to " << mHost << ":" << mPort << " failed\n";
        close(sockfd);
        return -1;
    }

    // Successful connection
    return sockfd;
}

void performHandshake(int& fd, int& repPort)
{
    //Send Ping
    std::string data = "*1\r\n$4\r\nPING\r\n";
    send(fd, data.c_str(), data.size(),0);
    recvData(fd);
    // Send REPLCONF
    data = "*3\r\n$8\r\nREPLCONF\r\n$14\r\nlistening-port\r\n$4\r\n"+ std::to_string(repPort) + "\r\n";
    send(fd, data.c_str(), data.size(),0);
    recvData(fd);
    
    data = "*3\r\n$8\r\nREPLCONF\r\n$4\r\ncapa\r\n$6\r\npsync2\r\n";
    send(fd, data.c_str(), data.size(),0);
    recvData(fd);
}
