#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "LuaManager.hpp"

#include "../ModuleManager.hpp"
#include "../SetManager.hpp"
#include "../menu/Console.hpp"
#include "imgui.h"

#include <Windows.h>
#include <shellapi.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_set>
#include <vector>

#if defined(MENU_ENABLE_LUA) && MENU_ENABLE_LUA
#define MENU_HAS_LUA 1
#define LUA_CORE
#define LUA_LIB
#include <sol/sol.hpp>
extern "C" {
#define MAKE_LIB
#include "../dependencies/lua/onelua.c"
#undef MAKE_LIB
}
#undef LUA_LIB
#undef LUA_CORE
#else
#define MENU_HAS_LUA 0
#endif

namespace {
    constexpr const char* kLuaApiVersion = "1.0.0";
    constexpr const char* kRevivalVersion = "2.1";
}

struct LuaManager::Impl {
    std::filesystem::path scriptsPath;
    bool initialized = false;
    bool available = MENU_HAS_LUA != 0;
    std::string status;

#if MENU_HAS_LUA
    struct ScriptEntry {
        std::filesystem::path path;
        std::string name;
        bool loaded = false;
        bool autoRun = false;
        std::string lastError;
        std::unique_ptr<sol::environment> environment;
    };

    sol::state lua;
    std::vector<ScriptEntry> scripts;
    std::unordered_set<std::string> autoRunNames;
    int selectedScript = -1;

    std::filesystem::path autoRunFile() const { return scriptsPath / "autoload.txt"; }

    void logLuaError(const ScriptEntry& script, const std::string& error) {
        Console::i().logError("Lua [" + script.name + "]: " + error);
    }

    void loadAutoRunList() {
        autoRunNames.clear();
        std::ifstream input(autoRunFile());
        std::string line;
        while (std::getline(input, line)) {
            if (!line.empty())
                autoRunNames.insert(line);
        }
    }

    void saveAutoRunList() {
        std::ofstream output(autoRunFile(), std::ios::trunc);
        for (const auto& script : scripts) {
            if (script.autoRun)
                output << script.name << '\n';
        }
    }

    ScriptEntry* findScript(const std::string& name) {
        auto it = std::find_if(scripts.begin(), scripts.end(), [&](const ScriptEntry& script) {
            return script.name == name;
        });
        return it == scripts.end() ? nullptr : &(*it);
    }

    Set* findSetting(const std::string& id, const std::string& group) {
        return SetManager::i().getSetByName(id, group);
    }

