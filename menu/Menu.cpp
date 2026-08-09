#include "Menu.hpp"

#include "../AimAssist.hpp"
#include "../Config.hpp"
#include "../ESP.hpp"
#include "../HUD.hpp"
#include "../Marker.hpp"
#include "../RCS.hpp"
#include "../scripting/LuaManager.hpp"
#include "BackgroundManager.hpp"
#include "Fonts.hpp"
#include "ImageLoader.hpp"
#include "ThemeManager.hpp"

#include <imgui-SFML.h>
#include <SFML/Graphics/Sprite.hpp>

#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace {
    constexpr float kSidebarWidth = 210.0f;
    constexpr float kSidebarInnerWidth = 194.0f;
    constexpr float kWindowWidth = 980.0f;
    constexpr float kWindowHeight = 680.0f;
}

void Menu::setColors() {
    if (!style)
        style = &ImGui::GetStyle();

    style->Colors[ImGuiCol_WindowBg] = *winCol;
    style->Colors[ImGuiCol_Border] = ImColor(0, 0, 0, 0);
    style->Colors[ImGuiCol_Button] = *bgCol;
    style->Colors[ImGuiCol_ButtonActive] = *btnActiveCol;
    style->Colors[ImGuiCol_ButtonHovered] = *btnHoverCol;
    style->Colors[ImGuiCol_FrameBg] = *bgCol;
    style->Colors[ImGuiCol_FrameBgActive] = *frameCol;
    style->Colors[ImGuiCol_FrameBgHovered] = *hoverCol;
    style->Colors[ImGuiCol_Text] = *textCol;
    style->Colors[ImGuiCol_ChildBg] = *childCol;
    style->Colors[ImGuiCol_CheckMark] = *itemActiveCol;
    style->Colors[ImGuiCol_SliderGrab] = *itemCol;
    style->Colors[ImGuiCol_SliderGrabActive] = *itemActiveCol;
    style->Colors[ImGuiCol_Header] = *itemActiveCol;
    style->Colors[ImGuiCol_HeaderHovered] = *itemCol;
    style->Colors[ImGuiCol_HeaderActive] = *itemActiveCol;
    style->Colors[ImGuiCol_ResizeGrip] = *resizeGripCol;
    style->Colors[ImGuiCol_ResizeGripHovered] = *resizeGripHoverCol;
    style->Colors[ImGuiCol_ResizeGripActive] = *itemActiveCol;
    style->Colors[ImGuiCol_SeparatorHovered] = *resizeGripHoverCol;
    style->Colors[ImGuiCol_SeparatorActive] = *itemActiveCol;
    style->Colors[ImGuiCol_TitleBgActive] = *itemActiveCol;
}

void Menu::loadFont() {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    ImFontConfig fontConfig;
    fontConfig.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromMemoryTTF((void*)poppinsFont, sizeof(poppinsFont), 18.0f, &fontConfig);

    static const ImWchar iconRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig iconConfig;
    iconConfig.MergeMode = true;
    iconConfig.PixelSnapH = true;
    iconConfig.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromMemoryTTF((void*)fontAwesome, sizeof(fontAwesome), 18.0f, &iconConfig, iconRanges);

    ImFontConfig bigFontConfig;
    bigFontConfig.FontDataOwnedByAtlas = false;
    bigFont = io.Fonts->AddFontFromMemoryTTF((void*)poppinsFont, sizeof(poppinsFont), 26.0f, &bigFontConfig);

    ImFontConfig bigIconConfig;
    bigIconConfig.MergeMode = true;
    bigIconConfig.PixelSnapH = true;
    bigIconConfig.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromMemoryTTF((void*)fontAwesome, sizeof(fontAwesome), 18.0f, &bigIconConfig, iconRanges);

    ImGui::SFML::UpdateFontTexture();
}

void Menu::loadTheme() {
    loadFont();

    style = &ImGui::GetStyle();
    style->WindowRounding = 0.0f;
    style->ChildRounding = 8.0f;
    style->FrameRounding = 5.0f;
    style->GrabRounding = 5.0f;
    style->PopupRounding = 7.0f;
    style->ScrollbarSize = 10.0f;
    style->WindowPadding = ImVec2(16.0f, 16.0f);
    style->FramePadding = ImVec2(8.0f, 6.0f);
    style->ItemSpacing = ImVec2(8.0f, 8.0f);

    setColors();

    const auto customLogo = std::filesystem::current_path() / "assets" / "icons" / "logo.png";
    if (!ImageLoader::i().loadFromFile("sidebar.logo", customLogo, true, false))
        ImageLoader::i().loadFromMemory("sidebar.logo", logo, sizeof(logo), true, false);

    BackgroundManager::i().initialize();
    ThemeManager::i().applyPreset(ThemeManager::i().currentPreset());
}

