#include "Menu.hpp"

#include "../AimAssist.hpp"
#include "../Config.hpp"
#include "../ESP.hpp"
#include "../HUD.hpp"
#include "../Marker.hpp"
#include "../RCS.hpp"
#include "../scripting/LuaManager.hpp"
#include "../resources/resource.h"
#include "BackgroundManager.hpp"
#include "Console.hpp"
#include "Fonts.hpp"
#include "ImageLoader.hpp"
#include "ThemeManager.hpp"

#include <imgui-SFML.h>
#include <SFML/Graphics/Sprite.hpp>

#include <Windows.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <string>

namespace {
    constexpr float kSidebarWidth = 236.0f;
    constexpr float kSidebarInnerWidth = 218.0f;
    constexpr float kWindowWidth = 1180.0f;
    constexpr float kWindowHeight = 860.0f;
    constexpr float kHeaderHeight = 218.0f;
    constexpr float kNavWidth = 198.0f;
    constexpr float kNavHeight = 58.0f;
    constexpr float kStatusHeight = 100.0f;

    struct EmbeddedAsset {
        const char* id;
        int resourceId;
    };

    constexpr std::array<EmbeddedAsset, 7> kEmbeddedAssets = {{
        { "revival.header", IDR_REVIVAL_HEADER },
        { "revival.legit", IDR_REVIVAL_LEGIT },
        { "revival.visuals", IDR_REVIVAL_VISUALS },
        { "revival.misc", IDR_REVIVAL_MISC },
        { "revival.scripts", IDR_REVIVAL_SCRIPTS },
        { "revival.themes", IDR_REVIVAL_THEMES },
        { "revival.configs", IDR_REVIVAL_CONFIGS },
    }};

    bool loadEmbeddedAsset(const char* id, int resourceId) {
        HMODULE module = GetModuleHandleW(nullptr);
        if (!module)
            return false;

        HRSRC resource = FindResourceW(module, MAKEINTRESOURCEW(resourceId), RT_RCDATA);
        if (!resource)
            return false;

        HGLOBAL loaded = LoadResource(module, resource);
        if (!loaded)
            return false;

        const DWORD size = SizeofResource(module, resource);
        const void* data = LockResource(loaded);
        if (!data || size == 0)
            return false;

        return ImageLoader::i().loadFromMemory(id, data, static_cast<std::size_t>(size), true, false);
    }

    void loadEmbeddedAssets() {
        for (const auto& asset : kEmbeddedAssets) {
            if (!loadEmbeddedAsset(asset.id, asset.resourceId))
                Console::i().logError(std::string("Failed to load embedded UI asset: ") + asset.id);
        }
    }

    void drawPanelFrame(const ImVec2& min, const ImVec2& max, bool selected = false) {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        const ImU32 fill = selected ? IM_COL32(26, 16, 42, 244) : IM_COL32(10, 13, 23, 235);
        const ImU32 border = selected ? IM_COL32(164, 61, 255, 215) : IM_COL32(67, 53, 91, 150);
        draw->AddRectFilled(min, max, fill, 10.0f);
        draw->AddRect(min, max, border, 10.0f, 0, selected ? 1.7f : 1.0f);
        if (selected) {
            draw->AddRect(
                ImVec2(min.x + 2.0f, min.y + 2.0f),
                ImVec2(max.x - 2.0f, max.y - 2.0f),
                IM_COL32(106, 42, 174, 100),
                8.0f,
                0,
                1.0f);
        }
    }

