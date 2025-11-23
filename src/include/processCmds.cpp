#include "processCmds.hpp"
#include "helperFunc.hpp"
#include "multiExec.hpp"
#include "executeCommand.hpp"
#include <bits/stdc++.h>

using namespace std;


void processCmds(std::vector<std::string>& parts, int client_fd, RedisAllData& redisDb, RedisInfo& redisInfo, bool isRep)
 {
    std::unordered_map<int, std::queue<std::vector<std::string>>> multiQueue = redisDb.multiQueue;
    std::unordered_map<int, bool> inMulti = redisDb.inMulti;

    string cmd = parts[0];

    // MULTI
    if (cmd == "MULTI") {
        string resp = handleMulti(client_fd, multiQueue, inMulti, redisDb, redisInfo);
        if(!isRep) sendData(resp, client_fd);
        return;
    }

    // EXEC
    if (cmd == "EXEC") {
        string resp = handleExec(client_fd, multiQueue, inMulti, redisDb, redisInfo);
        if(!isRep) sendData(resp, client_fd);
        return;
    }

    // DISCARD
    if (cmd == "DISCARD") {
        inMulti.erase(client_fd);
        multiQueue.erase(client_fd);
        if(!isRep) sendData("+OK\r\n", client_fd);
        return;
    }

    // If in MULTI -> queue commands
    if (inMulti[client_fd]) {
        multiQueue[client_fd].push(parts);
        if(!isRep) sendData("+QUEUED\r\n", client_fd);
        return;
    }

    // Normal command
    string resp = executeCommand(parts, client_fd, redisDb, redisInfo);
    if(!isRep) sendData(resp, client_fd);
}
