#pragma once
#include <bits/stdc++.h>
#include "dataStructs.hpp"




std::string handleIncr(std::string& key, std::mutex& store_mtx,std::unordered_map<std::string, std::string>& store);