#include "HUD.hpp"

#include "ModuleManager.hpp"
#include "SetManager.hpp"
#include "menu/Menu.hpp"
#include "menu/imgui_custom.hpp"
#include "menu/imgui_helper.hpp"
#include "scripting/LuaManager.hpp"
#include "util/ColorH.hpp"

#include <algorithm>
#include <ctime>
#include <string>
#include <vector>

namespace {
    void beginMiscCard(const char* id, const char* title, float height) {
        ImGui::BeginChild(id, ImVec2(0.0f, height), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.86f, 1.0f, 1.0f));
        ImGui::TextUnformatted(title);
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();
    }

    void endMiscCard() {
        ImGui::EndChild();
    }
}

HUD::HUD() : Module(obf("HUD"), obf("Renders Revival status and module overlays")) {
    colML = &SetManager::i().add(new Set(ImGuiHelper::rgbaToVec4(194, 112, 255, 255), obf("colML"), getName())).getVec4();
    colML_Bg = &SetManager::i().add(new Set(ImGuiHelper::rgbaToVec4(5, 7, 13, 205), obf("colML_Bg"), getName())).getVec4();
    statusAccent = &SetManager::i().add(new Set(ImGuiHelper::rgbaToVec4(170, 67, 255, 255), obf("statusAccent"), getName())).getVec4();
    statusBg = &SetManager::i().add(new Set(ImGuiHelper::rgbaToVec4(8, 10, 18, 232), obf("statusBg"), getName())).getVec4();

    speedML = &SetManager::i().add(new Set(0.4f, 0.1f, 1.0f, obf("speedML"), getName())).getVec3f();
    offsetML = &SetManager::i().add(new Set(0.06f, 0.0f, 0.4f, obf("offsetML"), getName())).getVec3f();
    rangeML = &SetManager::i().add(new Set(0.02f, 0.0f, 0.1f, obf("rangeML"), getName())).getVec3f();
    hudScale = &SetManager::i().add(new Set(1.0f, 0.75f, 1.50f, obf("hudScale"), getName())).getVec3f();
    hudAlpha = &SetManager::i().add(new Set(0.92f, 0.35f, 1.0f, obf("hudAlpha"), getName())).getVec3f();

    isML = &SetManager::i().add(new Set(false, obf("isML"), getName())).getBVal();
    isMLRainbow = &SetManager::i().add(new Set(true, obf("isMLRainbow"), getName())).getBVal();
    showWatermark = &SetManager::i().add(new Set(true, obf("showWatermark"), getName())).getBVal();
    showFps = &SetManager::i().add(new Set(true, obf("showFps"), getName())).getBVal();
    showClock = &SetManager::i().add(new Set(true, obf("showClock"), getName())).getBVal();
    showLuaStatus = &SetManager::i().add(new Set(true, obf("showLuaStatus"), getName())).getBVal();
    showAccentLine = &SetManager::i().add(new Set(true, obf("showAccentLine"), getName())).getBVal();
    compactStatus = &SetManager::i().add(new Set(false, obf("compactStatus"), getName())).getBVal();

    alignML = &SetManager::i().add(new Set(0, obf("alignML"), getName())).getIVal();
    sortML = &SetManager::i().add(new Set(1, obf("sortML"), getName())).getIVal();
    statusCorner = &SetManager::i().add(new Set(1, obf("statusCorner"), getName())).getIVal();
}