    void renderCoverTexture(sf::Texture& texture, const ImVec2& size) {
        const auto native = texture.getSize();
        if (native.x == 0 || native.y == 0 || size.x <= 0.0f || size.y <= 0.0f)
            return;

        const float textureAspect = static_cast<float>(native.x) / static_cast<float>(native.y);
        const float viewAspect = size.x / size.y;

        sf::IntRect rect(0, 0, static_cast<int>(native.x), static_cast<int>(native.y));
        if (textureAspect > viewAspect) {
            const int visibleWidth = static_cast<int>(static_cast<float>(native.y) * viewAspect);
            rect.left = (static_cast<int>(native.x) - visibleWidth) / 2;
            rect.width = visibleWidth;
        }
        else if (textureAspect < viewAspect) {
            const int visibleHeight = static_cast<int>(static_cast<float>(native.x) / viewAspect);
            rect.top = (static_cast<int>(native.y) - visibleHeight) / 2;
            rect.height = visibleHeight;
        }

        sf::Sprite sprite(texture, rect);
        ImGui::Image(sprite, sf::Vector2f(size.x, size.y));
    }

    void renderNavItem(int index, const char* label, const char* imageId) {
        const bool selected = Menu::state.selectedTab == index;
        const ImVec2 start = ImGui::GetCursorScreenPos();
        const ImVec2 end(start.x + kNavWidth, start.y + kNavHeight);
        drawPanelFrame(start, end, selected);

        const std::string buttonId = "##revival-nav-" + std::to_string(index);
        ImGui::InvisibleButton(buttonId.c_str(), ImVec2(kNavWidth, kNavHeight));
        if (ImGui::IsItemClicked())
            Menu::state.selectedTab = index;

        sf::Texture* texture = ImageLoader::i().get(imageId);
        if (texture) {
            ImGui::SetCursorScreenPos(ImVec2(start.x + 7.0f, start.y + 6.0f));
            renderCoverTexture(*texture, ImVec2(46.0f, 46.0f));
        }

        ImGui::SetCursorScreenPos(ImVec2(start.x + 63.0f, start.y + 19.0f));
        ImGui::PushStyleColor(
            ImGuiCol_Text,
            selected ? ImVec4(0.97f, 0.94f, 1.0f, 1.0f) : ImVec4(0.72f, 0.70f, 0.78f, 1.0f));
        ImGui::TextUnformatted(label);
        ImGui::PopStyleColor();

        ImGui::SetCursorScreenPos(ImVec2(start.x, end.y + 7.0f));
    }

    void renderPageHeader(const char* title, const char* subtitle, const char* imageId) {
        ImGui::BeginChild("##page-heading", ImVec2(0.0f, 92.0f), false, ImGuiWindowFlags_NoScrollbar);
        const ImVec2 min = ImGui::GetWindowPos();
        const ImVec2 max(min.x + ImGui::GetWindowSize().x, min.y + ImGui::GetWindowSize().y);
        drawPanelFrame(min, max, false);

        if (sf::Texture* texture = ImageLoader::i().get(imageId)) {
            ImGui::SetCursorPos(ImVec2(14.0f, 12.0f));
            renderCoverTexture(*texture, ImVec2(66.0f, 66.0f));
        }

        ImGui::SetCursorPos(ImVec2(94.0f, 19.0f));
        ImGui::PushFont(Menu::bigFont);
        ImGui::TextUnformatted(title);
        ImGui::PopFont();
        ImGui::SetCursorPos(ImVec2(95.0f, 55.0f));
        ImGui::TextDisabled("%s", subtitle);
        ImGui::EndChild();
        ImGui::Spacing();
    }
}

void Menu::setColors() {
    if (!style)
        style = &ImGui::GetStyle();

    style->Colors[ImGuiCol_WindowBg] = *winCol;
    style->Colors[ImGuiCol_Border] = ImVec4(0.26f, 0.19f, 0.35f, 0.65f);
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
    style->Colors[ImGuiCol_Separator] = ImVec4(0.30f, 0.21f, 0.40f, 0.65f);
    style->Colors[ImGuiCol_SeparatorHovered] = *resizeGripHoverCol;
    style->Colors[ImGuiCol_SeparatorActive] = *itemActiveCol;
    style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.025f, 0.030f, 0.050f, 0.90f);
    style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.42f, 0.15f, 0.67f, 0.90f);
    style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.58f, 0.20f, 0.86f, 1.0f);
    style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.70f, 0.28f, 1.0f, 1.0f);
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
    bigFont = io.Fonts->AddFontFromMemoryTTF((void*)poppinsFont, sizeof(poppinsFont), 28.0f, &bigFontConfig);

    ImGui::SFML::UpdateFontTexture();
}

