#pragma once

#include <bits/stdc++.h>

class UserInfo{
    std::string name;
    std::unordered_set<std::string> flags;
    std::unordered_set<std::string> passwords;
    std::unordered_set<int> allowedUsers;

    public:

    UserInfo();
    
    UserInfo(std::string & username);

    void addPassword(std::string& plainPass);
    std::vector<std::string> getPasswords();

    bool checkPassword(std::string& inputPass);

    void setUser(std::string& username);
    std::string getUser();

    void allowUserOnAuth(int& fd);
    bool isUserAllowed(int& fd);

    std::vector<std::string> getFlags();
};