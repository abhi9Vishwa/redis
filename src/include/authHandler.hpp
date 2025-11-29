#pragma once

#include <bits/stdc++.h>
#include "dataStructs.hpp"
#include "userInfo.hpp"

std::string getWhoAmI(UserInfo& userInfo);

std::string getUserFlags(int& fd, std::vector<std::string>& cmds, RedisAllData& redisDb, UserInfo& userInfo);

std::string updateUserPass(int& fd, std::vector<std::string>& cmds, RedisAllData& redisDb, UserInfo& userInfo);

std::string authenticateUser(std::string& username, std::string& password, int& fd, RedisAllData& redisDb);

bool isUserAllowed(int&fd, std::string& currUser, RedisAllData& redisDb);