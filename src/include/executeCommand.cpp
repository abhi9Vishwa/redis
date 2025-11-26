#include "executeCommand.hpp"
#include "dataStructs.hpp"
#include "infoClass.hpp"
#include "helperFunc.hpp"
#include "echoSetGet.hpp"
#include "streamsfuncs.hpp"
#include "IncrDecr.hpp"
#include "handleCmds.hpp"
#include "syncMasterSlave.hpp"
#include "rdbManager.hpp"
#include "subsChannel.hpp"

#include <bits/stdc++.h>
using namespace std;

std::string executeCommand(std::vector<std::string>& cmds, int& client_fd, RedisAllData& redisDb, RedisInfo& redisInfo)
{
    cout << "exec cmd" << endl;
    for(auto i : cmds)
    {
        cout << i << " ";
    }

    string res = " done";
    string cmd = cmds[0];
    cout << "Process: " << cmd << endl;
    if(cmd == "PING") {
        res = "+PONG\r\n";

    }
    else if(cmd == "ECHO" && cmds.size() >= 2) {
        res = handleEcho(cmds[1]);

    }
    else if(cmd == "SET") {
        if(cmds.size() == 5) {
            res = handleSet(cmds[1], cmds[2], redisDb.store, redisDb.store_mtx, redisDb.expiry, cmds[3], cmds[4]);

        }
        else if(cmds.size() >= 3) {
            res = handleSet(cmds[1], cmds[2], redisDb.store, redisDb.store_mtx, redisDb.expiry);

        }
    }
    else if(cmd == "GET" && cmds.size() >= 2) {
        res = handleGet(cmds[1], redisDb.store, redisDb.store_mtx, redisDb.expiry);
    }
    else if(cmd == "TYPE" && cmds.size() >= 2) {
        res = handleType(cmds[1], redisDb.store, redisDb.listStore, redisDb.streamStore);
    }
    else if(cmd == "XADD") {
        res = handleStreamAdd(cmds, redisDb.stream_mtx, redisDb.streamStore, redisDb.streamCVs);
    }
    else if(cmd == "XRANGE") {
        res = handleXRange(cmds[1], cmds[2], cmds[3], redisDb.stream_mtx, redisDb.streamStore);
    }
    else if(cmd == "XREAD") {
        res = handleXRead(cmds, redisDb.stream_mtx, redisDb.streamStore, redisDb.streamCVs);
    }
    else if(cmd == "INCR") {
        res = handleIncr(cmds[1], redisDb.store_mtx, redisDb.store);
    }
    else if(cmd == "INFO") {
        res = "";
        if(cmds[1] == "replication") {
            res = handleInfo(client_fd, redisInfo);
        }
        else res = "-ERR Invalid command for info";
    }
    else if(cmd == "REPLCONF") {
        if(cmds[1] != "GETACK" && cmds[1] != "ACK")
            res = handleReplConf(client_fd);
        cout << "REPLCONF" << cmds[1] << endl;
    }
    else if(cmd == "PSYNC") {
        res = handlePSync(client_fd, redisDb, redisInfo);
        sendData(res, client_fd);
        vector<uint8_t> rdb = getEmptyRdb();
        int rdbLen = rdb.size();
        string lenStr = to_string(rdbLen);
        string header = RESPBulkStringEncoder(lenStr);
        sendData(header, client_fd);
        send(client_fd, rdb.data(), rdb.size(), 0);
        cout << "finished psync" << endl;
    }
    else if(cmd == "WAIT") {
        res = handleWait(client_fd, cmds, redisDb, redisInfo);
    }
    else if(cmd == "CONFIG") {
        if(cmds[1] == "GET" && cmds[2] == "dir") {
            cout << "reached" << endl;
            res = getRdbDirectory(redisInfo);
        }
        if(cmds[1] == "GET" && cmds[2] == "dbfilename") {
            res = getRdbFilename(redisInfo);
        }
    }
    else if(cmd == "SAVE") {
        res = saveRdbDump(redisDb, redisInfo);
    }
    else if(cmd == "KEYS") {
        string rdbFilePath = redisInfo.getRdbDir() + "/" + redisInfo.getRdbFilename();
        res = loadRDB(rdbFilePath, redisDb);
    }
    else if(cmd == "SUBSCRIBE") {
        subscribeToChannel(client_fd, cmds, redisDb);
    }
    else if(cmd == "PUBLISH"){
        res = publishToChannel(client_fd, cmds, redisDb);
    }
    else if(cmd == "DEBUG") {
        cout << "Master data" << endl;
        cout << redisInfo.getReplOffset() << endl;
        cout << "replica data" << endl;
        for(auto i : redisDb.allReplicas) {
            cout << i.first << " " << i.second.ackedOffset << " " << i.second.lastSeenTime << endl;
        }
    }
    else {
        string err = "-ERR unknown command\r\n";
        send(client_fd, err.c_str(), err.size(), 0);
    }
    return res;
}