void Menu::loadTheme() {
    loadFont();

    style = &ImGui::GetStyle();
    style->WindowRounding = 8.0f;
    style->ChildRounding = 9.0f;
    style->FrameRounding = 6.0f;
    style->GrabRounding = 8.0f;
    style->PopupRounding = 8.0f;
    style->ScrollbarRounding = 9.0f;
    style->ScrollbarSize = 13.0f;
    style->WindowPadding = ImVec2(14.0f, 14.0f);
    style->FramePadding = ImVec2(9.0f, 7.0f);
    style->ItemSpacing = ImVec2(9.0f, 9.0f);

    setColors();
    loadEmbeddedAssets();

    BackgroundManager::i().initialize();
    ThemeManager::i().applyPreset(ThemeManager::i().currentPreset());
}

void Menu::renderLogo() {
    ImGui::BeginChild(
        "##revival-brand-header",
        ImVec2(0.0f, kHeaderHeight),
        false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoInputs);

    const ImVec2 min = ImGui::GetWindowPos();
    const ImVec2 max(min.x + ImGui::GetWindowSize().x, min.y + ImGui::GetWindowSize().y);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(min, max, IM_COL32(5, 7, 13, 255), 10.0f);

    if (sf::Texture* texture = ImageLoader::i().get("revival.header")) {
        ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
        renderCoverTexture(*texture, ImGui::GetContentRegionAvail());
    }
    else {
        ImGui::SetCursorPos(ImVec2(28.0f, 72.0f));
        ImGui::PushFont(bigFont);
        ImGui::TextUnformatted("REVIVAL V2");
        ImGui::PopFont();
        ImGui::TextDisabled("Embedded banner failed to load");
    }

    draw->AddRect(min, max, IM_COL32(110, 52, 160, 175), 10.0f, 0, 1.4f);
    draw->AddLine(ImVec2(min.x + 10.0f, max.y - 2.0f), ImVec2(max.x - 10.0f, max.y - 2.0f), IM_COL32(151, 62, 224, 165), 2.0f);
    ImGui::EndChild();
}

void Menu::renderUser() {
    ImGui::BeginChild("##sidebar-status", ImVec2(kSidebarInnerWidth, kStatusHeight), false, ImGuiWindowFlags_NoScrollbar);
    const ImVec2 min = ImGui::GetWindowPos();
    const ImVec2 max(min.x + ImGui::GetWindowSize().x, min.y + ImGui::GetWindowSize().y);
    drawPanelFrame(min, max, false);

    if (sf::Texture* texture = ImageLoader::i().get("revival.themes")) {
        ImGui::SetCursorPos(ImVec2(10.0f, 13.0f));
        renderCoverTexture(*texture, ImVec2(65.0f, 65.0f));
    }

    ImGui::SetCursorPos(ImVec2(86.0f, 15.0f));
    ImGui::TextUnformatted("Revival v2.1");
    ImGui::SetCursorPos(ImVec2(86.0f, 41.0f));
    ImGui::TextDisabled("Reaper Build");
    ImGui::SetCursorPos(ImVec2(86.0f, 68.0f));
    ImGui::TextColored(
        LuaManager::i().isAvailable() ? ImVec4(0.38f, 0.95f, 0.56f, 1.0f) : ImVec4(1.0f, 0.55f, 0.35f, 1.0f),
        "%s  %.0f FPS",
        LuaManager::i().isAvailable() ? "ACTIVE" : "LUA...",
        ImGui::GetIO().Framerate);
    ImGui::EndChild();
}

void Menu::renderPanel() {
    renderTabs();
    ImGui::Spacing();
    renderUser();
}