void Menu::renderLogo() {
    ImGui::BeginChild("##sidebar-logo", ImVec2(kSidebarInnerWidth, 88.0f), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::SetCursorPosY(14.0f);

    if (sf::Texture* texture = ImageLoader::i().get("sidebar.logo")) {
        const auto size = texture->getSize();
        if (size.x > 0 && size.y > 0) {
            sf::Sprite sprite(*texture);
            const float height = 38.0f;
            const float scale = height / static_cast<float>(size.y);
            sprite.setScale(scale, scale);
            ImGui::Image(sprite);
            ImGui::SameLine(0.0f, 10.0f);
        }
    }

    ImGui::BeginGroup();
    ImGui::PushFont(bigFont);
    ImGui::TextUnformatted("Revival");
    ImGui::PopFont();
    ImGui::TextDisabled("Standalone UI");
    ImGui::EndGroup();
    ImGui::EndChild();
}

void Menu::renderUser() {
    constexpr float height = 82.0f;
    const float remaining = ImGui::GetContentRegionAvail().y;
    if (remaining > height)
        ImGui::Dummy(ImVec2(0.0f, remaining - height - style->ItemSpacing.y));

    ImGui::BeginChild("##sidebar-status", ImVec2(kSidebarInnerWidth, height), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::TextUnformatted("Revival v2.1");
    ImGui::Spacing();
    ImGui::TextDisabled("%.0f FPS", ImGui::GetIO().Framerate);
    ImGui::TextDisabled("Lua: %s", LuaManager::i().isAvailable() ? "ready" : "optional");
    ImGui::EndChild();
}

void Menu::renderPanel() {
    renderLogo();
    ImGui::Spacing();
    renderTabs();
    renderUser();
}

void Menu::renderTabs() {
    ImGui::BeginChild("##sidebar-tabs", ImVec2(kSidebarInnerWidth, 390.0f), true, ImGuiWindowFlags_NoScrollbar);

    static ImGuiTextFilter2 filter;
    filter.Draw2(ICON_FA_SEARCH " Search", 176.0f);
    ImGui::Spacing();

    const std::array<std::string, 6> tabNames = {
        obf(ICON_FA_CROSSHAIRS " LegitBot"),
        obf(ICON_FA_EYE " Visuals"),
        obf(ICON_FA_COG " Misc"),
        obf("<> Scripts"),
        obf("* Themes"),
        obf(ICON_FA_SAVE " Configs")
    };

    const ImVec4 transparent(0.0f, 0.0f, 0.0f, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));

    for (int i = 0; i < static_cast<int>(tabNames.size()); ++i) {
        if (!filter.PassFilter(tabNames[i].c_str()))
            continue;

        const bool selected = state.selectedTab == i;
        ImGui::PushStyleColor(ImGuiCol_Button, selected ? style->Colors[ImGuiCol_ButtonActive] : transparent);
        ImGui::PushStyleColor(ImGuiCol_Text, selected ? style->Colors[ImGuiCol_Text] : *notSelectedTextColor);

        if (ImGui::Button(tabNames[i].c_str(), ImVec2(176.0f, 46.0f)))
            state.selectedTab = i;

        ImGui::PopStyleColor(2);
    }

    ImGui::PopStyleVar(2);
    ImGui::EndChild();
}

void Menu::renderLegit() {
    ImGuiHelper::drawTabHorizontally(
        "##legit-tabs",
        ImVec2(ImGuiHelper::getWidth(), 62.0f),
        { obf("Aim Assist"), obf("Recoil") },
        state.legitSubTab);

    ImGui::Spacing();
    ImGui::BeginChild("##legit-content", ImVec2(0.0f, 0.0f), true);

    if (state.legitSubTab == 0)
        AimAssist::i().renderImGui();
    else
        RCS::i().renderImGui();

    ImGui::EndChild();
}

void Menu::renderVisuals() {
    ImGuiHelper::drawTabHorizontally(
        "##visual-tabs",
        ImVec2(ImGuiHelper::getWidth(), 62.0f),
        { obf("ESP"), obf("Markers") },
        state.visualSubTab);

    ImGui::Spacing();
    ImGui::BeginChild("##visual-content", ImVec2(0.0f, 0.0f), true);

    if (state.visualSubTab == 0)
        ESP::i().renderImGui();
    else
        Marker::i().renderImGui();

    ImGui::EndChild();
}

void Menu::renderMisc() {
    ImGui::BeginChild("##misc-content", ImVec2(0.0f, 0.0f), true);
    HUD::i().renderImGui();
    ImGui::EndChild();
}

void Menu::render() {
    HUD::i().render();
    if (!isGUIVisible)
        return;

    ThemeManager::i().applyLiveStyle();

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(kWindowWidth, kWindowHeight), ImGuiCond_Always);
    ImGui::Begin(
        "##revival-menu",
        nullptr,
        ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoBringToFrontOnFocus);

    const ImVec2 windowPos = ImGui::GetWindowPos();
    const ImVec2 windowSize = ImGui::GetWindowSize();
    BackgroundManager::i().render(
        windowPos,
        ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y));

    if (ImGui::BeginTable("##main-layout", 2, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("##sidebar", ImGuiTableColumnFlags_WidthFixed, kSidebarWidth);
        ImGui::TableSetupColumn("##content", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        renderPanel();

        ImGui::TableSetColumnIndex(1);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.0f);
        switch (state.selectedTab) {
        case 0:
            renderLegit();
            break;
        case 1:
            renderVisuals();
            break;
        case 2:
            renderMisc();
            break;
        case 3:
            LuaManager::i().renderMenu();
            break;
        case 4:
            ThemeManager::i().renderMenu();
            break;
        case 5:
            Config::i().renderImGui();
            break;
        default:
            state.selectedTab = 0;
            break;
        }

        ImGui::EndTable();
    }

    ImGui::End();
}
