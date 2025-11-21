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
public:
    RedisInfo(std::string &role);
    ~RedisInfo();
    std::string getRole();
    std::string getReplId();
};