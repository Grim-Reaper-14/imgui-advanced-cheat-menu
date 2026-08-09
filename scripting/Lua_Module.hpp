#pragma once

#include <string>
#include <vector>

class LuaModule {
public:
    static bool exists(const std::string& name);
    static bool isEnabled(const std::string& name);
    static bool setEnabled(const std::string& name, bool enabled);
    static bool toggle(const std::string& name);
    static int getKey(const std::string& name);
    static bool setKey(const std::string& name, int key);
    static std::vector<std::string> list();
};
