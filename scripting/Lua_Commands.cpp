#include "Lua_Commands.hpp"

#include "Lua_Module.hpp"
#include "../menu/Console.hpp"

#include <sstream>

namespace {
    bool parseBool(const std::string& value, bool& result) {
        if (value == "1" || value == "true" || value == "on" || value == "enable" || value == "enabled") {
            result = true;
            return true;
        }
        if (value == "0" || value == "false" || value == "off" || value == "disable" || value == "disabled") {
            result = false;
            return true;
        }
        return false;
    }
}

bool LuaCommands::execute(const std::string& commandLine, std::string& output) {
    std::istringstream stream(commandLine);
    std::string command;
    stream >> command;

    if (command.empty()) {
        output = "empty command";
        return false;
    }

    if (command == "help" || command == "commands") {
        output = "help, log <text>, module.toggle <name>, module.set <name> <on|off>, module.key <name> <vk>";
        return true;
    }

    if (command == "log") {
        std::string text;
        std::getline(stream >> std::ws, text);
        Console::i().logInfo("Lua command: " + text);
        output = text;
        return true;
    }

    if (command == "module.toggle") {
        std::string name;
        stream >> name;
        if (name.empty() || !LuaModule::exists(name)) {
            output = "module not found";
            return false;
        }
        const bool enabled = LuaModule::toggle(name);
        output = name + (enabled ? " enabled" : " disabled");
        return true;
    }

    if (command == "module.set") {
        std::string name;
        std::string state;
        stream >> name >> state;
        bool enabled = false;
        if (name.empty() || !LuaModule::exists(name) || !parseBool(state, enabled)) {
            output = "usage: module.set <name> <on|off>";
            return false;
        }
        LuaModule::setEnabled(name, enabled);
        output = name + (enabled ? " enabled" : " disabled");
        return true;
    }

    if (command == "module.key") {
        std::string name;
        int key = 0;
        stream >> name >> key;
        if (name.empty() || !LuaModule::setKey(name, key)) {
            output = "usage: module.key <name> <vk>";
            return false;
        }
        output = "key updated";
        return true;
    }

    output = "unknown command: " + command;
    return false;
}

std::vector<std::string> LuaCommands::list() {
    return {
        "help",
        "log <text>",
        "module.toggle <name>",
        "module.set <name> <on|off>",
        "module.key <name> <vk>"
    };
}
