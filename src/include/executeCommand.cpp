#include "executeCommand.hpp"
#include "dataStructs.hpp"
#include "infoClass.hpp"
#include "helperFunc.hpp"
#include "echoSetGet.hpp"
#include "streamsfuncs.hpp"
#include "IncrDecr.hpp"
#include "handleCmds.hpp"

#include <bits/stdc++.h>
using namespace std;

std::string executeCommand(std::vector<std::string>& cmds, int& client_fd,RedisAllData & redisDb, RedisInfo& redisInfo)
{
    std::unordered_map<std::string, std::vector<StreamEntry>> streamStore = redisDb.streamStore;

    std::unordered_map<std::string, std::string> store = redisDb.store;

    std::unordered_map<std::string, std::condition_variable> streamCVs = redisDb.streamCVs;
    std::unordered_map<std::string, std::mutex> streamMtxMap = redisDb.streamMtxMap;

    std::unordered_map<std::string, long long> expiry = redisDb.expiry;

    std::unordered_map<std::string, std::vector<std::string>> listStore = redisDb.listStore;

    string cmd = cmds[0];
    cout << "Process: " << cmd << endl;
    if (cmd == "PING") {
        string res = "+PONG\r\n";
        sendData(res, client_fd);

    }
    else if (cmd == "ECHO" && cmds.size() >= 2) {
        string res = handleEcho(cmds[1]);
        sendData(res, client_fd);

    }
    else if (cmd == "SET") {
        if (cmds.size() == 5) {
            string res = handleSet(cmds[1], cmds[2],store, redisDb.store_mtx, expiry, cmds[3], cmds[4]);
            sendData(res, client_fd);
        }
        else if (cmds.size() >= 3) {
            string res = handleSet(cmds[1], cmds[2], store, redisDb.store_mtx, expiry);
            sendData(res, client_fd);
        }
    }
    else if (cmd == "GET" && cmds.size() >= 2) {
        string res = handleGet(cmds[1], store, redisDb.store_mtx, expiry);
        sendData(res, client_fd);
    }
    else if (cmd == "TYPE" && cmds.size() >= 2) {
        string res = handleType(cmds[1], store, listStore, streamStore);
        sendData(res, client_fd);
    }
    else if (cmd == "XADD") {
        string res = handleStreamAdd(cmds, redisDb.stream_mtx, streamStore, streamCVs);
        sendData(res, client_fd);
    }
    else if (cmd == "XRANGE") {
        string res = handleXRange(cmds[1], cmds[2], cmds[3], redisDb.stream_mtx,streamStore);
        sendData(res, client_fd);
    }
    else if (cmd == "XREAD") {
        string res = handleXRead(cmds, redisDb.stream_mtx, streamStore, streamCVs);
        sendData(res, client_fd);
    }
    else if (cmd == "INCR") {
        string res = handleIncr(cmds[1], redisDb.store_mtx, store);
        sendData(res, client_fd);
    }
    else if (cmd == "INFO") {
        string res = "";
        if (cmds[1] == "replication") {
            res = handleInfo(client_fd, redisInfo);
        }
        else res = "_ERR Invalid command for info";
        sendData(res, client_fd);
    }
    else if (cmd == "REPLCONF") {
        string res = handleReplConf(client_fd);
        sendData(res, client_fd);
    }
    else if (cmd == "PSYNC") {
        string res = handlePSync(client_fd, redisInfo);
        sendData(res, client_fd);
        vector<uint8_t> rdb = getEmptyRdb();
        int rdbLen = rdb.size();
        string lenStr = to_string(rdbLen);
        string header = RESPBulkStringEncoder(lenStr);
        sendData(header, client_fd);
        send(client_fd, rdb.data(), rdb.size(), 0);
    }
    else {
        string err = "-ERR unknown command\r\n";
        send(client_fd, err.c_str(), err.size(), 0);
    }
}