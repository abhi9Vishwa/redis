#pragma once
#include <bits/stdc++.h>
#include "dataStructs.hpp"
#include "infoClass.hpp"

std::string handleMulti(int& client_fd, std::unordered_map<int, std::queue<std::vector<std::string>>>& multiQueue, std::unordered_map<int, bool>& inMulti, RedisAllData& redisDb, RedisInfo& redisInfo);

std::string handleExec(int& client_fd, std::unordered_map<int, std::queue<std::vector<std::string>>>& multiQueue, std::unordered_map<int, bool>& inMulti, RedisAllData& redisDb, RedisInfo& redisInfo);