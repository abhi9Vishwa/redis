#include "multiExec.hpp"
#include "helperFunc.hpp"
#include "executeCommand.hpp"
#include "dataStructs.hpp"
#include "infoClass.hpp"
#include <bits/stdc++.h>
using namespace std;



string handleMulti(int& client_fd, std::unordered_map<int, std::queue<std::vector<std::string>>>& multiQueue, std::unordered_map<int, bool>& inMulti, RedisAllData& redisDb, RedisInfo& redisInfo) {
    inMulti[client_fd] = true;

    if (!multiQueue.count(client_fd)) {
        multiQueue[client_fd] = std::queue<std::vector<std::string>>();
    }

    return "+OK\r\n";
}

string handleExec(int& client_fd, std::unordered_map<int, std::queue<std::vector<std::string>>>& multiQueue, std::unordered_map<int, bool>& inMulti, RedisAllData& redisDb, RedisInfo& redisInfo) {

    if (!inMulti[client_fd]) {
        return "-ERR EXEC without MULTI\r\n";
    }

    auto& q = multiQueue[client_fd];

    // Prepare RESP array header
    string resp = "*" + std::to_string(q.size()) + "\r\n";

    while (!q.empty()) {
        vector<string> parts = q.front();
        q.pop();

        // Execute command normally
        string out = executeCommand(parts, client_fd, redisDb, redisInfo);
        resp += out;
    }

    // Reset state
    inMulti.erase(client_fd);
    multiQueue.erase(client_fd);

    return resp;
}
