#pragma once

#include <memory>
#include <string>

#if defined(MENU_ENABLE_LUA) && MENU_ENABLE_LUA
#include <sol/sol.hpp>
#endif

class LuaRuntime {
public:
    bool initialize();
    void shutdown();

    bool isAvailable() const;
    bool isInitialized() const;
    const std::string& status() const;

#if defined(MENU_ENABLE_LUA) && MENU_ENABLE_LUA
    sol::state& state();
#endif

private:
    bool initialized_ = false;
    bool available_ = false;
    std::string status_ = "Lua runtime not initialized";

#if defined(MENU_ENABLE_LUA) && MENU_ENABLE_LUA
    std::unique_ptr<sol::state> state_;
#endif
};
