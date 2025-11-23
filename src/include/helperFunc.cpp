#include "helperFunc.hpp"

#include <bits/stdc++.h>  
#include "dataStructs.hpp"

using namespace std;

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

std::string RESPBulkStringEncoder(std::string str)
{
    if (str == "")
        return "$-1\r\n";
    size_t len = str.size();
    return ("$" + std::to_string(len) + "\r\n" + str + "\r\n");
}

std::string recvData(int& fd)
{
    char buffer[4096];
    int bytesReceived = recv(fd, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0'; // Null-terminate the received data
        std::cout << "Received from server: " << buffer << "\n";
    }
    else if (bytesReceived == 0) {
        std::cout << "Server closed the connection.\n";
    }
    return std::string(buffer);
}


std::vector<std::string> RESPArrayParser(std::string& rawInput) {
    size_t pos = 0;
    if (rawInput[pos] != '*')
        throw std::runtime_error("Invalid RESP: missing array prefix '*'");

    // Parse array length
    size_t end = rawInput.find("\r\n", pos);
    if (end == std::string::npos)
        throw std::runtime_error("Invalid RESP: no CRLF after array header");
    int arraySize = stoi(rawInput.substr(1, end - 1));
    pos = end + 2;

    std::vector<std::string> result;
    for (int i = 0; i < arraySize; i++) {
        if (rawInput[pos] != '$')
            throw std::runtime_error("Invalid RESP: missing bulk string prefix '$'");

        end = rawInput.find("\r\n", pos);
        if (end == std::string::npos)
            throw std::runtime_error("Invalid RESP: no CRLF after length");
        int strLen = stoi(rawInput.substr(pos + 1, end - pos - 1));
        pos = end + 2;

        // Extract string of given length
        std::string value = rawInput.substr(pos, strLen);
        result.push_back(value);
        pos += strLen + 2;	// skip string and its CRLF
    }

    return result;
}

void pushSingleByte(const uint8_t& data, std::vector<uint8_t>& container) {
    container.push_back(data);
}

void pushBytes(void* startPtr, int lenToPush, std::vector<uint8_t>& container) {
    uint8_t* ptr = (uint8_t*)startPtr;
    container.insert(container.end(), ptr, ptr + lenToPush);
}


template <typename T>
void encodeLength(T& len, std::vector<uint8_t>& container) {
    pushSingleByte(0x80, container);
    uint32_t lenB = htonl(len);
    uint8_t* dataPtr = reinterpret_cast<uint8_t*> (&len);

    std::vector<uint8_t> data;
    for (int i = 0; i < sizeof(T) / sizeof(uint8_t); i++) {
        data.push_back(dataPtr[i]);
    }
    pushBytes(data.data(), data.size(), container);
}

std::vector<uint8_t> getEmptyRdb() {
    std::vector<uint8_t> rdb;
    std::string header = "REDIS0001";
    std::vector<uint8_t> headerBytes(header.begin(), header.end());
    pushBytes(headerBytes.data(), 9, rdb);
    pushSingleByte(0xFE, rdb);
    pushSingleByte(0x00, rdb);
    pushSingleByte(0xFF, rdb);

    uint64_t checksum = 0;
    pushBytes(&checksum, 8, rdb);
    return rdb;
}




pair<long long, long long> parseStreamId(const string& id, int end) {
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
using ll = long long;

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

size_t sendData(string resp, int& client_fd) {
    size_t sent = send(client_fd, resp.c_str(), resp.size(), 0);
    return sent;
}

bool isWriteCommand(const std::string& cmd) {
    static unordered_set<string> writeCmds = {
        "SET", "DEL", "XADD", "HSET", "LPUSH", "RPUSH", "INCR", "DECR", "MULTI", "EXEC"
    };
    string upper = cmd;
    transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    return writeCmds.count(upper);
}