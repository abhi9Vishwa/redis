#include "broadcastToRep.hpp"
#include "helperFunc.hpp"
#include <bits/stdc++.h>
#include "infoClass.hpp"
#include "syncMasterSlave.hpp"

using namespace std;



void replicateToReplicas(int& fd, std::string& rawCmd, RedisAllData& redisDb, RedisInfo& redisInfo)
{
    lock_guard lock(redisDb.replicaMtx);
    for (auto it = redisDb.allReplicas.begin(); it != redisDb.allReplicas.end(); ) {
        cout << "Size of replica vec : " << redisDb.allReplicas.size() << endl;
        int fd = it->first;
        size_t sent = sendData(rawCmd, fd);
        {
            lock_guard lock(redisInfo.redInfoMtx);
            if(it == redisDb.allReplicas.begin()) redisInfo.addReplOffset(sent);
            redisDb.repLastOffset[fd] += sent;
        }
        cout << "SENT to all reps" << endl;
        sendREPLGetAck(fd, redisInfo);
        if (sent <= 0) {
            close(fd);
            redisDb.allReplicas.erase(it);
        }
        it++;
    }
}

void updateRepsOffset(int& fd ,std::vector<std::string>& cmds, RedisAllData& redisDb){
    lock_guard lock(redisDb.replicaMtx);
    redisDb.allReplicas[fd].ackedOffset = stoll(cmds[2]);
    redisDb.allReplicas[fd].lastSeenTime = now_ms();
}