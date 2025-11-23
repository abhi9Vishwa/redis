#pragma once
#include <bits/stdc++.h>
#include "dataStructs.hpp"


std::string handleStreamAdd(std::vector<std::string>& input, std::mutex& stream_mtx, std::unordered_map<std::string, std::vector<StreamEntry>>& streamStore, std::unordered_map<std::string, std::condition_variable>& streamCVs);


std::vector<StreamEntry> getXRangeEntry(const std::string& streamId, const std::string& entryIdStart, const std::string& entryIdEnd,  std::unordered_map<std::string, std::vector<StreamEntry>>& streamStore);


std::string handleXRange(const std::string& streamId, const std::string& entryIdStart, const std::string& entryIdEnd, std::mutex& stream_mtx, std::unordered_map<std::string, std::vector<StreamEntry>>& streamStore);

std::string handleXRead(std::vector<std::string>& cmd, std::mutex& stream_mtx, std::unordered_map<std::string, std::vector<StreamEntry>>& streamStore, std::unordered_map<std::string, std::condition_variable>& streamCVs);


