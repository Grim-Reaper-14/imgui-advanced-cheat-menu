#include "LuaManager.hpp"

#include "Lua_Bindings.hpp"
#include "Lua_Scripts_Manager.hpp"
#include "Lua_run_time.hpp"
#include "../menu/Console.hpp"
#include "imgui.h"

#include <filesystem>
#include <memory>
#include <string>

struct LuaManager::Impl {
    LuaRuntime runtime;
    LuaScriptsManager scripts;
    std::filesystem::path scriptsPath;
    bool initialized = false;
};

LuaManager::LuaManager() : impl(std::make_unique<Impl>()) {}
LuaManager::~LuaManager() = default;

bool LuaManager::isAvailable() const {
    return impl && impl->runtime.isAvailable();
}

void LuaManager::initialize() {
    if (!impl || impl->initialized)
        return;

    impl->scriptsPath = std::filesystem::current_path() / "scripts";
    std::error_code error;
    std::filesystem::create_directories(impl->scriptsPath, error);

    if (impl->runtime.initialize()) {
        LuaBindings::bind(impl->runtime, impl->scriptsPath);
        impl->scripts.initialize(impl->runtime, impl->scriptsPath);
    }

    Console::i().logInfo(impl->runtime.status());
    impl->initialized = true;
}

void LuaManager::shutdown() {
    if (!impl || !impl->initialized)
        return;

    impl->scripts.shutdown();
    impl->runtime.shutdown();
    impl->initialized = false;
}

void LuaManager::update(float deltaSeconds) {
    if (impl && impl->initialized)
        impl->scripts.update(deltaSeconds);
}

void LuaManager::renderMenu() {
    if (!impl)
        return;
    if (!impl->initialized)
        initialize();

    ImGui::TextUnformatted("Revival Lua scripting");
    ImGui::SameLine();
    ImGui::TextDisabled(isAvailable() ? "API 1.1" : "disabled");
    ImGui::TextDisabled("%s", impl->runtime.status().c_str());
    ImGui::Spacing();

    if (isAvailable())
        impl->scripts.renderMenu();
}
