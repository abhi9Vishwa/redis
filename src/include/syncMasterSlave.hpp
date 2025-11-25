#pragma once

#include <bits/stdc++.h>
#include "infoClass.hpp"
#include "dataStructs.hpp"

void sendREPLGetAck(int &fd, RedisInfo& redisInfo);

void processCmdsFromMaster(std::vector<std::string>& cmds, int client_fd, RedisAllData &redisDb, RedisInfo &redisInfo, bool isRep);

std::string handleWait(int& fd, std::vector<std::string>& cmds, RedisAllData &redisDb, RedisInfo& redisInfo);

