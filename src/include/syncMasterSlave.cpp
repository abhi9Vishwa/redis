#include "syncMasterSlave.hpp"
#include <bits/stdc++.h>
#include "helperFunc.hpp"
#include "infoClass.hpp"
#include "processCmds.hpp"
#include <chrono>

using namespace std;

void sendREPLGetAck(int& fd, RedisInfo& redisInfo)
{
    string cmd = "*3\r\n$8\r\nREPLCONF\r\n$6\r\nGETACK\r\n$1\r\n*\r\n";
    int sent = sendData(cmd, fd);
    {
        lock_guard lock(redisInfo.redInfoMtx);
        redisInfo.addReplOffset(sent);
    }
}


std::string handleReplGetAck(RedisInfo& redisInfo) {
    std::vector<std::string> res;
    res.push_back("REPLCONF");
    res.push_back("ACK");
    res.push_back(std::to_string(redisInfo.getReplOffset()));
    std::string encodeStr = encodeToRESPArray(res);
    std::cout << redisInfo.getRole() << encodeStr << std::endl;
    return encodeStr;
}

void processCmdsFromMaster(std::vector<std::string>& cmds, int fd, RedisAllData& redisDb, RedisInfo& redisInfo, bool isRep)
{
    string resp = "";
    cout << "cmds data passed" << endl;
    for (auto i : cmds) cout << i << " ";
    if (cmds[0] == "REPLCONF" && cmds[1] == "GETACK") {
        resp = handleReplGetAck(redisInfo);
        sendData(resp, fd);
    }
    else {
        processCmds(cmds, fd, redisDb, redisInfo, true);
    }
}

std::string handleWait(int& fd,std::vector<std::string>& cmds, RedisAllData& redisDb, RedisInfo& redisInfo)
{
    string resp;
    int ct = 0;
    long long deadline = now_ms() + stoll(cmds[2]);
    string str = "*3\r\n$8\r\nREPLCONF\r\n$6\r\nGETACK\r\n$1\r\n*\r\n";
    lock_guard lock(redisDb.replicaMtx);
    for (auto i : redisDb.allReplicas) {
        int fd = i.first;
        sendData(str, fd);
    }
    while (true)
    {


        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        for (auto i : redisDb.allReplicas) {
            if (i.second.ackedOffset >= redisDb.repLastOffset[fd]) {
                ct++;
            }
        }


        if (ct >= stoi(cmds[1])) {
            resp = ":" + to_string(ct) + "\r\n";
            break;
        }
        if (now_ms() > deadline) {
            resp = ":" + to_string(ct) + "\r\n";
            break;
        }
    }

    // resp = ":" + to_string(redisDb.allReplicas.size()) + "\r\n";
    return resp;
}


