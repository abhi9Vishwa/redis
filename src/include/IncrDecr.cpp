#include "IncrDecr.hpp"
#include <bits/stdc++.h>
#include "helperFunc.hpp"

using namespace std;
using ll = long long;

std::string handleIncr(std::string& key, std::mutex& store_mtx, std::unordered_map<std::string, std::string>& store)
{
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