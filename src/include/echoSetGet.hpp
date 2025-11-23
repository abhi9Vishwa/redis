#pragma once

#include <string>
#include <bits/stdc++.h>
#include "dataStructs.hpp"
inline std::string handleEcho(std::string& str) {return str;}

std::string handleSet(std::string& key, std::string& val, std::unordered_map<std::string, std::string>& store, std::mutex& store_mtx, std::unordered_map<std::string, long long> &expiry, std::string opt = "", std::string expiryStr = "-1");

std::string handleGet(std::string& key, std::unordered_map<std::string, std::string>& store, std::mutex& store_mtx, std::unordered_map<std::string, long long> &expiry);

std::string handleType(std::string& key, std::unordered_map<std::string, std::string>& store, std::unordered_map<std::string, std::vector<std::string>>& listStore, std::unordered_map<std::string, std::vector<StreamEntry>>& streamStore);