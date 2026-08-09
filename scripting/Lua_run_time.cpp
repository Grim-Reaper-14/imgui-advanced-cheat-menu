#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#if defined(MENU_ENABLE_LUA) && MENU_ENABLE_LUA
#define LUA_CORE
#define LUA_LIB
#endif

#include "Lua_run_time.hpp"

#if defined(MENU_ENABLE_LUA) && MENU_ENABLE_LUA
extern "C" {
#define MAKE_LIB
#include "../dependencies/lua/onelua.c"
#undef MAKE_LIB
}
#undef LUA_LIB
#undef LUA_CORE
#endif

bool LuaRuntime::initialize() {
    if (initialized_)
        return available_;

#if defined(MENU_ENABLE_LUA) && MENU_ENABLE_LUA
    try {
        state_ = std::make_unique<sol::state>();
        state_->open_libraries(
            sol::lib::base,
            sol::lib::math,
            sol::lib::string,
            sol::lib::table,
            sol::lib::coroutine,
            sol::lib::utf8);

        (*state_)["dofile"] = sol::nil;
        (*state_)["loadfile"] = sol::nil;
        (*state_)["require"] = sol::nil;

        available_ = true;
        status_ = "Lua 5.4 + Sol2 runtime ready";
    }
    catch (const std::exception& error) {
        state_.reset();
        available_ = false;
        status_ = std::string("Lua runtime failed: ") + error.what();
    }
#else
    available_ = false;
    status_ = "Lua support is disabled for this build";
#endif

    initialized_ = true;
    return available_;
}

void LuaRuntime::shutdown() {
#if defined(MENU_ENABLE_LUA) && MENU_ENABLE_LUA
    state_.reset();
#endif
    available_ = false;
    initialized_ = false;
    status_ = "Lua runtime stopped";
}

bool LuaRuntime::isAvailable() const {
    return available_;
}

bool LuaRuntime::isInitialized() const {
    return initialized_;
}

const std::string& LuaRuntime::status() const {
    return status_;
}

#if defined(MENU_ENABLE_LUA) && MENU_ENABLE_LUA
sol::state& LuaRuntime::state() {
    return *state_;
}
#endif
