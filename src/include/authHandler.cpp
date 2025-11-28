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

std::string updateUserPass(std::vector<std::string>& cmds, RedisAllData& redisDb, UserInfo& userInfo)
{
    string pass = cmds[3].substr(1);
    
    cout << pass << endl;
    userInfo.addPassword(pass);
    return "+OK\r\n";
}

std::string authenticateUser(std::string password, UserInfo& userInfo)
{
    if(userInfo.checkPassword(password)) {
        return "+OK\r\n";
    }
    else {
        return "-WRONGPASS invalid username-password pair or user is disabled.";
    }
}

