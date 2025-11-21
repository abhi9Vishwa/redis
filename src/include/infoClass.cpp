#include "infoClass.hpp"

#include <bits/stdc++.h>
#include <boost/uuid/uuid.hpp>             // For boost::uuids::uuid
#include <boost/uuid/uuid_generators.hpp>  // For boost::uuids::random_generator
#include <boost/uuid/uuid_io.hpp>  

using namespace std;

RedisInfo::RedisInfo(std::string & role)
{
    this->role = role;
    boost::uuids::random_generator gen;
    boost::uuids::uuid id = gen();
    std::string s = boost::uuids::to_string(id);

    this->master_replid = s;
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
