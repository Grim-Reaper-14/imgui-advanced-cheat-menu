#include "HUD.hpp"

#include "ModuleManager.hpp"
#include "SetManager.hpp"
#include "menu/Menu.hpp"
#include "menu/imgui_custom.hpp"
#include "menu/imgui_helper.hpp"
#include "util/ColorH.hpp"

#include <algorithm>
#include <string>
#include <vector>

HUD::HUD() : Module(obf("HUD"), obf("Renders overlay elements such as the module list")) {
    colML = &SetManager::i().add(new Set(ImGuiHelper::rgbaToVec4(255, 100, 100, 255), obf("colML"), getName())).getVec4();
    colML_Bg = &SetManager::i().add(new Set(ImGuiHelper::rgbaToVec4(0, 0, 0, 100), obf("colML_Bg"), getName())).getVec4();

    speedML = &SetManager::i().add(new Set(0.4f, 0.1f, 1.0f, obf("speedML"), getName())).getVec3f();
    offsetML = &SetManager::i().add(new Set(0.06f, 0.0f, 0.4f, obf("offsetML"), getName())).getVec3f();
    rangeML = &SetManager::i().add(new Set(0.02f, 0.0f, 0.1f, obf("rangeML"), getName())).getVec3f();

    isML = &SetManager::i().add(new Set(false, obf("isML"), getName())).getBVal();
    isMLRainbow = &SetManager::i().add(new Set(true, obf("isMLRainbow"), getName())).getBVal();

    alignML = &SetManager::i().add(new Set(0, obf("alignML"), getName())).getIVal();
    sortML = &SetManager::i().add(new Set(1, obf("sortML"), getName())).getIVal();
}

void HUD::renderImGui() {
    ImGui::Checkbox_(obf("Show HUD").c_str(), &isToggled());
    ImGui::Checkbox_(obf("Show Module List").c_str(), isML);

    ImGui::PushItemWidth(static_cast<float>(Menu::elementSize));
    if (ImGui::BeginCombo(obf("Module List Style").c_str(), obf("Style").c_str())) {
        ImGui::ColorEdit4(obf("Module List Color##1").c_str(), reinterpret_cast<float*>(colML), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
        ImGui::Checkbox_(obf("Rainbow Color").c_str(), isMLRainbow);
        ImGui::SliderFloat_(obf("Rainbow Speed").c_str(), &speedML->x, speedML->y, speedML->z);
        ImGui::SliderFloat_(obf("Rainbow Offset").c_str(), &offsetML->x, offsetML->y, offsetML->z);
        ImGui::SliderFloat_(obf("Rainbow Range").c_str(), &rangeML->x, rangeML->y, rangeML->z);
        ImGui::ColorEdit4(obf("Background Color##1").c_str(), reinterpret_cast<float*>(colML_Bg), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
        ImGuiHelper::renderCombo(obf("Sort"), { obf("ASC"), obf("DESC") }, *sortML, static_cast<float>(Menu::elementSize));
        ImGuiHelper::renderCombo(obf("Align"), { obf("Left"), obf("Right"), obf("Middle") }, *alignML, static_cast<float>(Menu::elementSize));
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
}

void HUD::renderML() {
    if (!*isML)
        return;

    const ImVec2 windowPadding = ImGui::GetStyle().WindowPadding;
    std::vector<std::string*> modules;
    float calculatedHeight = windowPadding.y * 2.0f;
    float longestString = 0.0f;

    for (Module* module : ModuleManager::i().modules) {
        if (!module || !module->isToggled())
            continue;

        const ImVec2 textSize = ImGui::CalcTextSize(module->getName().c_str());
        longestString = std::max(longestString, textSize.x);
        calculatedHeight += textSize.y;
        modules.push_back(&module->getName());
    }

    if (modules.empty())
        return;

    std::sort(modules.begin(), modules.end(), [&](const std::string* a, const std::string* b) {
        const float widthA = ImGui::CalcTextSize(a->c_str()).x;
        const float widthB = ImGui::CalcTextSize(b->c_str()).x;
        return *sortML == 0 ? widthA < widthB : widthA > widthB;
    });

    calculatedHeight += ImGui::GetStyle().ItemSpacing.y * static_cast<float>(modules.size() - 1);

    ImGui::SetNextWindowSize(ImVec2(longestString + windowPadding.x * 2.0f, calculatedHeight));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, *colML_Bg);
    ImGui::Begin(
        "##module-list",
        nullptr,
        ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar);
    ImGui::PopStyleColor();

    for (int i = 0; i < static_cast<int>(modules.size()); ++i) {
        std::string* text = modules[i];
        const float width = ImGui::CalcTextSize(text->c_str()).x;

        if (*alignML == 1)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + longestString - width);
        else if (*alignML == 2)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (longestString - width) * 0.5f);

        if (*isMLRainbow) {
            float h = 0.0f;
            float s = 0.0f;
            float v = 0.0f;
            ColorH::RGBtoHSV(colML->x, colML->y, colML->z, h, s, v);
            ImGui::chromaText(*text, s, v, colML->w, i * (offsetML->x + 1.0f), speedML->x, rangeML->x);
        }
        else {
            ImGui::PushStyleColor(ImGuiCol_Text, *colML);
            ImGui::TextUnformatted(text->c_str());
            ImGui::PopStyleColor();
        }
    }

    ImGui::End();
}

void HUD::render() {
    if (isToggled())
        renderML();
}
