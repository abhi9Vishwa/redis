#include "userInfo.hpp"

#include <bits/stdc++.h>
#include <openssl/sha.h>

using namespace std;

std::string sha256(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input.c_str(), input.length());
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

UserInfo::UserInfo() {
    this->name = "default";
    this->flags.insert("nopass");
}

UserInfo::UserInfo(std::string& username) {
    this->name = username;
    this->flags.insert("nopass");
}

void UserInfo::addPassword(std::string& plainPass)
{
    auto it = flags.find("nopass");
    if(it != flags.end()) {
        flags.erase(it);
    }

    passwords.insert(sha256(plainPass));
}

std::vector<std::string> UserInfo::getPasswords()
{
    vector<string> res(passwords.begin(), passwords.end());
    return res;
}

void UserInfo::setUser(std::string& username)
{
    this->name = username;
}

std::string UserInfo::getUser()
{
    return this->name;
}

std::vector<std::string> UserInfo::getFlags()
{
    vector<string> res(flags.begin(), flags.end());
    return res;
}