    void bindApi() {
        auto revival = lua.create_named_table("revival");
        revival["api_version"] = kLuaApiVersion;
        revival["version"] = kRevivalVersion;

        auto app = lua.create_table();
        app["name"] = "Revival V2";
        app["version"] = kRevivalVersion;
        app["api_version"] = kLuaApiVersion;
        app.set_function("log", [](const std::string& text) { Console::i().log(text); });
        app.set_function("log_info", [](const std::string& text) { Console::i().logInfo(text); });
        app.set_function("log_error", [](const std::string& text) { Console::i().logError(text); });
        app.set_function("fps", []() { return ImGui::GetIO().Framerate; });
        app.set_function("scripts_path", [this]() { return scriptsPath.string(); });
        app.set_function("open_scripts_folder", [this]() {
            const std::string folder = scriptsPath.string();
            return reinterpret_cast<std::intptr_t>(
                       ShellExecuteA(nullptr, "open", folder.c_str(), nullptr, nullptr, SW_SHOWNORMAL)) > 32;
        });
        revival["app"] = app;

        auto modules = lua.create_table();
        modules.set_function("exists", [](const std::string& name) {
            return ModuleManager::i().getModuleByName(name) != nullptr;
        });
        modules.set_function("is_enabled", [](const std::string& name) {
            Module* module = ModuleManager::i().getModuleByName(name);
            return module ? module->isToggled() : false;
        });
        modules.set_function("set_enabled", [](const std::string& name, bool enabled) {
            Module* module = ModuleManager::i().getModuleByName(name);
            if (!module)
                return false;
            if (module->isToggled() != enabled)
                module->toggle();
            return true;
        });
        modules.set_function("toggle", [](const std::string& name) {
            Module* module = ModuleManager::i().getModuleByName(name);
            if (!module)
                return false;
            module->toggle();
            return module->isToggled();
        });
        modules.set_function("get_key", [](const std::string& name) {
            Module* module = ModuleManager::i().getModuleByName(name);
            return module ? module->getKey() : 0;
        });
        modules.set_function("set_key", [](const std::string& name, int key) {
            Module* module = ModuleManager::i().getModuleByName(name);
            if (!module)
                return false;
            module->setKey(key);
            return true;
        });
        modules.set_function("list", [this]() {
            sol::table result = lua.create_table();
            int index = 1;
            for (Module* module : ModuleManager::i().modules) {
                if (module)
                    result[index++] = module->getName();
            }
            return result;
        });
        revival["modules"] = modules;

        auto settings = lua.create_table();
        settings.set_function("exists", [this](const std::string& id, const std::string& group) {
            return findSetting(id, group) != nullptr;
        });
        settings.set_function("get_bool", [this](const std::string& id, const std::string& group) {
            Set* setting = findSetting(id, group);
            return setting && setting->isBool() ? setting->getBVal() : false;
        });
        settings.set_function("set_bool", [this](const std::string& id, const std::string& group, bool value) {
            Set* setting = findSetting(id, group);
            if (!setting || !setting->isBool())
                return false;
            setting->getBVal() = value;
            return true;
        });
        settings.set_function("get_int", [this](const std::string& id, const std::string& group) {
            Set* setting = findSetting(id, group);
            if (!setting)
                return 0;
            if (setting->isInt())
                return setting->getIVal();
            if (setting->isISlider())
                return setting->getVec3i().x;
            return 0;
        });
        settings.set_function("set_int", [this](const std::string& id, const std::string& group, int value) {
            Set* setting = findSetting(id, group);
            if (!setting)
                return false;
            if (setting->isInt()) {
                setting->getIVal() = value;
                return true;
            }
            if (setting->isISlider()) {
                Vec3i& slider = setting->getVec3i();
                slider.x = std::clamp(value, slider.y, slider.z);
                return true;
            }
            return false;
        });
        settings.set_function("get_float", [this](const std::string& id, const std::string& group) {
            Set* setting = findSetting(id, group);
            if (!setting)
                return 0.0f;
            if (setting->isFloat())
                return setting->getFVal();
            if (setting->isFSlider())
                return setting->getVec3f().x;
            return 0.0f;
        });
        settings.set_function("set_float", [this](const std::string& id, const std::string& group, float value) {
            Set* setting = findSetting(id, group);
            if (!setting)
                return false;
            if (setting->isFloat()) {
                setting->getFVal() = value;
                return true;
            }
            if (setting->isFSlider()) {
                Vec3f& slider = setting->getVec3f();
                slider.x = std::clamp(value, slider.y, slider.z);
                return true;
            }
            return false;
        });
        settings.set_function("list", [this](const std::string& group) {
            sol::table result = lua.create_table();
            int index = 1;
            for (Set* setting : SetManager::i().settings) {
                if (!setting)
                    continue;
                if (!group.empty() && setting->ID2 != group)
                    continue;
                result[index++] = setting->ID;
            }
            return result;
        });
        revival["settings"] = settings;

        auto ui = lua.create_table();
        ui.set_function("text", [](const std::string& text) { ImGui::TextUnformatted(text.c_str()); });
        ui.set_function("text_disabled", [](const std::string& text) { ImGui::TextDisabled("%s", text.c_str()); });
        ui.set_function("text_colored", [](const std::string& text, float r, float g, float b, float a) {
            ImGui::TextColored(ImVec4(r, g, b, a), "%s", text.c_str());
        });
        ui.set_function("separator", []() { ImGui::Separator(); });
        ui.set_function("spacing", []() { ImGui::Spacing(); });
        ui.set_function("same_line", []() { ImGui::SameLine(); });
        ui.set_function("button", [](const std::string& label) { return ImGui::Button(label.c_str()); });
        ui.set_function("checkbox", [](const std::string& label, bool value) {
            ImGui::Checkbox(label.c_str(), &value);
            return value;
        });
        ui.set_function("slider_int", [](const std::string& label, int value, int minValue, int maxValue) {
            ImGui::SliderInt(label.c_str(), &value, minValue, maxValue);
            return value;
        });
        ui.set_function("slider_float", [](const std::string& label, float value, float minValue, float maxValue) {
            ImGui::SliderFloat(label.c_str(), &value, minValue, maxValue, "%.2f");
            return value;
        });
        ui.set_function("collapsing_header", [](const std::string& label) {
            return ImGui::CollapsingHeader(label.c_str());
        });
        revival["ui"] = ui;

        auto time = lua.create_table();
        time.set_function("seconds", []() { return ImGui::GetTime(); });
        time.set_function("unix_ms", []() {
            const auto now = std::chrono::system_clock::now().time_since_epoch();
            return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
        });
        revival["time"] = time;

        auto events = lua.create_table();
        events.set_function("supported", [this]() {
            sol::table result = lua.create_table();
            result[1] = "on_load";
            result[2] = "on_update";
            result[3] = "on_render";
            result[4] = "on_unload";
            return result;
        });
        events.set_function("has", [](const std::string& name) {
            return name == "on_load" || name == "on_update" || name == "on_render" || name == "on_unload";
        });
        revival["events"] = events;

        // Compatibility aliases for scripts written against the pre-v1 API.
        lua["menu"] = app;
        lua["modules"] = modules;
        lua["ui"] = ui;
    }

