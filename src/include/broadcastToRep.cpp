#include "broadcastToRep.hpp"
#include "helperFunc.hpp"
#include <bits/stdc++.h>

using namespace std;


void replicateToReplicas(std::string& rawCmd, RedisAllData& redisDb)
{
    lock_guard lock(redisDb.replicaMtx);
    for (auto it = redisDb.allReplicas.begin(); it != redisDb.allReplicas.end(); ) {
        cout << "Size of replica vec : " << redisDb.allReplicas.size() << endl;
        int fd = it->sock_fd;
        cout << "sent to client : " << fd << endl;
        size_t sent = sendData(rawCmd, fd);
        if (sent <= 0) {
            close(fd);
            redisDb.allReplicas.erase(it);
        }
        it++;
    }
}