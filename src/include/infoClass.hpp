// infoClass.hpp
#pragma once

#include <string>
#include <boost/uuid/uuid.hpp>             // For boost::uuids::uuid
#include <boost/uuid/uuid_generators.hpp>  // For boost::uuids::random_generator
#include <boost/uuid/uuid_io.hpp> 

class RedisInfo{
private:
    std::string role;
    std::string master_replid;
    int master_repl_offset;
public:
    RedisInfo(std::string &role);
    RedisInfo();
    ~RedisInfo();
    std::string getRole();
    std::string getReplId();
    int getReplOffset();
    void setRole(std::string& role);
};