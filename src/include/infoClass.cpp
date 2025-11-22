#include "infoClass.hpp"

#include <bits/stdc++.h>
#include <boost/uuid/uuid.hpp>             // For boost::uuids::uuid
#include <boost/uuid/uuid_generators.hpp>  // For boost::uuids::random_generator
#include <boost/uuid/uuid_io.hpp>  

using namespace std;

RedisInfo::RedisInfo(){
    this->role = "not initialized yet";
    boost::uuids::random_generator gen;
    boost::uuids::uuid id = gen();
    std::string s = boost::uuids::to_string(id);

    this->master_replid = s;
    this->master_repl_offset = 0;
}

RedisInfo::RedisInfo(std::string & role)
{
    this->role = role;
    boost::uuids::random_generator gen;
    boost::uuids::uuid id = gen();
    std::string s = boost::uuids::to_string(id);

    this->master_replid = s;
    this->master_repl_offset = 0;
}

RedisInfo::~RedisInfo()
{
}

std::string RedisInfo::getRole()
{
    return this->role;
}

std::string RedisInfo::getReplId()
{
    return this->master_replid;
}

int RedisInfo::getReplOffset()
{
    return this->master_repl_offset;
}

std::string RedisInfo::getMasterHost()
{
    return this->getMasterHost();
}

int RedisInfo::getMasterPort()
{
    return this->getMasterPort();
}

void RedisInfo::setMasterHost(std::string& host)
{
    this->masterHost =host;
}

void RedisInfo::setMasterPort(int& port)
{
    this->masterPort = port;
}

void RedisInfo::setRole(std::string& role){
    this->role = role;
}