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
    constexpr float kSidebarWidth = 220.0f;
    constexpr float kSidebarInnerWidth = 204.0f;
    constexpr float kWindowWidth = 1080.0f;
    constexpr float kWindowHeight = 800.0f;
    constexpr float kHeaderHeight = 244.0f;
    constexpr float kNavWidth = 186.0f;
    constexpr float kNavHeight = 46.0f;

    constexpr const char* kScriptFallbackIcon = "\xEF\x84\xA1"; // Font Awesome code
    constexpr const char* kThemeFallbackIcon = "\xEF\x94\xBF";  // Font Awesome palette

    std::filesystem::path resolveAsset(const char* fileName) {
        std::error_code ec;
        std::filesystem::path base = std::filesystem::current_path(ec);
        if (ec)
            return {};

        for (int depth = 0; depth < 5; ++depth) {
            const auto candidate = base / "assets" / "icons" / fileName;
            if (std::filesystem::exists(candidate, ec) && !ec)
                return candidate;

            if (!base.has_parent_path())
                break;
            base = base.parent_path();
        }

        return {};
    }

    void loadUiAsset(const char* id, const char* fileName) {
        const auto path = resolveAsset(fileName);
        if (!path.empty())
            ImageLoader::i().loadFromFile(id, path, true, false);
    }

    void renderImageNavItem(int index, const char* label, const char* imageId, const char* fallbackIcon) {
        const bool selected = Menu::state.selectedTab == index;
        const ImVec4 transparent(0.0f, 0.0f, 0.0f, 0.0f);

        const ImVec2 start = ImGui::GetCursorScreenPos();
        const std::string id = "##nav-" + std::to_string(index);

        ImGui::PushStyleColor(ImGuiCol_Header, selected ? Menu::style->Colors[ImGuiCol_ButtonActive] : transparent);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Menu::style->Colors[ImGuiCol_ButtonHovered]);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, Menu::style->Colors[ImGuiCol_ButtonActive]);
        if (ImGui::Selectable(id.c_str(), selected, 0, ImVec2(kNavWidth, kNavHeight)))
            Menu::state.selectedTab = index;
        ImGui::PopStyleColor(3);

        const ImVec2 after = ImGui::GetCursorScreenPos();
        if (sf::Texture* texture = ImageLoader::i().get(imageId)) {
            ImGui::SetCursorScreenPos(ImVec2(start.x + 8.0f, start.y + 7.0f));
            ImGui::Image(*texture, sf::Vector2f(32.0f, 32.0f));
        }
        else {
            ImGui::SetCursorScreenPos(ImVec2(start.x + 11.0f, start.y + 13.0f));
            ImGui::TextUnformatted(fallbackIcon);
        }

        ImGui::SetCursorScreenPos(ImVec2(start.x + 50.0f, start.y + 13.0f));
        ImGui::TextUnformatted(label);
        ImGui::SetCursorScreenPos(after);
    }
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

    loadUiAsset("revival.header", "revival_header.jpg");
    loadUiAsset("revival.scripts", "scripts_icon.jpg");
    loadUiAsset("revival.themes", "themes_icon.jpg");

    BackgroundManager::i().initialize();
    ThemeManager::i().applyPreset(ThemeManager::i().currentPreset());
}

void Menu::renderLogo() {
    ImGui::BeginChild(
        "##revival-brand-header",
        ImVec2(0.0f, kHeaderHeight),
        true,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoInputs);

    if (sf::Texture* texture = ImageLoader::i().get("revival.header")) {
        const ImVec2 available = ImGui::GetContentRegionAvail();
        const auto native = texture->getSize();
        if (native.x > 0 && native.y > 0) {
            const float aspect = static_cast<float>(native.x) / static_cast<float>(native.y);
            float width = available.x;
            float height = width / aspect;
            if (height > available.y) {
                height = available.y;
                width = height * aspect;
            }

            const float offsetX = (available.x - width) * 0.5f;
            const float offsetY = (available.y - height) * 0.5f;
            ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + offsetX, ImGui::GetCursorPosY() + offsetY));
            ImGui::Image(*texture, sf::Vector2f(width, height));
        }
    }
    else {
        ImGui::SetCursorPosY(72.0f);
        ImGui::PushFont(bigFont);
        ImGui::TextUnformatted("REVIVAL V2");
        ImGui::PopFont();
        ImGui::TextDisabled("Reaper Build");
    }

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
    ImGui::TextDisabled("Lua: %s", LuaManager::i().isAvailable() ? "Sol2 ready" : "initializing");
    ImGui::EndChild();
}

void Menu::renderPanel() {
    renderTabs();
    renderUser();
}

void Menu::renderTabs() {
    ImGui::BeginChild("##sidebar-tabs", ImVec2(kSidebarInnerWidth, 374.0f), true, ImGuiWindowFlags_NoScrollbar);

    static ImGuiTextFilter2 filter;
    filter.Draw2(ICON_FA_SEARCH " Search", kNavWidth);
    ImGui::Spacing();

    const std::array<std::string, 6> tabNames = {
        obf(ICON_FA_CROSSHAIRS " LegitBot"),
        obf(ICON_FA_EYE " Visuals"),
        obf(ICON_FA_COG " Misc"),
        "Scripts",
        "Themes",
        obf(ICON_FA_SAVE " Configs")
    };

    const ImVec4 transparent(0.0f, 0.0f, 0.0f, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));

    for (int i = 0; i < static_cast<int>(tabNames.size()); ++i) {
        if (!filter.PassFilter(tabNames[i].c_str()))
            continue;

        if (i == 3) {
            renderImageNavItem(i, "Scripts", "revival.scripts", kScriptFallbackIcon);
            continue;
        }
        if (i == 4) {
            renderImageNavItem(i, "Themes", "revival.themes", kThemeFallbackIcon);
            continue;
        }

        const bool selected = state.selectedTab == i;
        ImGui::PushStyleColor(ImGuiCol_Button, selected ? style->Colors[ImGuiCol_ButtonActive] : transparent);
        ImGui::PushStyleColor(ImGuiCol_Text, selected ? style->Colors[ImGuiCol_Text] : *notSelectedTextColor);

        if (ImGui::Button(tabNames[i].c_str(), ImVec2(kNavWidth, kNavHeight)))
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

    renderLogo();
    ImGui::Spacing();

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
