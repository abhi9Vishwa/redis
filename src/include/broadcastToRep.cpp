#include "broadcastToRep.hpp"
#include "helperFunc.hpp"
#include <bits/stdc++.h>
#include "infoClass.hpp"
#include "syncMasterSlave.hpp"

using namespace std;



void replicateToReplicas(std::string& rawCmd, RedisAllData& redisDb)
{
    lock_guard lock(redisDb.replicaMtx);
    for (auto it = redisDb.allReplicas.begin(); it != redisDb.allReplicas.end(); ) {
        cout << "Size of replica vec : " << redisDb.allReplicas.size() << endl;
        int fd = it->sock_fd;
        size_t sent = sendData(rawCmd, fd);
        cout << "SENT to all reps" << endl;
        sendREPLGetAck(fd);
        if (sent <= 0) {
            close(fd);
            redisDb.allReplicas.erase(it);
        }
        it++;
    }
}