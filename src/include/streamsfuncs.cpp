#include "streamsfuncs.hpp"
#include <bits/stdc++.h>
#include "helperFunc.hpp"

using namespace std;

using ll = long long;

std::string handleStreamAdd(std::vector<std::string>& input, mutex& stream_mtx, unordered_map<string, vector<StreamEntry>>& streamStore, unordered_map<string, condition_variable>& streamCVs)
{
    if (input.size() < 4 || (input.size() - 3) % 2 != 0) {
        string err = "-ERR wrong number of arguments for 'xadd' command\r\n";
        return err;
    }

    string streamId = input[1];
    string givenId = input[2];
    StreamEntry entry;
    string resp;
    int flag = 0;
    long long na;
    long long nb;
    cout << "Input :" << givenId << endl;
    if (givenId == "*") {
        flag = 1;
    }
    else {
        size_t sepIdx = givenId.find('-');
        if (sepIdx == string::npos) {
            string err = "-ERR wrong entry id format\r\n";
            return err;
        }

        try {
            na = stoll(givenId.substr(0, sepIdx));
            string seqPart = givenId.substr(sepIdx + 1);
            if (seqPart == "*")
                flag = 2;
            else {
                nb = stoll(seqPart);
                flag = 3;
            }
        }
        catch (...) {
            string err = "-ERR invalid stream ID\r\n";
            return err;
        }
    }
    cout << "flag: " << flag << endl;
    for (size_t i = 3; i < input.size(); i += 2) {
        entry.fields[input[i]] = input[i + 1];
    }

    {
        lock_guard lock(stream_mtx);
        if (streamStore.find(streamId) == streamStore.end())
            streamStore[streamId] = vector<StreamEntry>();

        vector<StreamEntry>& vec = streamStore[streamId];
        long long last_ms = 0, last_seq = 0;

        if (!vec.empty()) {
            string lastId = vec.back().id;
            size_t sep = lastId.find('-');
            last_ms = stoll(lastId.substr(0, sep));
            last_seq = stoll(lastId.substr(sep + 1));
        }

        if (flag == 1) {
            na = now_ms();
            if (na == last_ms)
                nb = last_seq + 1;
            else
                nb = 0;
        }
        else if (flag == 2) {
            if (na < last_ms) {
                string err =
                    "(error) ERR The ID specified in XADD must be greater "
                    "than " +
                    to_string(last_ms) + "-" + to_string(last_seq);
                return err;
            }
            if (na == last_ms)
                nb = last_seq + 1;
            else
                nb = 0;
        }
        if (flag == 3) {
            if (na < last_ms || (na == last_ms && nb <= last_seq)) {
                string err =
                    "-ERR The ID specified in XADD must be greater than the "
                    "last to" +
                    to_string(last_ms) + "-" + to_string(last_seq) + "\r\n";
                return err;
            }
        }
        entry.id = to_string(na) + '-' + to_string(nb);
        vec.push_back(entry);
    }
    resp = "$" + to_string(entry.id.size()) + "\r\n" + entry.id + "\r\n";

    if (streamCVs.find(streamId) != streamCVs.end()) {
        cout << "cond variable found" << endl;
        streamCVs[streamId].notify_all();
    }
    return resp;
}

vector<StreamEntry> getXRangeEntry(const string& streamId, const string& entryIdStart, const string& entryIdEnd, std::unordered_map<std::string, std::vector<StreamEntry>>& streamStore)
{
auto& data = streamStore[streamId];
    if (data.empty()) { // empty array
        return {};
    }
    int stIdx;
    if (entryIdStart == "-")
        stIdx = 0;
    else
        stIdx = binarySearch(entryIdStart, data);

    pair<ll, ll> endID;
    if (entryIdEnd == "+")
        endID = { LLONG_MAX, LLONG_MAX };
    else
        endID = parseStreamId(entryIdEnd, 1);
    vector<StreamEntry> res;
    while (stIdx < data.size() && parseStreamId(data[stIdx].id) <= endID) {
        res.push_back(data[stIdx]);
        stIdx++;
    }
    return res;
}

