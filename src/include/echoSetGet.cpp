#include "echoSetGet.hpp"
#include <bits/stdc++.h>
#include "helperFunc.hpp"

using namespace std;

std::string handleSet(std::string& key, std::string& val, unordered_map<string, string>& store, mutex& store_mtx, unordered_map<string, long long> &expiry, std::string opt, std::string expiryStr)
{
    int expiryVal = stoi(expiryStr);
    long long expire_at = -1;

    if (opt == "EX") {
        expire_at = now_ms() + expiryVal * 1000LL;
    }
    else if (opt == "PX") {
        expire_at = now_ms() + expiryVal;
    }

    {
        lock_guard<mutex> lock(store_mtx);
        store[key] = val;
        if (expire_at != -1)
            expiry[key] = expire_at;
        else
            expiry.erase(key);
    }
    string resp = "+OK\r\n";
    return resp;
}

std::string handleSet(std::string& key, std::string& val, std::unordered_map<std::string, std::string>& store, std::mutex& store_mtx, std::unordered_map<std::string, long long>& expiry)
{
    return std::string();
}

std::string handleGet(std::string& key, std::unordered_map<std::string, std::string>& store, std::mutex& store_mtx, std::unordered_map<std::string, long long>& expiry)
{
    string resp = "";
    lock_guard<mutex> lock(store_mtx);

    if (store.find(key) == store.end()) {
        resp = RESPBulkStringEncoder("");
    }
    else {
        if (expiry.find(key) != expiry.end() && now_ms() >= expiry[key]) {
            store.erase(key);
            expiry.erase(key);
            resp = RESPBulkStringEncoder("");
        }
        else {
            resp = RESPBulkStringEncoder(store[key]);
        }
    }
    return resp;
}

std::string handleType(std::string& key, std::unordered_map<std::string, std::string>& store, std::unordered_map<std::string, vector<std::string>>& listStore, std::unordered_map<std::string, std::vector<StreamEntry>>& streamStore)
{
    string resp = "";
    if (store.find(key) != store.end()) {
        resp = "string";
    }
    else if (listStore.find(key) != listStore.end()) {
        resp = "list";
    }
    else if (streamStore.find(key) != streamStore.end()) {
        resp = "stream";
    }
    else
        resp = "none";
    resp = RESPBulkStringEncoder(resp);
    return resp;
}
