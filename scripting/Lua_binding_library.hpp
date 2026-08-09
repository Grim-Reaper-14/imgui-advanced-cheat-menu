#pragma once

#include <string>
#include <vector>

class LuaBindingLibrary {
public:
    static bool settingExists(const std::string& id, const std::string& group = "");
    static bool getBool(const std::string& id, const std::string& group, bool fallback = false);
    static bool setBool(const std::string& id, const std::string& group, bool value);
    static int getInt(const std::string& id, const std::string& group, int fallback = 0);
    static bool setInt(const std::string& id, const std::string& group, int value);
    static float getFloat(const std::string& id, const std::string& group, float fallback = 0.0f);
    static bool setFloat(const std::string& id, const std::string& group, float value);
    static std::vector<std::string> listSettings(const std::string& group = "");
};
