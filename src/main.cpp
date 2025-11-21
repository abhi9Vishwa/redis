#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "infoClass.hpp"
#include "helperFunc.hpp"
#include "handleCmds.hpp"

using namespace std;

using ll = long long;

int server_fd = socket(AF_INET, SOCK_STREAM, 0);

struct StreamEntry {
    string id;
    unordered_map<string, string> fields;
};

unordered_map<string, vector<StreamEntry>> streamStore;
mutex stream_mtx;

unordered_map<string, string> store;
mutex store_mtx;

unordered_map<string, condition_variable> streamCVs;
unordered_map<string, mutex> streamMtxMap;

unordered_map<string, long long> expiry;

unordered_map<int, queue<vector<string>>> multiQueue;
// to be implemented later
unordered_map<string, vector<string>> listStore;

long long now_ms() {
    using namespace chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch())
        .count();
}

bool checkIsStrNum(const string& s) {
    int n = s.size();
    int i = 0;
    while (s[i] == ' ') { i++; }
    if (i == n) return false;

    if (s[i] == '-' || s[i] == '+') i++;
    if (i == n) return false;

    bool hasDigit = false;
    while (i < n && isdigit(s[i])) {
        hasDigit = true;
        i++;
    }

    // skip trailing spaces
    while (i < n && s[i] == ' ') i++;

    // valid only if all characters are processed and at least one digit found
    return hasDigit && i == n;
}

pair<long long, long long> parseStreamId(const string& id, int end = 0) {
    size_t sepIdx = id.find('-');
    if (sepIdx == string::npos) {
        if (end == 0)
            return { stoll(id), 0 };
        else
            return { stoll(id), INT_MAX };
    }
    long long a = stoll(id.substr(0, sepIdx));
    long long b = stoll(id.substr(sepIdx + 1));

    return { a, b };
}

int binarySearch(const string& target, const vector<StreamEntry>& vec) {
    int l = 0;
    int r = vec.size() - 1;

    pair<ll, ll> tarPair = parseStreamId(target);

    while (l <= r) {
        int m = r - (l + r) / 2;

        pair<ll, ll> mp = parseStreamId(vec[m].id);
        if (mp == tarPair)
            return m;
        else if (mp < tarPair)
            l = m + 1;
        else
            r = m - 1;
    }
    return l;
}

void handle_sigint(int) {
    close(server_fd);
    exit(0);
}

void sendData(string& resp, int& client_fd) {
    send(client_fd, resp.c_str(), resp.size(), 0);
}

vector<string> RESPArrayParser(string& rawInput) {
    size_t pos = 0;
    if (rawInput[pos] != '*')
        throw runtime_error("Invalid RESP: missing array prefix '*'");

    // Parse array length
    size_t end = rawInput.find("\r\n", pos);
    if (end == string::npos)
        throw runtime_error("Invalid RESP: no CRLF after array header");
    int arraySize = stoi(rawInput.substr(1, end - 1));
    pos = end + 2;

    vector<string> result;
    for (int i = 0; i < arraySize; i++) {
        if (rawInput[pos] != '$')
            throw runtime_error("Invalid RESP: missing bulk string prefix '$'");

        end = rawInput.find("\r\n", pos);
        if (end == string::npos)
            throw runtime_error("Invalid RESP: no CRLF after length");
        int strLen = stoi(rawInput.substr(pos + 1, end - pos - 1));
        pos = end + 2;

        // Extract string of given length
        string value = rawInput.substr(pos, strLen);
        result.push_back(value);
        pos += strLen + 2;	// skip string and its CRLF
    }

    return result;
}

// string RESPBulkStringEncoder(string str) {
//     if (str == "")
//         return "$-1\r\n";
//     size_t len = str.size();
//     return ("$" + to_string(len) + "\r\n" + str + "\r\n");
// }

string handleEcho(string& str) {
    return str;
}

