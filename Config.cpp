#include "Config.hpp"

#include "ModuleManager.hpp"
#include "SetManager.hpp"
#include "menu/Menu.hpp"
#include "menu/imgui_helper.hpp"
#include "util/FileH.hpp"
#include "util/StringH.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {
    std::string sanitizeConfigName(std::string name) {
        const std::string invalid = "<>:\"/\\|?*";
        name.erase(
            std::remove_if(name.begin(), name.end(), [&](unsigned char ch) {
                return ch < 32 || invalid.find(static_cast<char>(ch)) != std::string::npos;
            }),
            name.end());

        while (!name.empty() && (name.back() == ' ' || name.back() == '.'))
            name.pop_back();
        while (!name.empty() && name.front() == ' ')
            name.erase(name.begin());

        if (name == "." || name == "..")
            name.clear();

        return name;
    }

    std::string configPath(const std::string& name) {
        return FileH::getProjCfgPath() + "\\" + name + ".ini";
    }

    bool hasFields(const std::vector<std::string>& values, std::size_t count) {
        return values.size() >= count;
    }
}

Config::Config() {
    checkCfgs();
}

bool Config::save() {
    return save(FileH::getProjCfgPath() + obf("\\DefaultCfg.ini"));
}

bool Config::save(const std::string& filePath) {
    std::ofstream output(filePath, std::ios::trunc);
    if (!output.is_open())
        return false;

    const std::string configName = obf("CFG:") + StringH::getFileNameFromPath(filePath);
    output << StringH::strToBytes(Obf::xor_(configName)) << '\n';

    for (Module* module : ModuleManager::i().modules) {
        if (!module)
            continue;

        std::string line = obf("MOD:") + module->getName() + ":" +
            std::to_string(module->isToggled()) + ":" +
            std::to_string(module->getKey());
        output << StringH::strToBytes(Obf::xor_(line)) << '\n';
    }

    for (Set* setting : SetManager::i().settings) {
        if (!setting)
            continue;

        const bool hasSecondaryId = !setting->ID2.empty();
        std::string line = std::to_string(hasSecondaryId) + ":" + setting->ID + ":" +
            (hasSecondaryId ? setting->ID2 + ":" : "");
        line = obf("SET:") + line;

        if (setting->isBool())
            line += std::to_string(setting->getBVal());
        else if (setting->isInt())
            line += std::to_string(setting->getIVal());
        else if (setting->isFloat())
            line += std::to_string(setting->getFVal());
        else if (setting->isDouble())
            line += std::to_string(setting->getDVal());
        else if (setting->isISlider())
            line += std::to_string(setting->getVec3i().x) + ":" + std::to_string(setting->getVec3i().y) + ":" + std::to_string(setting->getVec3i().z);
        else if (setting->isFSlider())
            line += std::to_string(setting->getVec3f().x) + ":" + std::to_string(setting->getVec3f().y) + ":" + std::to_string(setting->getVec3f().z);
        else if (setting->isDSlider())
            line += std::to_string(setting->getVec3d().x) + ":" + std::to_string(setting->getVec3d().y) + ":" + std::to_string(setting->getVec3d().z);
        else if (setting->isVec4())
            line += std::to_string(setting->getVec4().x) + ":" + std::to_string(setting->getVec4().y) + ":" + std::to_string(setting->getVec4().z) + ":" + std::to_string(setting->getVec4().w);
        else
            continue;

        output << StringH::strToBytes(Obf::xor_(line)) << '\n';
    }

    return true;
}

bool Config::load() {
    return load(FileH::getProjCfgPath() + obf("\\DefaultCfg.ini"));
}

bool Config::load(const std::string& filePath) {
    std::ifstream input(filePath);
    if (!input.is_open())
        return false;

    std::string encodedLine;
    while (std::getline(input, encodedLine)) {
        const std::string decoded = Obf::xor_(StringH::bytesToStr(encodedLine));
        const std::vector<std::string> values = StringH::split(decoded, ":");
        if (values.empty())
            continue;

        if (values[0] == "MOD") {
            if (!hasFields(values, 4))
                continue;

            Module* module = ModuleManager::i().getModuleByName(values[1]);
            if (!module)
                continue;

            module->setToggled(std::atoi(values[2].c_str()));
            const int key = std::atoi(values[3].c_str());
            module->setKey(key, key != 0);
            continue;
        }

        if (values[0] != "SET" || !hasFields(values, 4))
            continue;

        const bool hasSecondaryId = std::atoi(values[1].c_str()) != 0;
        const std::size_t valueIndex = hasSecondaryId ? 4 : 3;
        if (values.size() <= valueIndex)
            continue;

        Set* setting = hasSecondaryId
            ? (values.size() > 3 ? SetManager::i().getSetByName(values[2], values[3]) : nullptr)
            : SetManager::i().getSetByName(values[2]);

        if (!setting)
            continue;

        if (setting->isBool())
            setting->setBVal(std::atoi(values[valueIndex].c_str()) != 0);
        else if (setting->isInt())
            setting->setIVal(std::atoi(values[valueIndex].c_str()));
        else if (setting->isFloat())
            setting->setFVal(static_cast<float>(std::atof(values[valueIndex].c_str())));
        else if (setting->isDouble())
            setting->setDVal(std::atof(values[valueIndex].c_str()));
        else if (setting->isISlider() && values.size() > valueIndex + 2)
            setting->setVec3i(
                std::atoi(values[valueIndex].c_str()),
                std::atoi(values[valueIndex + 1].c_str()),
                std::atoi(values[valueIndex + 2].c_str()));
        else if (setting->isFSlider() && values.size() > valueIndex + 2)
            setting->setVec3f(
                static_cast<float>(std::atof(values[valueIndex].c_str())),
                static_cast<float>(std::atof(values[valueIndex + 1].c_str())),
                static_cast<float>(std::atof(values[valueIndex + 2].c_str())));
        else if (setting->isDSlider() && values.size() > valueIndex + 2)
            setting->setVec3d(
                std::atof(values[valueIndex].c_str()),
                std::atof(values[valueIndex + 1].c_str()),
                std::atof(values[valueIndex + 2].c_str()));
        else if (setting->isVec4() && values.size() > valueIndex + 3)
            setting->setVec4(ImVec4(
                static_cast<float>(std::atof(values[valueIndex].c_str())),
                static_cast<float>(std::atof(values[valueIndex + 1].c_str())),
                static_cast<float>(std::atof(values[valueIndex + 2].c_str())),
                static_cast<float>(std::atof(values[valueIndex + 3].c_str()))));
    }

    Menu::setColors();
    return true;
}

