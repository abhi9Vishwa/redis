#include "subsChannel.hpp"

#include <bits/stdc++.h>

#include "helperFunc.hpp"

using namespace std;
std::string subscribeToChannel(int& fd, std::vector<std::string>& cmds, RedisAllData& redisDb) {
    string channel = cmds[1];
    {
        lock_guard lock(redisDb.subsMtx);
        redisDb.subscriptions[fd].insert(channel);
    }
    vector<string> data = {"subscribe", "channel", to_string(redisDb.subscriptions[fd].size())};
    return encodeToRESPArray(data);
}
