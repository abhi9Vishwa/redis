#pragma once
#include <bits/stdc++.h>
#include "dataStructs.hpp"
#include "infoClass.hpp"

void processCmds(std::vector<std::string>& parts, int client_fd, RedisAllData& redisDb, RedisInfo& redisInfo, bool isRep = false);