void Menu::renderTabs() {
    const float availableHeight = ImGui::GetContentRegionAvail().y;
    const float tabsHeight = std::max(220.0f, availableHeight - kStatusHeight - style->ItemSpacing.y);

    ImGui::BeginChild(
        "##sidebar-tabs",
        ImVec2(kSidebarInnerWidth, tabsHeight),
        false,
        ImGuiWindowFlags_AlwaysVerticalScrollbar);

    static ImGuiTextFilter2 filter;
    filter.Draw2(ICON_FA_SEARCH " Search", kNavWidth - style->ScrollbarSize - 5.0f);
    ImGui::Spacing();

    struct NavEntry {
        const char* label;
        const char* imageId;
    };

    const std::array<NavEntry, 6> entries = {{
        { "LegitBot", "revival.legit" },
        { "Visuals", "revival.visuals" },
        { "Misc", "revival.misc" },
        { "Scripts", "revival.scripts" },
        { "Themes", "revival.themes" },
        { "Configs", "revival.configs" },
    }};

    for (int i = 0; i < static_cast<int>(entries.size()); ++i) {
        if (!filter.PassFilter(entries[i].label))
            continue;
        renderNavItem(i, entries[i].label, entries[i].imageId);
    }

    ImGui::Dummy(ImVec2(0.0f, 28.0f));
    ImGui::EndChild();
}

void Menu::renderLegit() {
    renderPageHeader("LegitBot", "Legit assistance and recoil controls", "revival.legit");

    ImGuiHelper::drawTabHorizontally(
        "##legit-tabs",
        ImVec2(ImGuiHelper::getWidth(), 58.0f),
        { obf("Aim Assist"), obf("Recoil Control") },
        state.legitSubTab);

    ImGui::Spacing();
    if (state.legitSubTab == 0)
        AimAssist::i().renderImGui();
    else
        RCS::i().renderImGui();

    ImGui::Dummy(ImVec2(0.0f, 24.0f));
}

void Menu::renderVisuals() {
    renderPageHeader("Visuals", "ESP, markers and display options", "revival.visuals");
    ImGuiHelper::drawTabHorizontally(
        "##visual-tabs",
        ImVec2(ImGuiHelper::getWidth(), 58.0f),
        { obf("ESP"), obf("Markers") },
        state.visualSubTab);

    ImGui::Spacing();
    if (state.visualSubTab == 0)
        ESP::i().renderImGui();
    else
        Marker::i().renderImGui();
    ImGui::Dummy(ImVec2(0.0f, 24.0f));
}

void Menu::renderMisc() {
    renderPageHeader("Misc", "HUD and utility settings", "revival.misc");
    HUD::i().renderImGui();
    ImGui::Dummy(ImVec2(0.0f, 24.0f));
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
    BackgroundManager::i().render(windowPos, ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y));

    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRect(windowPos, ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y), IM_COL32(111, 48, 157, 180), 8.0f, 0, 1.4f);
    draw->AddRect(ImVec2(windowPos.x + 4.0f, windowPos.y + 4.0f), ImVec2(windowPos.x + windowSize.x - 4.0f, windowPos.y + windowSize.y - 4.0f), IM_COL32(62, 42, 88, 120), 6.0f, 0, 1.0f);

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
        ImGui::BeginChild(
            "##main-content-scroll",
            ImVec2(0.0f, 0.0f),
            false,
            ImGuiWindowFlags_AlwaysVerticalScrollbar);

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
            renderPageHeader("Scripts", "Lua + Sol2 script management", "revival.scripts");
            LuaManager::i().renderMenu();
            break;
        case 4:
            renderPageHeader("Themes", "Customize the look and feel of Revival V2", "revival.themes");
            ThemeManager::i().renderMenu();
            break;
        case 5:
            renderPageHeader("Configs", "Save and load Revival V2 profiles", "revival.configs");
            Config::i().renderImGui();
            break;
        default:
            state.selectedTab = 0;
            break;
        }

        ImGui::EndChild();
        ImGui::EndTable();
    }

    ImGui::End();
}
