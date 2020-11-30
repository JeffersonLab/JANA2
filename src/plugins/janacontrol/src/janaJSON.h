
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <sstream>
#include <vector>
#include <string>
#include <map>

#include <JANA/JObject.h>


// Generic (double, float, int, uint64_t, ...)
template <typename T>
void JJSON_Add(std::stringstream &ss, const T &t, int &indent_level){
    ss << std::string(indent_level*2, ' ') << t;
}

// std::string
//template <typename T>
void JJSON_Add(std::stringstream &ss, const std::string &s, int &indent_level){
    // Next 3 lines decide whether to put quotes around string
    bool has_whitespace = s.find_first_not_of("\t\n ") != string::npos;
    bool is_json = s.find("{") == 0;
    std::string quote = ((has_whitespace && !is_json) || s.empty()) ? "\"":"";
    ss << std::string(indent_level*2, ' ') << quote << s << quote;
}

// std::vector
template <typename T>
void JJSON_Add(std::stringstream &ss, const std::vector<T> &V, int &indent_level){

    ss << std::string(indent_level*2, ' ') << "[\n";
    indent_level++;
    auto items_left = V.size();
    for( auto v : V){
        JJSON_Add(ss, v, indent_level);
        if(--items_left != 0) ss << ",";
        ss << "\n";
    }
    indent_level--;
    ss << std::string(indent_level*2, ' ') << "]\n";
}

// std::map
template <typename T>
void JJSON_Add(std::stringstream &ss, const std::map<std::string, T> &M, int &indent_level){

    ss << std::string(indent_level*2, ' ') << "{\n";
    indent_level++;
    auto items_left = M.size();
    for( auto p : M){
        ss << std::string(indent_level*2, ' ') << "\"" << p.first << "\":";
        JJSON_Add(ss, p.second, indent_level);
        if(--items_left != 0) ss << ",";
        ss << "\n";
    }
    indent_level--;
    ss << std::string(indent_level*2, ' ') << "}\n";
}
// std::unordered_map
template <typename T>
void JJSON_Add(std::stringstream &ss, const std::unordered_map<std::string, T> &M, int &indent_level){

    ss << std::string(indent_level*2, ' ') << "{\n";
    indent_level++;
    auto items_left = M.size();
    for( auto p : M){
        ss << std::string(indent_level*2, ' ') << "\"" << p.first << "\":";
        JJSON_Add(ss, p.second, indent_level);
        if(--items_left != 0) ss << ",";
        ss << "\n";
    }
    indent_level--;
    ss << std::string(indent_level*2, ' ') << "}\n";
}

// JObjectSummary
void JJSON_Add(std::stringstream &ss, const JObjectSummary &jobj_summary, int &indent_level) {
    auto fields = jobj_summary.get_fields();
    JJSON_Add(ss, fields, indent_level);
}

// JObjectMember
void JJSON_Add(std::stringstream &ss, const JObjectMember &jobj_member, int &indent_level) {
    std::unordered_map<std::string, std::string> vals;
    vals["name"] = jobj_member.name;
    vals["type"] = jobj_member.type;
    vals["value"] = jobj_member.value;
    JJSON_Add(ss, vals, indent_level);
}

template<typename T>
std::string JJSON_Create(const T &t, int indent_level = 0){
    std::stringstream ss;
    JJSON_Add(ss, t, indent_level);
    return ss.str();
}
