#include "rdbManager.hpp"
#include "helperFunc.hpp"
#include <bits/stdc++.h>

std::string getRdbDirectory(RedisInfo& redisInfo)
{
    std::string dir = redisInfo.getRdbDir();
    return RESPBulkStringEncoder(dir);
}

std::string getRdbFilename(RedisInfo& redisInfo)
{
    std::string file = redisInfo.getRdbFilename();
    return RESPBulkStringEncoder(file);
}
