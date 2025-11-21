#include "helperFunc.hpp"

#include <bits/stdc++.h>  

std::string RESPBulkStringEncoder(std::string str)
{
    if (str == "")
        return "$-1\r\n";
    size_t len = str.size();
    return ("$" + std::to_string(len) + "\r\n" + str + "\r\n");
}