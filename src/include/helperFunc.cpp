#include "helperFunc.hpp"

#include <bits/stdc++.h>  

std::string RESPBulkStringEncoder(std::string str)
{
    if (str == "")
        return "$-1\r\n";
    size_t len = str.size();
    return ("$" + std::to_string(len) + "\r\n" + str + "\r\n");
}

void recvData(int& fd)
{
    char buffer[1024];
    int bytesReceived = recv(fd, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0'; // Null-terminate the received data
        std::cout << "Received from server: " << buffer << "\n";
    }
    else if (bytesReceived == 0) {
        std::cout << "Server closed the connection.\n";
    }
}
