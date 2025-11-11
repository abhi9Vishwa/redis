#include <iostream>
#include <cstdlib>
#include <string>
#include <chrono>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <bits/stdc++.h>
using namespace std;

int server_fd = socket(AF_INET, SOCK_STREAM, 0);

struct StreamEntry {
    string id;
    unordered_map<string, string> fields;
};

unordered_map<string, vector<StreamEntry>> streamStore;
mutex stream_mtx;

unordered_map<string, string> store;
mutex store_mtx;

unordered_map<string, long long> expiry;

// to be implemented later
unordered_map<string, vector<string>> listStore;

long long now_ms(){
    using namespace chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

void handle_sigint(int) {
    close(server_fd);
    exit(0);
}

vector<string> RESPArrayParser(string& rawInput){
  size_t pos = 0;
    if (rawInput[pos] != '*')
        throw runtime_error("Invalid RESP: missing array prefix '*'");

    // Parse array length
    size_t end = rawInput.find("\r\n", pos);
    if (end == string::npos) throw runtime_error("Invalid RESP: no CRLF after array header");
    int arraySize = stoi(rawInput.substr(1, end - 1));
    pos = end + 2;

    vector<string> result;
    for (int i = 0; i < arraySize; i++) {
        if (rawInput[pos] != '$')
            throw runtime_error("Invalid RESP: missing bulk string prefix '$'");

        end = rawInput.find("\r\n", pos);
        if (end == string::npos) throw runtime_error("Invalid RESP: no CRLF after length");
        int strLen = stoi(rawInput.substr(pos + 1, end - pos - 1));
        pos = end + 2;

        // Extract string of given length
        string value = rawInput.substr(pos, strLen);
        result.push_back(value);
        pos += strLen + 2; // skip string and its CRLF
    }

    return result;
}

string RESPBulkStringEncoder(string str){
    if(str == "")return "$-1\r\n";
    size_t len = str.size();
    return ("$" + to_string(len) + "\r\n" + str + "\r\n");
}

void handleEcho(string & str, int clientFd){
    send(clientFd, str.c_str(), str.size(), 0);
}

void handleSet(string& key, string& val, int clientFd, string opt = "", string expiryStr = "-1"){
    int expiryVal = stoi(expiryStr);
    long long expire_at = -1;

    if(opt == "EX"){
        expire_at = now_ms() + expiryVal * 1000LL;
    }
    else if(opt == "PX"){
        expire_at = now_ms() + expiryVal;
    }

    {
        lock_guard<mutex> lock(store_mtx); 
        store[key] = val;
        if(expire_at != -1) expiry[key] = expire_at;
        else expiry.erase(key);
    }
    string resp = "+OK\r\n";
    send(clientFd, resp.c_str(), resp.size(), 0);
}

void handleGet(string& key, int clientFd){
    string resp = "";
    lock_guard<mutex> lock(store_mtx);

    if(store.find(key) == store.end()){
        resp = RESPBulkStringEncoder("");
    }
    else{
        if(expiry.find(key) != expiry.end() && now_ms() >= expiry[key]){
            store.erase(key);
            expiry.erase(key);
            resp = RESPBulkStringEncoder("");
        }
        else{
            resp = RESPBulkStringEncoder(store[key]);
        }
    }
    send(clientFd, resp.c_str(), resp.size(), 0);
}

void handleType(string& key, int client_fd){
    string resp ="";
    if(store.find(key) != store.end()){
        resp = "string";
    }else if(listStore.find(key) != listStore.end()){
        resp = "list";
    } 
    else if(streamStore.find(key) != streamStore.end()){
        resp = "stream";
    }
    else resp = "none";
    resp = RESPBulkStringEncoder(resp);
    send(client_fd, resp.c_str(), resp.size(), 0);
}

void handleStreamAdd(vector<string>& input, int& client_fd){
    string streamId = input[1];
    StreamEntry entry;
    entry.id = input[2];
    for (size_t i = 3; i < input.size(); i+=2){
        entry.fields[input[i]] = input[i+1];
    }
    {
        lock_guard lock(stream_mtx);
        streamStore[streamId].push_back(entry);
    }

    string resp = RESPBulkStringEncoder(input[2]);
    send(client_fd, resp.c_str(), resp.size(), 0);
}

void handle_client(int client_fd) {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_recv = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes_recv <= 0) break;
        string req(buffer, bytes_recv);
        if (req.find("PING") != string::npos) {
            string response = "+PONG\r\n";
            send(client_fd, response.c_str(), response.size(), 0);
        }
        else{
          vector<string> cmds = RESPArrayParser(req);
          for(int i =0; i < cmds.size(); i++){
            if(cmds[i] == "ECHO"){
                handleEcho(cmds[1], client_fd);
            }
            if(cmds[i] == "SET"){
                if(cmds.size() == 5){
                    handleSet(cmds[1], cmds[2], client_fd, cmds[3], cmds[4]);
                }
                else if(cmds.size() == 3) handleSet(cmds[1], cmds[2], client_fd);
            }
            if(cmds[i] == "GET"){
                handleGet(cmds[1], client_fd);
            }
            if(cmds[i] == "TYPE"){
                handleType(cmds[i+1], client_fd);
            }
            if(cmds[0] == "XADD"){
                handleStreamAdd(cmds, client_fd);
            }
          }
        }
    }
    close(client_fd);
}
int main(int argc, char **argv) {
  signal(SIGINT, handle_sigint);
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(6379);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 6379\n";
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

  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  // Uncomment this block to pass the first stage

  while (true)
  {
	  int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
	  std::cout << "Client connected\n";
	  thread(handle_client, client_fd).detach();
  }
  
  
  close(server_fd);

  return 0;
}
