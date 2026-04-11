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

inline Json::object serializeVector4(const Vector4& vec)
{
    Json::object json;
    json["x"] = double(vec.x);
    json["y"] = double(vec.y);
    json["z"] = double(vec.z);
    json["w"] = double(vec.w);

    return json;
}

inline Vector4 deserializeVector4(const Json& json)
{
    Vector4 vec;

    vec.x = float(json["x"].number_value());
    vec.y = float(json["y"].number_value());
    vec.z = float(json["z"].number_value());
    vec.w = float(json["w"].number_value());

    return vec;
}

inline Json::object serializeQuaternion(const Quaternion& quat)
{
    Json::object json;
    json["x"] = double(quat.x);
    json["y"] = double(quat.y);
    json["z"] = double(quat.z);
    json["w"] = double(quat.w);

    return json;
}

inline Quaternion deserializeQuaternion(const Json& json)
{
    Quaternion quat;
    quat.x = float(json["x"].number_value());
    quat.y = float(json["y"].number_value());
    quat.z = float(json["z"].number_value());
    quat.w = float(json["w"].number_value());

    return quat;
}

inline Json::array serializeMatrix(const Matrix& mat)
{
    Json::array json;
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            json.push_back(double(mat(i, j)));
        }
    }

    return json;
}

inline Matrix deserializeMatrix(const Json& json)
{
    Matrix mat;

    const Json::array& arr = json.array_items();
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            mat(i, j) = float(arr[i * 4 + j].number_value());
        }
    }

    return mat;
}

