#include "infoClass.hpp"

#include <bits/stdc++.h>
#include <boost/uuid/uuid.hpp>             // For boost::uuids::uuid
#include <boost/uuid/uuid_generators.hpp>  // For boost::uuids::random_generator
#include <boost/uuid/uuid_io.hpp>  

using namespace std;

RedisInfo::RedisInfo() {
    this->role = "not initialized yet";
    boost::uuids::random_generator gen;
    boost::uuids::uuid id = gen();
    std::string s = boost::uuids::to_string(id);

    this->masterReplid = s;
    this->masterReplOffset = 0;
}

RedisInfo::RedisInfo(std::string& role)
{
    this->role = role;
    boost::uuids::random_generator gen;
    boost::uuids::uuid id = gen();
    std::string s = boost::uuids::to_string(id);

    this->masterReplid = s;
    this->masterReplOffset = 0;
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
    return this->masterReplid;
}

long long RedisInfo::getReplOffset()
{
    return this->masterReplOffset;
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
    this->masterHost = host;
}

void RedisInfo::setMasterPort(int& port)
{
    this->masterPort = port;
}

void RedisInfo::addReplOffset(int n)
{
    cout<< "offset" <<n<<endl;
    this->masterReplOffset += n;
    cout<<"master offset" << this->masterReplOffset <<endl;
}

void RedisInfo::setRole(std::string& role) {
    this->role = role;
}

void RedisInfo::setRdbDir(std::string& dir)
{
    this->rdbDir = dir;
}

void RedisInfo::setRdbFilename(std::string& filename)
{
    this->rdbFilename = filename;
}

std::string RedisInfo::getRdbDir()
{
    return this->rdbDir;
}

std::string RedisInfo::getRdbFilename()
{
    return this->rdbFilename;
}
