#pragma once

#include <filesystem>

class LuaRuntime;

class LuaBindings {
public:
    static void bind(LuaRuntime& runtime, const std::filesystem::path& scriptsPath);
};
