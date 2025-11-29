#include "authHandler.hpp"
#include "helperFunc.hpp"
#include "userInfo.hpp"


using namespace std;

std::string getWhoAmI(UserInfo& userInfo)
{
    string resp = RESPBulkStringEncoder(userInfo.getUser());
    return resp;
}

std::string getUserFlags(int& fd, std::vector<std::string>& cmds, RedisAllData& redisDb, UserInfo& userInfo)
{
    string resp;
    if(cmds.size() < 3) {
        resp = "-ERR user name is not provided";
    }

    vector<string> data = { "flags", encodeToRESPArray(userInfo.getFlags()), "passwords", encodeToRESPArray(userInfo.getPasswords()) };
    resp = encodeToRESPArray(data);
    return resp;
}

std::string updateUserPass(int& fd,std::vector<std::string>& cmds, RedisAllData& redisDb, UserInfo& userInfo)
{
    string pass = cmds[3].substr(1);
    
    cout << pass << endl;
    userInfo.addPassword(pass);
    userInfo.allowUserOnAuth(fd);
    return "+OK\r\n";
}

std::string authenticateUser(std::string& username, std::string& password, int& fd, RedisAllData& redisDb)
{
    UserInfo* userInfo = &redisDb.userData[username];
    if(userInfo->checkPassword(password)) {
        userInfo->allowUserOnAuth(fd);
        redisDb.currUser = username;
        return "+OK\r\n";
    }
    else {
        return "-WRONGPASS invalid username-password pair or user is disabled.";
    }
}

bool isUserAllowed(int&fd, std::string& currUser, RedisAllData& redisDb){
    UserInfo* userInfo = &redisDb.userData[currUser];
    return userInfo->isNoPassSet() || userInfo->isUserAllowed(fd); 
}
