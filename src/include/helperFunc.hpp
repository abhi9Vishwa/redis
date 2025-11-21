#pragma once

#include <arpa/inet.h>
#include <bits/stdc++.h>


// long long now_ms();
// bool checkIsStrNum(const std::string& s);
// void sendData(std::string& resp, int& client_fd);

// Bulk encode string
std::string RESPBulkStringEncoder(std::string str);