string handleXRange(const string& streamId, const string& entryIdStart, const string& entryIdEnd, std::mutex& stream_mtx, std::unordered_map<std::string, std::vector<StreamEntry>>& streamStore)
{
    string resp;
    vector<StreamEntry> res;
    {
        lock_guard lock(stream_mtx);
        res = getXRangeEntry(streamId, entryIdStart, entryIdEnd, streamStore);
    }
    resp = RESPEncodeStream(res);
    return resp;
}

std::string handleXRead(std::vector<std::string>& cmd, std::mutex& stream_mtx, std::unordered_map<std::string, std::vector<StreamEntry>>& streamStore, std::unordered_map<std::string, std::condition_variable>& streamCVs)
{
    int streamsPos = -1;
    int blockMs = -1;

    for (int i = 1; i < cmd.size(); i++) {
        if (cmd[i] == "BLOCK") {
            if (i + 1 >= cmd.size()) return "-ERR syntax error\r\n";
            blockMs = stoi(cmd[++i]);
        }
        else if (cmd[i] == "STREAMS") {
            streamsPos = i;
            break;
        }
    }

    if (streamsPos == -1 || streamsPos + 1 >= cmd.size()) {
        return "-ERR syntax error\r\n";
    }

    int numStreams = (cmd.size() - streamsPos - 1) / 2;
    if (numStreams <= 0) {
        return "-ERR wrong number of arguments for 'XREAD'\r\n";
    }

    vector<string> streams;
    vector<string> ids;

    for (int i = streamsPos + 1; i < streamsPos + 1 + numStreams; i++)
        streams.push_back(cmd[i]);
    for (int i = streamsPos + 1 + numStreams; i < cmd.size(); i++)
        ids.push_back(cmd[i]);

    if (streams.size() != ids.size()) {
        return "-ERR number of streams and IDs must match\r\n";
    }

    // Build response array
    string resp = "*" + to_string(streams.size()) + "\r\n";

    for (int i = 0; i < streams.size(); i++) {
        string streamId = streams[i];
        string startId = ids[i];
        vector<StreamEntry> rangeRes = {};

        {
            // Call XRANGE-like helper to get entries newer than startId
            lock_guard lock(stream_mtx);
            if (startId == "$") {
                if (streamStore.find(streamId) != streamStore.end() && !streamStore[streamId].empty()) {
                    startId = streamStore[streamId].back().id;
                }
                else {
                    startId = "0-0"; // empty stream — start from nothing
                }
            }
            rangeRes = getXRangeEntry(streamId, startId, "+", streamStore);
        }

        //check for existing entries
        if (blockMs != -1 && rangeRes.empty()) {
            unique_lock ulock(stream_mtx);

            if (streamCVs.find(streamId) == streamCVs.end()) {
                streamCVs[streamId];
            }
            bool gotNew = true;
            if (blockMs == 0) {
                streamCVs[streamId].wait(
                    ulock, [&] {
                        if (!streamStore.count(streamId) || streamStore[streamId].empty())
                            return false;
                        auto lastIdPair = parseStreamId(streamStore[streamId].back().id);
                        return lastIdPair > parseStreamId(startId);
                    }
                );
            }
            else {
                bool gotNew = streamCVs[streamId].wait_for(
                    ulock, chrono::milliseconds(blockMs), [&] {
                        if (!streamStore.count(streamId) || streamStore[streamId].empty())
                            return false;
                        auto lastIdPair = parseStreamId(streamStore[streamId].back().id);
                        return lastIdPair > parseStreamId(startId);
                    }
                );

            }
            if (gotNew) {
                // Fetch only the NEW entries that arrived during blocking
                rangeRes = getXRangeEntry(streamId, startId, "+", streamStore);
                ulock.unlock();
            }
            else {
                ulock.unlock();
                return "$-1\r\n";
            }
        }

        if (rangeRes.empty()) continue;

        string rangeResp = RESPEncodeStream(rangeRes);

        for (auto t : rangeRes) {
            cout << "id: " << t.id << endl;
            for (auto tf : t.fields) {
                cout << "f1 " << tf.first << " f2 " << tf.second << endl;
            }
        }

        // Stream response = [streamName, entriesArray]
        resp += "*2\r\n";
        resp += "$" + to_string(streamId.size()) + "\r\n" + streamId + "\r\n";
        resp += rangeResp;
    }

    if (resp == "*0\r\n")
        return "$-1\r\n";  // nothing found
    return resp;
}
