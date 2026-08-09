#include "Lua_Scripts_Manager.hpp"

#include "Lua_Commands.hpp"
#include "Lua_run_time.hpp"
#include "../menu/Console.hpp"
#include "imgui.h"

#include <Windows.h>
#include <shellapi.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <string>
#include <unordered_set>
#include <vector>

struct LuaScriptsManager::Impl {
    LuaRuntime* runtime = nullptr;
    std::filesystem::path scriptsPath;
    bool initialized = false;
    int selectedScript = -1;
    std::array<char, 256> commandBuffer{};
    std::string commandOutput;

#if defined(MENU_ENABLE_LUA) && MENU_ENABLE_LUA
    struct ScriptEntry {
        std::filesystem::path path;
        std::string name;
        bool loaded = false;
        bool autoRun = false;
        std::string lastError;
        std::unique_ptr<sol::environment> environment;
    };

    std::vector<ScriptEntry> scripts;
    std::unordered_set<std::string> autoRunNames;

    std::filesystem::path autoRunFile() const {
        return scriptsPath / "autoload.txt";
    }

    void logLuaError(const ScriptEntry& script, const std::string& error) {
        Console::i().logError("Lua [" + script.name + "]: " + error);
    }

    ScriptEntry* findScript(const std::string& name) {
        auto it = std::find_if(scripts.begin(), scripts.end(), [&](const ScriptEntry& script) {
            return script.name == name;
        });
        return it == scripts.end() ? nullptr : &(*it);
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
        if (!runtime || !runtime->isAvailable())
            return false;

        unloadScript(script);
        script.lastError.clear();

        sol::state& lua = runtime->state();
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

LuaScriptsManager::LuaScriptsManager() : impl_(std::make_unique<Impl>()) {}
LuaScriptsManager::~LuaScriptsManager() = default;

void LuaScriptsManager::initialize(LuaRuntime& runtime, const std::filesystem::path& scriptsPath) {
    if (!impl_ || impl_->initialized)
        return;

    impl_->runtime = &runtime;
    impl_->scriptsPath = scriptsPath;
    std::error_code error;
    std::filesystem::create_directories(impl_->scriptsPath, error);

#if defined(MENU_ENABLE_LUA) && MENU_ENABLE_LUA
    impl_->loadAutoRunList();
    impl_->scanScripts();
    for (auto& script : impl_->scripts) {
        if (script.autoRun)
            impl_->loadScript(script);
    }
#endif

    impl_->initialized = true;
}

void LuaScriptsManager::shutdown() {
    if (!impl_ || !impl_->initialized)
        return;

#if defined(MENU_ENABLE_LUA) && MENU_ENABLE_LUA
    for (auto& script : impl_->scripts)
        impl_->unloadScript(script);
    impl_->scripts.clear();
#endif

    impl_->runtime = nullptr;
    impl_->initialized = false;
    impl_->selectedScript = -1;
}

void LuaScriptsManager::update(float deltaSeconds) {
    if (!impl_ || !impl_->initialized)
        return;

#if defined(MENU_ENABLE_LUA) && MENU_ENABLE_LUA
    for (auto& script : impl_->scripts) {
        if (script.loaded)
            impl_->callUpdate(script, deltaSeconds);
    }
#else
    (void)deltaSeconds;
#endif
}

void LuaScriptsManager::refresh() {
    if (!impl_ || !impl_->initialized)
        return;
#if defined(MENU_ENABLE_LUA) && MENU_ENABLE_LUA
    impl_->scanScripts();
#endif
}

bool LuaScriptsManager::isInitialized() const {
    return impl_ && impl_->initialized;
}

int LuaScriptsManager::scriptCount() const {
#if defined(MENU_ENABLE_LUA) && MENU_ENABLE_LUA
    return impl_ ? static_cast<int>(impl_->scripts.size()) : 0;
#else
    return 0;
#endif
}

int LuaScriptsManager::loadedCount() const {
#if defined(MENU_ENABLE_LUA) && MENU_ENABLE_LUA
    if (!impl_)
        return 0;
    return static_cast<int>(std::count_if(impl_->scripts.begin(), impl_->scripts.end(), [](const Impl::ScriptEntry& script) {
        return script.loaded;
    }));
#else
    return 0;
#endif
}

void LuaScriptsManager::renderMenu() {
    if (!impl_ || !impl_->initialized)
        return;

    ImGui::Text("Scripts: %d   Loaded: %d", scriptCount(), loadedCount());

#if defined(MENU_ENABLE_LUA) && MENU_ENABLE_LUA
    if (ImGui::Button("Refresh scripts"))
        impl_->scanScripts();
    ImGui::SameLine();
    if (ImGui::Button("Reload loaded")) {
        for (auto& script : impl_->scripts) {
            if (script.loaded)
                impl_->loadScript(script);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Open scripts folder")) {
        const std::string folder = impl_->scriptsPath.string();
        ShellExecuteA(nullptr, "open", folder.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::BeginChild("##revival-script-list", ImVec2(210.0f, 0.0f), true);
    for (int i = 0; i < static_cast<int>(impl_->scripts.size()); ++i) {
        auto& script = impl_->scripts[i];
        const std::string label = script.loaded ? "[on] " + script.name : "[off] " + script.name;
        if (ImGui::Selectable(label.c_str(), impl_->selectedScript == i))
            impl_->selectedScript = i;
    }
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("##revival-script-details", ImVec2(0.0f, 0.0f), true);

    if (impl_->selectedScript >= 0 && impl_->selectedScript < static_cast<int>(impl_->scripts.size())) {
        auto& script = impl_->scripts[impl_->selectedScript];
        ImGui::TextUnformatted(script.name.c_str());
        ImGui::TextDisabled("%s", script.path.string().c_str());
        ImGui::Spacing();

        bool autoRun = script.autoRun;
        if (ImGui::Checkbox("Auto-run on startup", &autoRun)) {
            script.autoRun = autoRun;
            impl_->saveAutoRunList();
        }

        if (!script.loaded) {
            if (ImGui::Button("Load"))
                impl_->loadScript(script);
        }
        else {
            if (ImGui::Button("Reload"))
                impl_->loadScript(script);
            ImGui::SameLine();
            if (ImGui::Button("Unload"))
                impl_->unloadScript(script);
        }

        if (!script.lastError.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "Last error");
            ImGui::TextWrapped("%s", script.lastError.c_str());
        }

        if (script.loaded) {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextDisabled("Script UI");
            impl_->callNoArgs(script, "on_render");
        }
    }
    else {
        ImGui::TextDisabled("Select a .lua script from the list.");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextUnformatted("Lua command console");
    ImGui::SetNextItemWidth(-90.0f);
    const bool enterPressed = ImGui::InputText(
        "##lua-command",
        impl_->commandBuffer.data(),
        impl_->commandBuffer.size(),
        ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    const bool runPressed = ImGui::Button("Run", ImVec2(72.0f, 0.0f));

    if (enterPressed || runPressed) {
        std::string output;
        LuaCommands::execute(impl_->commandBuffer.data(), output);
        impl_->commandOutput = output;
        impl_->commandBuffer.fill('\0');
    }

    if (!impl_->commandOutput.empty())
        ImGui::TextWrapped("%s", impl_->commandOutput.c_str());

    ImGui::EndChild();
#else
    ImGui::TextWrapped("Lua support is disabled for this build.");
#endif
}
