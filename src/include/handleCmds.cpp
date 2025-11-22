#include "handleCmds.hpp"
#include <bits/stdc++.h>

#include "infoClass.hpp"
#include "helperFunc.hpp"

std::string handleInfo(int& client_fd, RedisInfo& redisInfo)
{
    std::string data = "";
    data += "Replication\n";
    data += "role:" + redisInfo.getRole() + "\n";
    data += "master_replid:" + redisInfo.getReplId() + "\n";
    std::string res = RESPBulkStringEncoder(data);
    return res;
}