void HUD::renderImGui() {
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(6.0f, 6.0f));

    if (ImGui::BeginTable("##misc-layout", 2, ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        beginMiscCard("##misc-status-card", "Status Overlay", 315.0f);
        ImGui::Checkbox_(obf("Enable HUD").c_str(), &isToggled());
        ImGui::Checkbox_(obf("Revival watermark").c_str(), showWatermark);
        ImGui::Checkbox_(obf("FPS counter").c_str(), showFps);
        ImGui::Checkbox_(obf("Clock").c_str(), showClock);
        ImGui::Checkbox_(obf("Lua status").c_str(), showLuaStatus);
        ImGui::Checkbox_(obf("Compact status").c_str(), compactStatus);
        ImGuiHelper::renderCombo(
            obf("Screen corner"),
            { obf("Top Left"), obf("Top Right"), obf("Bottom Left"), obf("Bottom Right") },
            *statusCorner,
            210.0f);
        endMiscCard();

        ImGui::TableSetColumnIndex(1);
        beginMiscCard("##misc-module-card", "Module List", 315.0f);
        ImGui::Checkbox_(obf("Show module list").c_str(), isML);
        ImGui::Checkbox_(obf("Rainbow modules").c_str(), isMLRainbow);
        ImGuiHelper::renderCombo(obf("Sort"), { obf("ASC"), obf("DESC") }, *sortML, 210.0f);
        ImGuiHelper::renderCombo(obf("Align"), { obf("Left"), obf("Right"), obf("Middle") }, *alignML, 210.0f);
        ImGui::ColorEdit4(obf("Module color").c_str(), reinterpret_cast<float*>(colML), ImGuiColorEditFlags_AlphaBar);
        ImGui::ColorEdit4(obf("Module background").c_str(), reinterpret_cast<float*>(colML_Bg), ImGuiColorEditFlags_AlphaBar);
        endMiscCard();

        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        beginMiscCard("##misc-appearance-card", "Appearance", 300.0f);
        ImGui::SetNextItemWidth(210.0f);
        ImGui::SliderFloat("HUD scale", &hudScale->x, hudScale->y, hudScale->z, "%.2fx");
        ImGui::SetNextItemWidth(210.0f);
        ImGui::SliderFloat("HUD opacity", &hudAlpha->x, hudAlpha->y, hudAlpha->z, "%.2f");
        ImGui::Checkbox_(obf("Accent line").c_str(), showAccentLine);
        ImGui::ColorEdit4(obf("Accent color").c_str(), reinterpret_cast<float*>(statusAccent), ImGuiColorEditFlags_AlphaBar);
        ImGui::ColorEdit4(obf("Status background").c_str(), reinterpret_cast<float*>(statusBg), ImGuiColorEditFlags_AlphaBar);
        endMiscCard();

        ImGui::TableSetColumnIndex(1);
        beginMiscCard("##misc-behavior-card", "Module List Motion", 300.0f);
        ImGui::TextDisabled("Rainbow animation controls");
        ImGui::Spacing();
        ImGui::SetNextItemWidth(210.0f);
        ImGui::SliderFloat_(obf("Speed").c_str(), &speedML->x, speedML->y, speedML->z);
        ImGui::SetNextItemWidth(210.0f);
        ImGui::SliderFloat_(obf("Offset").c_str(), &offsetML->x, offsetML->y, offsetML->z);
        ImGui::SetNextItemWidth(210.0f);
        ImGui::SliderFloat_(obf("Range").c_str(), &rangeML->x, rangeML->y, rangeML->z);
        ImGui::Spacing();
        ImGui::TextWrapped("These controls affect the animated module list when Rainbow modules is enabled.");
        endMiscCard();

        ImGui::EndTable();
    }

    ImGui::PopStyleVar();
    ImGui::Dummy(ImVec2(0.0f, 24.0f));
}

void HUD::renderStatus() {
    if (!*showWatermark && !*showFps && !*showClock && !*showLuaStatus)
        return;

    const ImGuiIO& io = ImGui::GetIO();
    constexpr float pad = 14.0f;
    ImVec2 position(pad, pad);
    ImVec2 pivot(0.0f, 0.0f);

    switch (std::clamp(*statusCorner, 0, 3)) {
    case 1:
        position = ImVec2(io.DisplaySize.x - pad, pad);
        pivot = ImVec2(1.0f, 0.0f);
        break;
    case 2:
        position = ImVec2(pad, io.DisplaySize.y - pad);
        pivot = ImVec2(0.0f, 1.0f);
        break;
    case 3:
        position = ImVec2(io.DisplaySize.x - pad, io.DisplaySize.y - pad);
        pivot = ImVec2(1.0f, 1.0f);
        break;
    default:
        break;
    }

    ImGui::SetNextWindowPos(position, ImGuiCond_Always, pivot);
    ImVec4 bg = *statusBg;
    bg.w *= std::clamp(hudAlpha->x, 0.0f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bg);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(statusAccent->x, statusAccent->y, statusAccent->z, 0.45f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(11.0f, 8.0f));

    ImGui::Begin(
        "##revival-status-overlay",
        nullptr,
        ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_NoMove);

    ImGui::SetWindowFontScale(hudScale->x);

    if (*showWatermark) {
        ImGui::TextColored(*statusAccent, "REVIVAL V2");
        if (!*compactStatus)
            ImGui::SameLine();
    }

    if (*showFps) {
        ImGui::Text("%.0f FPS", io.Framerate);
        if (*compactStatus && (*showClock || *showLuaStatus))
            ImGui::SameLine();
    }

    if (*showClock) {
        std::time_t now = std::time(nullptr);
        std::tm local{};
        localtime_s(&local, &now);
        char buffer[16]{};
        std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &local);
        ImGui::TextUnformatted(buffer);
        if (*compactStatus && *showLuaStatus)
            ImGui::SameLine();
    }

    if (*showLuaStatus) {
        const bool luaReady = LuaManager::i().isAvailable();
        ImGui::TextColored(
            luaReady ? ImVec4(0.38f, 0.95f, 0.56f, 1.0f) : ImVec4(1.0f, 0.55f, 0.35f, 1.0f),
            "Lua: %s",
            luaReady ? "READY" : "OFF");
    }

    if (*showAccentLine) {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        const ImVec2 min = ImGui::GetWindowPos();
        const ImVec2 size = ImGui::GetWindowSize();
        draw->AddRectFilled(
            ImVec2(min.x, min.y + size.y - 2.0f),
            ImVec2(min.x + size.x, min.y + size.y),
            ImGui::ColorConvertFloat4ToU32(*statusAccent),
            1.0f);
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
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
    if (!isToggled())
        return;

    renderStatus();
    renderML();
}
