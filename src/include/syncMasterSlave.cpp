#include "syncMasterSlave.hpp"
#include <bits/stdc++.h>
#include "helperFunc.hpp"
#include "infoClass.hpp"
#include "processCmds.hpp"

using namespace std;

void sendREPLGetAck(int& fd)
{   
    string cmd = "*3\r\n$8\r\nREPLCONF\r\n$6\r\nGETACK\r\n$1\r\n*\r\n";
    sendData(cmd, fd);
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
    cout<<"cmds data passed" <<endl;
    for(auto i : cmds) cout<< i<<" ";
    if(cmds[0] == "REPLCONF" && cmds[1] == "GETACK"){
        resp = handleReplGetAck(redisInfo);
        sendData(resp, fd);
    }
    else{
        processCmds(cmds, fd, redisDb, redisInfo, true);
    }
}
