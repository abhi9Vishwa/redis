// infoClass.hpp
#pragma once

#include <string>
#include <boost/uuid/uuid.hpp>             // For boost::uuids::uuid
#include <boost/uuid/uuid_generators.hpp>  // For boost::uuids::random_generator
#include <boost/uuid/uuid_io.hpp> 
#include <mutex>

class RedisInfo {
private:
    std::string role;
    std::string masterReplid;
    long long masterReplOffset;
    std::string masterHost;
    int masterPort;
public:
    std::mutex redInfoMtx;
    RedisInfo(std::string& role);
    RedisInfo();
    ~RedisInfo();
    std::string getRole();
    std::string getReplId();
    long long getReplOffset();
    std::string getMasterHost();
    int getMasterPort();
    void setMasterHost(std::string& host);
    void setMasterPort(int& port);
    void addReplOffset(int n);
    void setRole(std::string& role);
};