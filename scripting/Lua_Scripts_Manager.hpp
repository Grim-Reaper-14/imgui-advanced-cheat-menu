#pragma once

#include <filesystem>
#include <memory>

class LuaRuntime;

class LuaScriptsManager {
public:
    LuaScriptsManager();
    ~LuaScriptsManager();

    void initialize(LuaRuntime& runtime, const std::filesystem::path& scriptsPath);
    void shutdown();
    void update(float deltaSeconds);
    void renderMenu();
    void refresh();

    bool isInitialized() const;
    int scriptCount() const;
    int loadedCount() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
