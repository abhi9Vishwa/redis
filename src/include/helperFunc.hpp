#pragma once

#include <arpa/inet.h>
#include <bits/stdc++.h>
#include "dataStructs.hpp"


long long now_ms();

bool checkIsStrNum(const std::string& s);
// void sendData(std::string& resp, int& client_fd);

// Bulk encode string
std::string RESPBulkStringEncoder(std::string str);

std::string RESPEncodeStream(std::vector<StreamEntry>& vec);

std::string recvData(int& fd);

std::vector<std::string> RESPArrayParser(std::string& rawInput);

std::vector<uint8_t> getEmptyRdb();

std::pair<long long, long long> parseStreamId(const std::string& id, int end = 0);

int binarySearch(const std::string& target, const std::vector<StreamEntry>& vec);

void sendData(std::string resp, int& client_fd);


