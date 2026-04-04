#pragma once

#include "json.h"

#include <fstream>
#include <sstream>
#include <string>


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


inline void serialize(const Vector3& vec, json::jobject& json) {
    json["x"] = vec.x;
    json["y"] = vec.y;
    json["z"] = vec.z;
}

inline void deserialize(const json::jobject& json, Vector3& vec) {
    vec.x = json["x"];
    vec.y = json["y"];
    vec.z = json["z"];
}