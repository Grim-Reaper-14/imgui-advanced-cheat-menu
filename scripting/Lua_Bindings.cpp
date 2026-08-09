#include "Lua_Bindings.hpp"

#include "Lua_Commands.hpp"
#include "Lua_Module.hpp"
#include "Lua_binding_library.hpp"
#include "Lua_run_time.hpp"
#include "../menu/Console.hpp"
#include "imgui.h"

#include <Windows.h>
#include <shellapi.h>

#include <chrono>
#include <cstdint>
#include <string>
#include <tuple>

namespace {
    constexpr const char* kLuaApiVersion = "1.1.0";
    constexpr const char* kRevivalVersion = "2.1";
}

void LuaBindings::bind(LuaRuntime& runtime, const std::filesystem::path& scriptsPath) {
#if defined(MENU_ENABLE_LUA) && MENU_ENABLE_LUA
    sol::state& lua = runtime.state();

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
    app.set_function("scripts_path", [scriptsPath]() { return scriptsPath.string(); });
    app.set_function("open_scripts_folder", [scriptsPath]() {
        const std::string folder = scriptsPath.string();
        return reinterpret_cast<std::intptr_t>(
                   ShellExecuteA(nullptr, "open", folder.c_str(), nullptr, nullptr, SW_SHOWNORMAL)) > 32;
    });
    revival["app"] = app;

    auto modules = lua.create_table();
    modules.set_function("exists", &LuaModule::exists);
    modules.set_function("is_enabled", &LuaModule::isEnabled);
    modules.set_function("set_enabled", &LuaModule::setEnabled);
    modules.set_function("toggle", &LuaModule::toggle);
    modules.set_function("get_key", &LuaModule::getKey);
    modules.set_function("set_key", &LuaModule::setKey);
    modules.set_function("list", [&lua]() {
        sol::table result = lua.create_table();
        int index = 1;
        for (const auto& name : LuaModule::list())
            result[index++] = name;
        return result;
    });
    revival["modules"] = modules;

    auto settings = lua.create_table();
    settings.set_function("exists", [](const std::string& id, const std::string& group) {
        return LuaBindingLibrary::settingExists(id, group);
    });
    settings.set_function("get_bool", [](const std::string& id, const std::string& group) {
        return LuaBindingLibrary::getBool(id, group, false);
    });
    settings.set_function("set_bool", &LuaBindingLibrary::setBool);
    settings.set_function("get_int", [](const std::string& id, const std::string& group) {
        return LuaBindingLibrary::getInt(id, group, 0);
    });
    settings.set_function("set_int", &LuaBindingLibrary::setInt);
    settings.set_function("get_float", [](const std::string& id, const std::string& group) {
        return LuaBindingLibrary::getFloat(id, group, 0.0f);
    });
    settings.set_function("set_float", &LuaBindingLibrary::setFloat);
    settings.set_function("list", [&lua](const std::string& group) {
        sol::table result = lua.create_table();
        int index = 1;
        for (const auto& id : LuaBindingLibrary::listSettings(group))
            result[index++] = id;
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

    auto commands = lua.create_table();
    commands.set_function("run", [](const std::string& line) {
        std::string output;
        const bool ok = LuaCommands::execute(line, output);
        return std::make_tuple(ok, output);
    });
    commands.set_function("list", [&lua]() {
        sol::table result = lua.create_table();
        int index = 1;
        for (const auto& command : LuaCommands::list())
            result[index++] = command;
        return result;
    });
    revival["commands"] = commands;

    auto time = lua.create_table();
    time.set_function("seconds", []() { return ImGui::GetTime(); });
    time.set_function("unix_ms", []() {
        const auto now = std::chrono::system_clock::now().time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    });
    revival["time"] = time;

    auto events = lua.create_table();
    events.set_function("supported", [&lua]() {
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

    // Backwards compatibility with scripts from the original v1 API.
    lua["menu"] = app;
    lua["modules"] = modules;
    lua["ui"] = ui;
#else
    (void)runtime;
    (void)scriptsPath;
#endif
}
