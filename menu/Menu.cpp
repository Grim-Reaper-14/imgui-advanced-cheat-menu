#include "Menu.hpp"

#include "../AimAssist.hpp"
#include "../Config.hpp"
#include "../ESP.hpp"
#include "../HUD.hpp"
#include "../Marker.hpp"
#include "../RCS.hpp"
#include "../scripting/LuaManager.hpp"
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
#include <vector>

namespace {
    constexpr float kSidebarWidth = 220.0f;
    constexpr float kSidebarInnerWidth = 204.0f;
    constexpr float kWindowWidth = 1080.0f;
    constexpr float kWindowHeight = 800.0f;
    constexpr float kHeaderHeight = 224.0f;
    constexpr float kNavWidth = 186.0f;
    constexpr float kNavHeight = 48.0f;
    constexpr float kStatusHeight = 82.0f;

    constexpr const char* kLegitIcon = ICON_FA_CROSSHAIRS;
    constexpr const char* kVisualIcon = ICON_FA_EYE;
    constexpr const char* kMiscIcon = ICON_FA_COG;
    constexpr const char* kScriptIcon = "\xEF\x84\xA1";
    constexpr const char* kThemeIcon = "\xEF\x94\xBF";
    constexpr const char* kConfigIcon = ICON_FA_SAVE;

    std::filesystem::path executableDirectory() {
        std::array<wchar_t, 32768> buffer{};
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0 || length >= buffer.size())
            return {};

        return std::filesystem::path(buffer.data(), buffer.data() + length).parent_path();
    }

    std::filesystem::path findAssetFrom(std::filesystem::path base, const char* fileName) {
        if (base.empty())
            return {};

        std::error_code ec;
        for (int depth = 0; depth < 10; ++depth) {
            const auto candidate = base / "assets" / "icons" / fileName;
            if (std::filesystem::exists(candidate, ec) && !ec)
                return candidate;

            ec.clear();
            if (!base.has_parent_path())
                break;

            const auto parent = base.parent_path();
            if (parent == base)
                break;
            base = parent;
        }

        return {};
    }

    std::filesystem::path resolveAsset(const char* fileName) {
        std::error_code ec;

        const auto cwd = std::filesystem::current_path(ec);
        if (!ec) {
            if (const auto path = findAssetFrom(cwd, fileName); !path.empty())
                return path;
        }

        if (const auto path = findAssetFrom(executableDirectory(), fileName); !path.empty())
            return path;

        const std::filesystem::path sourceFile = __FILE__;
        if (sourceFile.is_absolute()) {
            if (const auto path = findAssetFrom(sourceFile.parent_path(), fileName); !path.empty())
                return path;
        }

        return {};
    }

    bool loadUiAsset(const char* id, const char* fileName) {
        const auto path = resolveAsset(fileName);
        if (path.empty()) {
            Console::i().logError(std::string("Missing Revival UI asset: ") + fileName);
            return false;
        }

        if (!ImageLoader::i().loadFromFile(id, path, true, false)) {
            Console::i().logError(std::string("Failed to load Revival UI asset: ") + path.string());
            return false;
        }

        Console::i().logInfo(std::string("Loaded Revival UI asset: ") + path.string());
        return true;
    }

    void renderNavItem(
        int index,
        const char* label,
        const char* fallbackIcon,
        const char* imageId) {
        const bool selected = Menu::state.selectedTab == index;
        const ImVec4 transparent(0.0f, 0.0f, 0.0f, 0.0f);

        const ImVec2 start = ImGui::GetCursorScreenPos();
        const std::string id = "##nav-" + std::to_string(index);

        ImGui::PushStyleColor(
            ImGuiCol_Header,
            selected ? Menu::style->Colors[ImGuiCol_ButtonActive] : transparent);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Menu::style->Colors[ImGuiCol_ButtonHovered]);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, Menu::style->Colors[ImGuiCol_ButtonActive]);
        if (ImGui::Selectable(id.c_str(), selected, 0, ImVec2(kNavWidth, kNavHeight)))
            Menu::state.selectedTab = index;
        ImGui::PopStyleColor(3);

        const ImVec2 after = ImGui::GetCursorScreenPos();
        const ImVec2 iconMin(start.x + 8.0f, start.y + 7.0f);
        const ImVec2 iconMax(iconMin.x + 34.0f, iconMin.y + 34.0f);

        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(
            iconMin,
            iconMax,
            selected ? IM_COL32(255, 255, 255, 34) : IM_COL32(7, 10, 16, 150),
            6.0f);
        draw->AddRect(
            iconMin,
            iconMax,
            selected ? IM_COL32(255, 255, 255, 65) : IM_COL32(68, 76, 92, 80),
            6.0f);

        if (sf::Texture* texture = ImageLoader::i().get(imageId)) {
            ImGui::SetCursorScreenPos(ImVec2(iconMin.x + 2.0f, iconMin.y + 2.0f));
            ImGui::Image(*texture, sf::Vector2f(30.0f, 30.0f));
        }
        else {
            ImGui::SetCursorScreenPos(ImVec2(iconMin.x + 8.0f, iconMin.y + 8.0f));
            ImGui::PushStyleColor(
                ImGuiCol_Text,
                selected ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : *Menu::notSelectedTextColor);
            ImGui::TextUnformatted(fallbackIcon);
            ImGui::PopStyleColor();
        }

        ImGui::SetCursorScreenPos(ImVec2(start.x + 52.0f, start.y + 14.0f));
        ImGui::PushStyleColor(
            ImGuiCol_Text,
            selected ? Menu::style->Colors[ImGuiCol_Text] : *Menu::notSelectedTextColor);
        ImGui::TextUnformatted(label);
        ImGui::PopStyleColor();
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
    style->ScrollbarSize = 11.0f;
    style->WindowPadding = ImVec2(16.0f, 16.0f);
    style->FramePadding = ImVec2(8.0f, 6.0f);
    style->ItemSpacing = ImVec2(8.0f, 8.0f);

    setColors();

    loadUiAsset("revival.header", "revival_header.jpg");
    loadUiAsset("revival.legit", "legit_icon.jpg");
    loadUiAsset("revival.visuals", "visuals_icon.jpg");
    loadUiAsset("revival.misc", "misc_icon.jpg");
    loadUiAsset("revival.scripts", "scripts_icon.jpg");
    loadUiAsset("revival.themes", "themes_icon.jpg");
    loadUiAsset("revival.configs", "configs_icon.jpg");

    BackgroundManager::i().initialize();
    ThemeManager::i().applyPreset(ThemeManager::i().currentPreset());
}