string handleSet(string& key, string& val, string opt = "", string expiryStr = "-1") {
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

string handleGet(string& key) {
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

string handleType(string& key) {
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

string handleStreamAdd(vector<string>& input) {
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

string RESPEncodeStream(vector<StreamEntry>& vec) {
    if (vec.empty()) return "$-1\r\n";
    string res;

    size_t totSize = vec.size();
    res = string("*") + to_string(totSize) + "\r\n";
    res += string("*") + "2" + "\r\n";	// to account for StreamEntry

    for (auto& i : vec) {
        res += "$" + to_string(i.id.size()) + "\r\n" + i.id + "\r\n";
        res += string("*") + to_string(i.fields.size() * 2) + "\r\n";
        for (auto& j : i.fields) {
            res += "$" + to_string(j.first.size()) + "\r\n" + j.first + "\r\n";
            res +=
                "$" + to_string(j.second.size()) + "\r\n" + j.second + "\r\n";
        }
    }
    return res;
}

vector<StreamEntry> getXRangeEntry(const string& streamId, const string& entryIdStart, const string& entryIdEnd) {
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

string handleXRange(const string& streamId, const string& entryIdStart, const string& entryIdEnd) {
    string resp;
    vector<StreamEntry> res;
    {
        lock_guard lock(stream_mtx);
        res = getXRangeEntry(streamId, entryIdStart, entryIdEnd);
    }
    resp = RESPEncodeStream(res);
    return resp;
}

string handleXRead(vector<string>& cmd) {
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
            rangeRes = getXRangeEntry(streamId, startId, "+");
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
                rangeRes = getXRangeEntry(streamId, startId, "+");
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

string handleIncr(string& key) {
    string resp = "";
    lock_guard lock(store_mtx);

    if (store.find(key) == store.end()) {
        store[key] = "1";
    }
    else {
        if (checkIsStrNum(store[key])) {
            ll t = stoll(store[key]);
            t += 1;
            store[key] = to_string(t);
        }
        else {
            return "-ERR value is not an integer or out of range";
        }
    }
    resp = ":" + store[key] + "\r\n";
    return resp;
}

void processCmds(vector<string>& cmds, int& client_fd);

void handleExec(int& client_fd) {
    // TODO: Solve the bug in the response. Response spill over to next command.
    // TODO: Return response as single RESP array rather than individual strings  
    string resp;
    if (multiQueue.count(client_fd) == 0) {
        resp = "-ERR EXEC without MULTI";
        sendData(resp, client_fd);
    }
    else {
        if (multiQueue[client_fd].size() == 0) {
            resp = "*0\r\n";
            sendData(resp, client_fd);
        }
        while (!multiQueue[client_fd].empty()) {
            vector<string> cmds = multiQueue[client_fd].front();
            multiQueue[client_fd].pop();
            processCmds(cmds, client_fd);
        }
        multiQueue.erase(client_fd);
    }
}

string handleMulti(int& client_fd) {
    string resp = "";
    resp += "+OK\r\n";
    sendData(resp, client_fd);
    bool execFlag = false;

    char buffer[4096];
    if (multiQueue.find(client_fd) == multiQueue.end()) multiQueue[client_fd] = queue<vector<string>>();
    string req;
    while (true) {
        int bytes_recv = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes_recv <= 0)
            break;

        req.append(buffer, bytes_recv);
        vector<string> cmds = RESPArrayParser(req);
        cout << cmds[0] << endl;
        if (cmds[0] == "EXEC") {
            handleExec(client_fd);
            break;
        }
        else if (cmds[0] == "DISCARD") {
            if (multiQueue.find(client_fd) == multiQueue.end()) {
                string resp = "-ERR DISCARD without MULTI";
                sendData(resp, client_fd);
            }
            else {
                multiQueue.erase(client_fd);
                string qres = "+OK\r\n";
                sendData(qres, client_fd);
            }
        }
        else {
            multiQueue[client_fd].push(cmds);
            string qres = RESPBulkStringEncoder("QUEUED");
            sendData(qres, client_fd);
        }
        req.clear();
    }
    cout << "Exit func" << endl;
    return "";
}



void processCmds(vector<string>& cmds, int& client_fd) {
    string cmd = cmds[0];
    cout << "Process: " << cmd << endl;
    if (cmd == "PING") {
        string res = "+PONG\r\n";
        sendData(res, client_fd);

    }
    else if (cmd == "ECHO" && cmds.size() >= 2) {
        string res = handleEcho(cmds[1]);
        sendData(res, client_fd);

    }
    else if (cmd == "SET") {
        if (cmds.size() == 5) {
            string res = handleSet(cmds[1], cmds[2], cmds[3], cmds[4]);
            sendData(res, client_fd);
        }
        else if (cmds.size() >= 3) {
            string res = handleSet(cmds[1], cmds[2]);
            sendData(res, client_fd);
        }
    }
    else if (cmd == "GET" && cmds.size() >= 2) {
        string res = handleGet(cmds[1]);
        sendData(res, client_fd);
    }
    else if (cmd == "TYPE" && cmds.size() >= 2) {
        string res = handleType(cmds[1]);
        sendData(res, client_fd);
    }
    else if (cmd == "XADD") {
        string res = handleStreamAdd(cmds);
        sendData(res, client_fd);
    }
    else if (cmd == "XRANGE") {
        string res = handleXRange(cmds[1], cmds[2], cmds[3]);
        sendData(res, client_fd);
    }
    else if (cmd == "XREAD") {
        string res = handleXRead(cmds);
        sendData(res, client_fd);
    }
    else if (cmd == "INCR") {
        string res = handleIncr(cmds[1]);
        sendData(res, client_fd);
    }
    else if (cmd == "MULTI") {
        string res = handleMulti(client_fd);
        // sendData(res, client_fd);
    }
    else if (cmd == "EXEC") {
        cout << "second exec" << endl;
        string res = handleMulti(client_fd);
        // sendData(res, client_fd);
    }
    // else if (cmd == "INFO") {
    //     if (cmds[1] == "replication") {
    //         string res = handleInfo(client_fd);
    //     }
    // }
    else {
        string err = "-ERR unknown command\r\n";
        send(client_fd, err.c_str(), err.size(), 0);
    }
}

void handle_client(int client_fd) {
    try {
        char buffer[4096];
        string req;
        while (true) {
            memset(buffer, 0, sizeof(buffer));
            int bytes_recv = recv(client_fd, buffer, sizeof(buffer), 0);
            if (bytes_recv <= 0)
                break;

            req.append(buffer, bytes_recv);
            if (req.find("\r\n") == string::npos)
                continue;

            try {
                vector<string> cmds = RESPArrayParser(req);
                if (cmds.empty()) {
                    string err = "-ERR empty command\r\n";
                    send(client_fd, err.c_str(), err.size(), 0);
                    req.clear();
                    continue;
                }
                processCmds(cmds, client_fd);
            }
            catch (const exception& e) {
                string err = string("-ERR ") + e.what() + "\r\n";
                send(client_fd, err.c_str(), err.size(), 0);
            }

            req.clear();
        }
    }
    catch (const exception& e) {
        cerr << "client handler crashed: " << e.what() << endl;
    }
    close(client_fd);
}
int main(int argc, char** argv) {
    signal(SIGINT, handle_sigint);
    int port = 6379;
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    if (server_fd < 0) {
        std::cerr << "Failed to create server socket\n";
        return 1;
    }
    // string portIn = argv[1];
    // // get Port number 
    // if (argc == 3 && portIn == "--port") {
    //     try {
    //         port = stoi(argv[2]);
    //     }
    //     catch (const std::exception& e) {
    //         cerr << "Invalid port provided" << endl;
    //         std::cerr << e.what() << '\n';
    //     }
    // }


    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) <
        0) {
        std::cerr << "setsockopt failed\n";
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) !=
        0) {
        std::cerr << "Failed to bind to port " << port << "\n";
        return 1;
    }

    int connection_backlog = 5;
    if (listen(server_fd, connection_backlog) != 0) {
        std::cerr << "listen failed\n";
        return 1;
    }

    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    std::cout << "Waiting for a client to connect...\n";

    // You can use print statements as follows for debugging, they'll be visible
    // when running tests.
    std::cout << "Logs from your program will appear here!\n";

    // Uncomment this block to pass the first stage

    while (true) {
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, (socklen_t*)&client_addr_len);
        std::cout << "Client connected\n";
        thread(handle_client, client_fd).detach();
    }

    close(server_fd);

    return 0;
}
