#include "subsChannel.hpp"

#include <bits/stdc++.h>

#include "helperFunc.hpp"

using namespace std;

void removeSubsForFd(int fd, string& channel, RedisAllData& redisDb) {
    lock_guard lock(redisDb.subsMtx);
    auto it = redisDb.clientSubscriptions.find(fd);
    if(it != redisDb.clientSubscriptions.end()) {
        redisDb.clientSubscriptions.erase(it);
        redisDb.channelSubscription[channel].erase(fd);
    }
}

int addSubsChannel(int& fd, std::string& channel, RedisAllData& redisDb) {
    {
        lock_guard lock(redisDb.subsMtx);
        redisDb.clientSubscriptions[fd].insert(channel);
        redisDb.channelSubscription[channel].insert(fd);
    }
    return redisDb.clientSubscriptions[fd].size();
}

std::string subscribeToChannel(int& fd, std::vector<std::string>& cmds, RedisAllData& redisDb) {
    if(cmds.size() < 2) {
        return "-ERR wrong number of arguments for 'subscribe'";
    }
    string channel = cmds[1];
    int channelCt = addSubsChannel(fd, channel, redisDb);
    vector<string> data = {cmds[0], channel, to_string(channelCt)};
    string resp = encodeToRESPArray(data);

    while (true)
    {
        ssize_t sent = send(fd, resp.c_str(), resp.size(), 0);
        if(sent < 0) {
            removeSubsForFd(fd, channel, redisDb);
            return "sent failed";
        }
        char buffer[4096];
        ssize_t received = recv(fd, buffer, sizeof(buffer), 0);
        if(received <= 1) return "recv ended";
        string req(buffer);
        vector<string> input = RESPArrayParser(req);
        if(input[0] == "SUBSCRIBE"){
            channelCt = addSubsChannel(fd, input[1], redisDb);
            data = {input[0], input[1], to_string(channelCt)};
            resp = encodeToRESPArray(data);
        }
        else if(input[0] == "PING"){
            data = {"PONG", ""};
            resp = encodeToRESPArray(data);
        }
        else {
            resp = "-ERR Only subscription commands allowed";
        }
    }
    cout << "out of loop" << endl;

    return resp;
}

std::string publishToChannel(int& fd, std::vector<std::string>& cmds, RedisAllData& redisDb)
{
    string channel = cmds[1];
    int clientSubs = redisDb.channelSubscription[channel].size();
    string resp = ":" + to_string(clientSubs) + "\r\n";
    return resp;
}
