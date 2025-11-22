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
    data += "master_repl_offset:" + std::to_string(redisInfo.getReplOffset()) + "\n";
    std::string res = RESPBulkStringEncoder(data);
    return res;
}

std::string handleReplConf(int& fd)
{
    std::string resp = "+OK\r\n";
    return resp;
}

std::string handlePSync(int& fd, RedisInfo& redisInfo)
{

    std::string resp = "+FULLRESYNC " + redisInfo.getReplId() + " "+ std::to_string(redisInfo.getReplOffset()) +"\r\n";
    return resp;
}


