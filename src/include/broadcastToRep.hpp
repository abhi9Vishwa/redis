#pragma once

#include <bits/stdc++.h>
#include "dataStructs.hpp"
#include "infoClass.hpp"


void replicateToReplicas(int& fd, std::string& rawCmd, RedisAllData& redisDb, RedisInfo& redisInfo);

void updateRepsOffset(int& fd ,std::vector<std::string>& cmds, RedisAllData& redisDb);