void Menu::renderLogo() {
    ImGui::BeginChild(
        "##revival-brand-header",
        ImVec2(0.0f, kHeaderHeight),
        false,
        ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoInputs);

    if (sf::Texture* texture = ImageLoader::i().get("revival.header")) {
        const ImVec2 available = ImGui::GetContentRegionAvail();
        const auto native = texture->getSize();

        if (native.x > 0 && native.y > 0 && available.x > 0.0f && available.y > 0.0f) {
            const float textureAspect = static_cast<float>(native.x) / static_cast<float>(native.y);
            const float viewAspect = available.x / available.y;
            ImVec2 uv0(0.0f, 0.0f);
            ImVec2 uv1(1.0f, 1.0f);

            if (textureAspect > viewAspect) {
                const float visibleWidth = viewAspect / textureAspect;
                const float crop = (1.0f - visibleWidth) * 0.5f;
                uv0.x = crop;
                uv1.x = 1.0f - crop;
            }
            else {
                const float visibleHeight = textureAspect / viewAspect;
                const float crop = (1.0f - visibleHeight) * 0.5f;
                uv0.y = crop;
                uv1.y = 1.0f - crop;
            }

            ImGui::Image(
                *texture,
                sf::Vector2f(available.x, available.y),
                sf::Color::White,
                sf::Color::Transparent);
        }
    }
    else {
        ImGui::SetCursorPosY(72.0f);
        ImGui::PushFont(bigFont);
        ImGui::TextUnformatted("REVIVAL V2");
        ImGui::PopFont();
        ImGui::TextDisabled("Reaper Build - artwork missing");
    }

    ImGui::EndChild();
}

void Menu::renderUser() {
    ImGui::BeginChild(
        "##sidebar-status",
        ImVec2(kSidebarInnerWidth, kStatusHeight),
        true,
        ImGuiWindowFlags_NoScrollbar);
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
    const float availableHeight = ImGui::GetContentRegionAvail().y;
    const float tabsHeight = std::max(
        180.0f,
        availableHeight - kStatusHeight - style->ItemSpacing.y);

    ImGui::BeginChild(
        "##sidebar-tabs",
        ImVec2(kSidebarInnerWidth, tabsHeight),
        true,
        ImGuiWindowFlags_AlwaysVerticalScrollbar);

    static ImGuiTextFilter2 filter;
    filter.Draw2(ICON_FA_SEARCH " Search", kNavWidth - style->ScrollbarSize - 3.0f);
    ImGui::Spacing();

    struct NavEntry {
        const char* label;
        const char* fallbackIcon;
        const char* imageId;
    };

    const std::array<NavEntry, 6> navEntries = {{
        { "LegitBot", kLegitIcon, "revival.legit" },
        { "Visuals", kVisualIcon, "revival.visuals" },
        { "Misc", kMiscIcon, "revival.misc" },
        { "Scripts", kScriptIcon, "revival.scripts" },
        { "Themes", kThemeIcon, "revival.themes" },
        { "Configs", kConfigIcon, "revival.configs" },
    }};

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);

    for (int i = 0; i < static_cast<int>(navEntries.size()); ++i) {
        const auto& entry = navEntries[i];
        if (!filter.PassFilter(entry.label))
            continue;

        renderNavItem(i, entry.label, entry.fallbackIcon, entry.imageId);
        ImGui::Dummy(ImVec2(0.0f, 2.0f));
    }

    ImGui::Dummy(ImVec2(0.0f, 70.0f));

    ImGui::PopStyleVar();
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

        ImGui::EndChild();
        ImGui::EndTable();
    }

    ImGui::End();
}