    bool callNoArgs(ScriptEntry& script, const char* callback) {
        if (!script.loaded || !script.environment)
            return false;
        sol::object object = (*script.environment)[callback];
        if (!object.valid() || object.get_type() != sol::type::function)
            return true;
        sol::protected_function function = object.as<sol::protected_function>();
        sol::protected_function_result result = function();
        if (!result.valid()) {
            sol::error error = result;
            script.lastError = error.what();
            logLuaError(script, script.lastError);
            return false;
        }
        return true;
    }

    bool callUpdate(ScriptEntry& script, float deltaSeconds) {
        if (!script.loaded || !script.environment)
            return false;
        sol::object object = (*script.environment)["on_update"];
        if (!object.valid() || object.get_type() != sol::type::function)
            return true;
        sol::protected_function function = object.as<sol::protected_function>();
        sol::protected_function_result result = function(deltaSeconds);
        if (!result.valid()) {
            sol::error error = result;
            script.lastError = error.what();
            logLuaError(script, script.lastError);
            return false;
        }
        return true;
    }

    void unloadScript(ScriptEntry& script) {
        if (script.loaded)
            callNoArgs(script, "on_unload");
        script.environment.reset();
        script.loaded = false;
    }

    bool loadScript(ScriptEntry& script) {
        unloadScript(script);
        script.lastError.clear();

        sol::load_result loadedChunk = lua.load_file(script.path.string());
        if (!loadedChunk.valid()) {
            sol::error error = loadedChunk;
            script.lastError = error.what();
            logLuaError(script, script.lastError);
            return false;
        }

        auto environment = std::make_unique<sol::environment>(lua, sol::create, lua.globals());
        sol::protected_function function = loadedChunk;
        sol::set_environment(*environment, function);
        sol::protected_function_result result = function();
        if (!result.valid()) {
            sol::error error = result;
            script.lastError = error.what();
            logLuaError(script, script.lastError);
            return false;
        }

        script.environment = std::move(environment);
        script.loaded = true;
        callNoArgs(script, "on_load");
        Console::i().logInfo("Loaded Lua script: " + script.name);
        return true;
    }

    void scanScripts() {
        std::error_code error;
        std::filesystem::create_directories(scriptsPath, error);
        std::unordered_set<std::string> onDisk;

        for (const auto& entry : std::filesystem::directory_iterator(scriptsPath, error)) {
            if (error)
                break;
            if (!entry.is_regular_file() || entry.path().extension() != ".lua")
                continue;

            const std::string name = entry.path().filename().string();
            onDisk.insert(name);
            if (findScript(name))
                continue;

            ScriptEntry script;
            script.path = entry.path();
            script.name = name;
            script.autoRun = autoRunNames.find(name) != autoRunNames.end();
            scripts.push_back(std::move(script));
        }

        for (auto it = scripts.begin(); it != scripts.end();) {
            if (onDisk.find(it->name) == onDisk.end()) {
                unloadScript(*it);
                it = scripts.erase(it);
            }
            else {
                ++it;
            }
        }

        std::sort(scripts.begin(), scripts.end(), [](const ScriptEntry& a, const ScriptEntry& b) {
            return a.name < b.name;
        });

        if (selectedScript >= static_cast<int>(scripts.size()))
            selectedScript = scripts.empty() ? -1 : 0;
    }
#endif
};