void Config::renderImGui() {
    static char newName[96] = {};
    static char renameName[96] = {};
    static std::string renameSource;
    static std::string deleteSource;
    static bool openRenamePopup = false;
    static bool openDeletePopup = false;

    ImGui::BeginChild("##configs-toolbar", ImVec2(0.0f, 54.0f), true, ImGuiWindowFlags_NoScrollbar);

    ImGui::SetNextItemWidth(220.0f);
    ImGui::InputTextWithHint("##config-name", "Config name", newName, IM_ARRAYSIZE(newName));
    ImGui::SameLine();

    if (ImGui::Button("Create / Save")) {
        const std::string cleanName = sanitizeConfigName(newName);
        if (!cleanName.empty()) {
            save(configPath(cleanName));
            newName[0] = '\0';
            checkCfgs();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Refresh"))
        checkCfgs();

    ImGui::EndChild();
    ImGui::Spacing();

    ImGui::BeginChild("##configs-list", ImVec2(0.0f, 0.0f), true);

    for (int i = 0; i < static_cast<int>(cfgs_.size()); ++i) {
        const std::string& path = cfgs_[i];
        const std::string fileName = StringH::getFileNameFromPath(path);

        ImGui::PushID(i);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, *Menu::childCol1);
        ImGui::BeginChild("##config-row", ImVec2(0.0f, 72.0f), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::PopStyleColor();

        ImGui::TextUnformatted(fileName.c_str());

        if (ImGui::Button("Load"))
            load(path);
        ImGui::SameLine();
        if (ImGui::Button("Overwrite"))
            save(path);
        ImGui::SameLine();
        if (ImGui::Button("Rename")) {
            renameSource = path;
            const std::filesystem::path source(path);
            const std::string stem = source.stem().string();
            std::snprintf(renameName, IM_ARRAYSIZE(renameName), "%s", stem.c_str());
            openRenamePopup = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Duplicate")) {
            const std::filesystem::path source(path);
            std::filesystem::path target = source.parent_path() / (source.stem().string() + "_copy.ini");
            int suffix = 2;
            while (std::filesystem::exists(target)) {
                target = source.parent_path() / (source.stem().string() + "_copy" + std::to_string(suffix++) + ".ini");
            }
            std::error_code error;
            std::filesystem::copy_file(source, target, std::filesystem::copy_options::none, error);
            checkCfgs();
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete")) {
            deleteSource = path;
            openDeletePopup = true;
        }

        ImGui::EndChild();
        ImGui::PopID();
    }

    if (openRenamePopup) {
        ImGui::OpenPopup("Rename config");
        openRenamePopup = false;
    }
    if (openDeletePopup) {
        ImGui::OpenPopup("Delete config");
        openDeletePopup = false;
    }

    if (ImGui::BeginPopupModal("Rename config", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("New name", renameName, IM_ARRAYSIZE(renameName));

        if (ImGui::Button("Rename")) {
            const std::string cleanName = sanitizeConfigName(renameName);
            if (!cleanName.empty() && !renameSource.empty()) {
                const std::filesystem::path source(renameSource);
                const std::filesystem::path target = source.parent_path() / (cleanName + ".ini");
                std::error_code error;
                std::filesystem::rename(source, target, error);
                if (!error)
                    checkCfgs();
            }
            renameSource.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            renameSource.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("Delete config", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("Delete this config?");

        if (ImGui::Button("Delete")) {
            if (!deleteSource.empty())
                FileH::deleteFile(deleteSource);
            deleteSource.clear();
            checkCfgs();
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            deleteSource.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::EndChild();
}

void Config::checkCfgs() {
    cfgs_ = FileH::getFilesInDir(FileH::getProjCfgPath());
    std::sort(cfgs_.begin(), cfgs_.end());
}
