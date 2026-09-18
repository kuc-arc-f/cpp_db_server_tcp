#pragma once
#include <iostream>

struct QueItem {
    std::string sql;
    std::string uuid;
};

struct ActionReq {
    std::string action_name;
    std::string table;
    std::string sql;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ActionReq, action_name, table, sql)

struct LargeJsonRes {
    std::string result;
    std::string text;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LargeJsonRes, result , text)

struct ActionSelectReq {
    std::string action_name;
    std::string table;
    std::string sql;
};