LuaManager::LuaManager() : impl(std::make_unique<Impl>()) {}
LuaManager::~LuaManager() = default;
bool LuaManager::isAvailable() const { return impl && impl->available; }

void LuaManager::initialize() {
    if (!impl || impl->initialized)
        return;

    impl->scriptsPath = std::filesystem::current_path() / "scripts";
    std::error_code error;
    std::filesystem::create_directories(impl->scriptsPath, error);

#if MENU_HAS_LUA
    impl->lua.open_libraries(
        sol::lib::base,
        sol::lib::math,
        sol::lib::string,
        sol::lib::table,
        sol::lib::coroutine,
        sol::lib::utf8);
    impl->lua["dofile"] = sol::nil;
    impl->lua["loadfile"] = sol::nil;
    impl->lua["require"] = sol::nil;
    impl->bindApi();
    impl->loadAutoRunList();
    impl->scanScripts();
    for (auto& script : impl->scripts) {
        if (script.autoRun)
            impl->loadScript(script);
    }
    impl->status = std::string("Revival Lua API ") + kLuaApiVersion + " ready (Lua 5.4 + Sol2)";
#else
    impl->status = "Lua support is disabled.";
#endif

    Console::i().logInfo(impl->status);
    impl->initialized = true;
}

void LuaManager::shutdown() {
    if (!impl || !impl->initialized)
        return;
#if MENU_HAS_LUA
    for (auto& script : impl->scripts)
        impl->unloadScript(script);
    impl->scripts.clear();
#endif
    impl->initialized = false;
}

void LuaManager::update(float deltaSeconds) {
    if (!impl || !impl->initialized)
        return;
#if MENU_HAS_LUA
    for (auto& script : impl->scripts) {
        if (script.loaded)
            impl->callUpdate(script, deltaSeconds);
    }
#else
    (void)deltaSeconds;
#endif
}

void LuaManager::renderMenu() {
    if (!impl)
        return;
    if (!impl->initialized)
        initialize();

    ImGui::TextUnformatted("Revival Lua API");
    ImGui::SameLine();
    ImGui::TextDisabled(isAvailable() ? kLuaApiVersion : "disabled");
    ImGui::TextDisabled("Namespace: revival.* | Lua 5.4 + Sol2");
    ImGui::Spacing();

#if MENU_HAS_LUA
    if (ImGui::Button("Refresh scripts"))
        impl->scanScripts();
    ImGui::SameLine();
    if (ImGui::Button("Reload all")) {
        for (auto& script : impl->scripts) {
            if (script.loaded)
                impl->loadScript(script);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Open scripts folder")) {
        const std::string folder = impl->scriptsPath.string();
        ShellExecuteA(nullptr, "open", folder.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::BeginChild("##script-list", ImVec2(210.0f, 0.0f), true);
    for (int i = 0; i < static_cast<int>(impl->scripts.size()); ++i) {
        auto& script = impl->scripts[i];
        std::string label = script.loaded ? "[on] " + script.name : "[off] " + script.name;
        if (ImGui::Selectable(label.c_str(), impl->selectedScript == i))
            impl->selectedScript = i;
    }
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("##script-details", ImVec2(0.0f, 0.0f), true);
    if (impl->selectedScript >= 0 && impl->selectedScript < static_cast<int>(impl->scripts.size())) {
        auto& script = impl->scripts[impl->selectedScript];
        ImGui::TextUnformatted(script.name.c_str());
        ImGui::TextDisabled("%s", script.path.string().c_str());
        ImGui::Spacing();

        bool autoRun = script.autoRun;
        if (ImGui::Checkbox("Auto-run on startup", &autoRun)) {
            script.autoRun = autoRun;
            impl->saveAutoRunList();
        }

        if (!script.loaded) {
            if (ImGui::Button("Load"))
                impl->loadScript(script);
        }
        else {
            if (ImGui::Button("Reload"))
                impl->loadScript(script);
            ImGui::SameLine();
            if (ImGui::Button("Unload"))
                impl->unloadScript(script);
        }

        if (!script.lastError.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "Last error:");
            ImGui::TextWrapped("%s", script.lastError.c_str());
        }

        if (script.loaded) {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextDisabled("Script UI");
            impl->callNoArgs(script, "on_render");
        }
    }
    else {
        ImGui::TextDisabled("Select a .lua script from the list.");
    }
    ImGui::EndChild();
#else
    ImGui::TextWrapped("%s", impl->status.c_str());
#endif
}
