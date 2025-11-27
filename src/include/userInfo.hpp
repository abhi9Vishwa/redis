#pragma once

#include <bits/stdc++.h>

class UserInfo{
    std::string name;
    std::unordered_set<std::string> flags;
    std::unordered_set<std::string> passwords;

    public:

    UserInfo();
    
    UserInfo(std::string & username);

    void addPassword(std::string& plainPass);
    std::vector<std::string> getPasswords();

    void setUser(std::string& username);
    std::string getUser();

    std::vector<std::string> getFlags();
};