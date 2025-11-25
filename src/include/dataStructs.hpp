#pragma once

#include <bits/stdc++.h>

struct StreamEntry {
    std::string id;
    std::unordered_map<std::string, std::string> fields;
};

struct ReplicaInfo {
    int sock_fd;
    long long ackedOffset;
    long long lastSeenTime;
};

struct RedisAllData {
    RedisAllData() = default;

    RedisAllData(const RedisAllData&) = delete;
    RedisAllData& operator=(const RedisAllData&) = delete;

    std::unordered_map<std::string, std::vector<StreamEntry>> streamStore;
    std::mutex stream_mtx;

    std::unordered_map<std::string, std::string> store;
    std::mutex store_mtx;

    std::unordered_map<std::string, std::condition_variable> streamCVs;
    std::unordered_map<std::string, std::mutex> streamMtxMap;

    std::unordered_map<std::string, long long> expiry;

    std::unordered_map<int, std::queue<std::vector<std::string>>> multiQueue;
    std::unordered_map<int, bool> inMulti;
    
    // std::vector<ReplicaInfo> allReplicas;
    std::mutex replicaMtx;
    std::unordered_map<int, ReplicaInfo> allReplicas;

    std::unordered_map<int, long long> repLastOffset;

    // to be implemented later
    std::unordered_map<std::string, std::vector<std::string>> listStore;
};