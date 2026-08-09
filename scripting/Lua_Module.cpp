#include "Lua_Module.hpp"

#include "../ModuleManager.hpp"

bool LuaModule::exists(const std::string& name) {
    return ModuleManager::i().getModuleByName(name) != nullptr;
}

bool LuaModule::isEnabled(const std::string& name) {
    Module* module = ModuleManager::i().getModuleByName(name);
    return module ? module->isToggled() : false;
}

bool LuaModule::setEnabled(const std::string& name, bool enabled) {
    Module* module = ModuleManager::i().getModuleByName(name);
    if (!module)
        return false;

    if (module->isToggled() != enabled)
        module->toggle();
    return true;
}

bool LuaModule::toggle(const std::string& name) {
    Module* module = ModuleManager::i().getModuleByName(name);
    if (!module)
        return false;

    module->toggle();
    return module->isToggled();
}

int LuaModule::getKey(const std::string& name) {
    Module* module = ModuleManager::i().getModuleByName(name);
    return module ? module->getKey() : 0;
}

bool LuaModule::setKey(const std::string& name, int key) {
    Module* module = ModuleManager::i().getModuleByName(name);
    if (!module)
        return false;

    module->setKey(key);
    return true;
}

std::vector<std::string> LuaModule::list() {
    std::vector<std::string> result;
    result.reserve(ModuleManager::i().modules.size());

    for (Module* module : ModuleManager::i().modules) {
        if (module)
            result.push_back(module->getName());
    }

    return result;
}
