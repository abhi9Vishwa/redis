#include "handleCmds.hpp"
#include <bits/stdc++.h>

#include "infoClass.hpp"
#include "helperFunc.hpp"

void addReplicaCreds(int& fd, std::vector<ReplicaInfo>& allReps, std::mutex& repMtx) {
    ReplicaInfo r = { fd };
    {
        std::lock_guard lock(repMtx);
        allReps.push_back(r);
    }
}

std::string handleInfo(int& client_fd, RedisInfo& redisInfo)
{
    std::string data = "";
    data += "Replication\n";
    data += "role:" + redisInfo.getRole() + "\n";
    data += "masterReplid:" + redisInfo.getReplId() + "\n";
    data += "masterReplOffset:" + std::to_string(redisInfo.getReplOffset()) + "\n";
    std::string res = RESPBulkStringEncoder(data);
    return res;
}

std::string handleReplConf(int& fd)
{
    std::string resp = "*3\r\n$8\r\nREPLCONF\r\n$3\r\nACK\r\n$1\r\n0\r\n";
    return resp;
}

std::string handlePSync(int& fd, RedisAllData& redisdb, RedisInfo& redisInfo)
{
    std::string resp = "+FULLRESYNC " + redisInfo.getReplId() + " " + std::to_string(redisInfo.getReplOffset()) + "\r\n";
    addReplicaCreds(fd, redisdb.allReplicas, redisdb.replicaMtx);
    return resp;
}



