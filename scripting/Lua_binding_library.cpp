#include "Lua_binding_library.hpp"

#include "../SetManager.hpp"

#include <algorithm>

namespace {
    Set* findSetting(const std::string& id, const std::string& group) {
        return SetManager::i().getSetByName(id, group);
    }
}

bool LuaBindingLibrary::settingExists(const std::string& id, const std::string& group) {
    return findSetting(id, group) != nullptr;
}

bool LuaBindingLibrary::getBool(const std::string& id, const std::string& group, bool fallback) {
    Set* setting = findSetting(id, group);
    return setting && setting->isBool() ? setting->getBVal() : fallback;
}

bool LuaBindingLibrary::setBool(const std::string& id, const std::string& group, bool value) {
    Set* setting = findSetting(id, group);
    if (!setting || !setting->isBool())
        return false;
    setting->getBVal() = value;
    return true;
}

int LuaBindingLibrary::getInt(const std::string& id, const std::string& group, int fallback) {
    Set* setting = findSetting(id, group);
    if (!setting)
        return fallback;
    if (setting->isInt())
        return setting->getIVal();
    if (setting->isISlider())
        return setting->getVec3i().x;
    return fallback;
}

bool LuaBindingLibrary::setInt(const std::string& id, const std::string& group, int value) {
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
}

float LuaBindingLibrary::getFloat(const std::string& id, const std::string& group, float fallback) {
    Set* setting = findSetting(id, group);
    if (!setting)
        return fallback;
    if (setting->isFloat())
        return setting->getFVal();
    if (setting->isFSlider())
        return setting->getVec3f().x;
    return fallback;
}

bool LuaBindingLibrary::setFloat(const std::string& id, const std::string& group, float value) {
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
}

std::vector<std::string> LuaBindingLibrary::listSettings(const std::string& group) {
    std::vector<std::string> result;
    for (Set* setting : SetManager::i().settings) {
        if (!setting)
            continue;
        if (!group.empty() && setting->ID2 != group)
            continue;
        result.push_back(setting->ID);
    }
    return result;
}
