#pragma once
#include <string>
namespace erp::search {
// Treat user-entered %, _ and backslashes as text rather than SQL wildcards.
inline std::string containsPattern(const std::string &input) {
    auto first=input.find_first_not_of(" \t\r\n");
    if(first==std::string::npos)return "%";
    auto last=input.find_last_not_of(" \t\r\n");
    std::string pattern="%";
    for(auto ch:input.substr(first,last-first+1)) {
        if(ch=='%'||ch=='_'||ch=='\\')pattern+='\\';
        pattern+=ch;
    }
    return pattern+"%";
}
}
