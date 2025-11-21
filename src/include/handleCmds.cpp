#include "handleCmds.hpp"
#include <bits/stdc++.h>

#include "infoClass.hpp"
#include "helperFunc.hpp"

std::string handleInfo(int& client_fd)
{
    std::string role = "master";
    RedisInfo redisInfo(role);
    std::string data  = "";
    data += "Replication\n";
    data += "role:" + redisInfo.getRole();
    data += "master_replid:" + redisInfo.getReplId();
    std::string res = RESPBulkStringEncoder(data); 
    return res;
}