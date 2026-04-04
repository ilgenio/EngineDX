#pragma once

#include "json11.hpp"

#include <fstream>
#include <sstream>
#include <string>

using namespace json11;

inline std::string readFile(const std::string& path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

inline void writeFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    file << content;
}


inline Json::object serializeVector3(const Vector3& vec) 
{
    Json::object json;
    json["x"] = double(vec.x);
    json["y"] = double(vec.y);
    json["z"] = double(vec.z);

    return json;
}

inline Vector3 deserializeVector3(const Json& json) 
{
    Vector3 vec;

    vec.x = float(json["x"].number_value());
    vec.y = float(json["y"].number_value());
    vec.z = float(json["z"].number_value());

    return vec;
}

