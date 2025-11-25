#include "handleClient.hpp"
#include "dataStructs.hpp"
#include "infoClass.hpp"
#include "processCmds.hpp"
#include "helperFunc.hpp"
#include "broadcastToRep.hpp"


#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
using namespace std;



void handle_client(int client_fd, RedisAllData& redisDb, RedisInfo& redisInfo)
{
    try {
        char buffer[4096];
        string req;
        while (true) {
            memset(buffer, 0, sizeof(buffer));
            int bytes_recv = recv(client_fd, buffer, sizeof(buffer), 0);
            if (bytes_recv <= 0)
                break;
            cout << "Bytes recved" << bytes_recv << endl;
            req.append(buffer, bytes_recv);
            if (req.find("\r\n") == string::npos)
                continue;

            try {
                vector<string> cmds = RESPArrayParser(req);

                if(cmds[0] == "REPLCONF" && cmds[1] == "ACK"){
                    cout<< "received offset from slave: "<<req<<endl;
                    updateRepsOffset(client_fd, cmds, redisDb);
                    req.clear();
                    continue;
                }

                if (cmds.empty()) {
                    string err = "-ERR empty command\r\n";
                    send(client_fd, err.c_str(), err.size(), 0);
                    req.clear();
                    continue;
                }
                processCmds(cmds, client_fd, redisDb, redisInfo);
                replicateToReplicas(client_fd, req, redisDb, redisInfo);
                // if(redisInfo.getRole() == "master"){
                //     if(isWriteCommand(cmds[0])){
                //         // send cmds to replicas in map
                //     }
                // }
                // else {
                //     if(!isWriteCommand(cmds[0])){
                //         processCmds(cmds, client_fd, redisDb, redisInfo);
                //     }
                // }
                req.clear();
            }
            catch (const exception& e) {
                string err = string("-ERR ") + e.what() + "\r\n";
                send(client_fd, err.c_str(), err.size(), 0);
            }

            req.clear();
        }
    }
    catch (const exception& e) {
        cerr << "client handler crashed: " << e.what() << endl;
    }
    close(client_fd);

}