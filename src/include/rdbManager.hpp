#pragma once

#include <bits/stdc++.h>

#include "dataStructs.hpp"
#include "infoClass.hpp"

std::string getRdbDirectory(RedisInfo& redisInfo);

std::string getRdbFilename(RedisInfo& redisInfo);

std::string saveRdbDump(RedisAllData& redisDb, RedisInfo& redisInfo);

std::string loadRDB(std::string& file, RedisAllData& redisDb);