#pragma once

#include <bits/stdc++.h>
#include "infoClass.hpp"
#include "helperFunc.hpp"

std::string handleInfo(int& client_fd, RedisInfo& redisInfo);

std::string handleReplConf(int& fd);

std::string handlePSync(int& fd, RedisInfo& redisInfo);