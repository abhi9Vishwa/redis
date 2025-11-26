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

    std::string channel = cmds[1];
    int channelCt = addSubsChannel(fd, channel, redisDb);
    vector<string> data = { "subscribe", channel, std::to_string(channelCt) };
    std::string resp = encodeToRESPArray(data);

    while(true) {
        // send response
        ssize_t sent = send(fd, resp.c_str(), resp.size(), 0);
        if(sent <= 0) {
            removeSubsForFd(fd, channel, redisDb);
            return "client disconnected";
        }

        // receive command
        char buffer[4096];
        ssize_t received = recv(fd, buffer, sizeof(buffer) - 1, 0);
        if(received <= 0) {
            // client closed the connection -> clean up
            removeSubsForFd(fd, channel, redisDb);
            return "client disconnected";
        }

        buffer[received] = '\0';
        std::string req(buffer);

        std::vector<std::string> input = RESPArrayParser(req);
        if(input.empty()) {
            resp = "-ERR invalid command";
            continue;
        }

        std::string cmd = input[0];
        channel = input[1];
        if(cmd == "QUIT") {
            removeSubsForFd(fd, channel, redisDb);
            return "*1\r\n$5\r\nOK\r\n";  // example
        }
        else if(cmd == "SUBSCRIBE") {
            if(input.size() < 2) {
                resp = "-ERR missing channel";
            }
            else {
                channelCt = addSubsChannel(fd, input[1], redisDb);
                vector<string> data = { "subscribe", input[1], std::to_string(channelCt) };
                resp = encodeToRESPArray(data);
            }
        }
        else if(cmd == "PING") {
            vector<string> data = { "pong", "" };
            resp = encodeToRESPArray(data);
        }
        else {
            resp = "-ERR Only subscription commands allowed";
        }
    }
}

std::string publishToChannel(int& fd, std::vector<std::string>& cmds, RedisAllData& redisDb)
{
    string channel = cmds[1];
    string message = cmds[2];
    int clientSubs = redisDb.channelSubscription[channel].size();
    vector<string> data = {"message", channel, message};
    string resp = encodeToRESPArray(data);
    for(auto i : redisDb.channelSubscription[channel]) {
        ssize_t sent;
        try
        {
            sent =  send(i, resp.data(), resp.size(), 0);
            if(sent == 0){
                cout << "client socket is closed" << endl;
                removeSubsForFd(fd, channel, redisDb);
            }
            else if(sent == -1){
                cout << "client socket error" << endl;
                removeSubsForFd(fd, channel, redisDb);
            }
            else {
                cout << "successfully published" << endl;
            }
        }
        catch(const std::exception& e)
        {
            cout << "publish error" << endl;
            std::cerr << e.what() << '\n';
        }
        
    }
    return resp;
}
