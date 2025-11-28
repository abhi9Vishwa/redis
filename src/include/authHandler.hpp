#pragma once

#include <bits/stdc++.h>
#include "dataStructs.hpp"
#include "userInfo.hpp"

std::string getWhoAmI(UserInfo& userInfo);

std::string getUserFlags(int& fd, std::vector<std::string>& cmds, RedisAllData& redisDb, UserInfo& userInfo);

std::string updateUserPass(std::vector<std::string>& cmds, UserInfo& userInfo);

std::string authenticateUser(std::string password, RedisAllData& redisDb, UserInfo& userInfo);