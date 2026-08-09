#pragma once

#include <string>
#include <vector>

class LuaCommands {
public:
    static bool execute(const std::string& commandLine, std::string& output);
    static std::vector<std::string> list();
};
