#include "ThemeManager.hpp"

#include "BackgroundManager.hpp"
#include "Menu.hpp"

namespace {
    ImVec4 rgba(int r, int g, int b, int a = 255) {
        return ImVec4(
            static_cast<float>(r) / 255.0f,
            static_cast<float>(g) / 255.0f,
            static_cast<float>(b) / 255.0f,
            static_cast<float>(a) / 255.0f);
    }
}

ThemeManager::ThemeManager() {
    preset_ = &SetManager::i().add(new Set(0, "themePreset", "Appearance")).getIVal();
    windowRounding_ = &SetManager::i().add(new Set(8.0f, 0.0f, 24.0f, "windowRounding", "Appearance")).getVec3f();
    childRounding_ = &SetManager::i().add(new Set(8.0f, 0.0f, 24.0f, "childRounding", "Appearance")).getVec3f();
    frameRounding_ = &SetManager::i().add(new Set(4.0f, 0.0f, 16.0f, "frameRounding", "Appearance")).getVec3f();
    uiScale_ = &SetManager::i().add(new Set(1.0f, 0.80f, 1.35f, "uiScale", "Appearance")).getVec3f();
}

int ThemeManager::currentPreset() const {
    return preset_ ? *preset_ : 0;
}

void ThemeManager::applyPreset(int preset) {
    *preset_ = preset;

    switch (preset) {
    default:
    case 0: // Classic Red
        *Menu::winCol = rgba(0, 0, 0, 230);
        *Menu::bgCol = rgba(31, 30, 31);
        *Menu::childCol = rgba(33, 34, 45);
        *Menu::childCol1 = rgba(38, 39, 50);
        *Menu::btnActiveCol = rgba(239, 73, 88);
        *Menu::btnHoverCol = rgba(173, 55, 65);
        *Menu::itemCol = rgba(240, 74, 88);
        *Menu::itemActiveCol = rgba(240, 50, 66);
        BackgroundManager::i().setPalette(rgba(18, 15, 18, 245), rgba(95, 22, 35, 210));
        break;

    case 1: // Midnight
        *Menu::winCol = rgba(10, 12, 20, 238);
        *Menu::bgCol = rgba(20, 24, 37);
        *Menu::childCol = rgba(23, 28, 44);
        *Menu::childCol1 = rgba(29, 35, 53);
        *Menu::btnActiveCol = rgba(90, 105, 255);
        *Menu::btnHoverCol = rgba(62, 75, 185);
        *Menu::itemCol = rgba(120, 92, 255);
        *Menu::itemActiveCol = rgba(87, 112, 255);
        BackgroundManager::i().setPalette(rgba(10, 13, 28, 248), rgba(57, 26, 96, 225));
        break;

    case 2: // Emerald
        *Menu::winCol = rgba(7, 18, 17, 238);
        *Menu::bgCol = rgba(20, 34, 31);
        *Menu::childCol = rgba(24, 40, 36);
        *Menu::childCol1 = rgba(29, 48, 43);
        *Menu::btnActiveCol = rgba(38, 198, 137);
        *Menu::btnHoverCol = rgba(29, 133, 97);
        *Menu::itemCol = rgba(48, 214, 151);
        *Menu::itemActiveCol = rgba(33, 186, 128);
        BackgroundManager::i().setPalette(rgba(7, 24, 20, 248), rgba(12, 85, 62, 220));
        break;

    case 3: // Sunset
        *Menu::winCol = rgba(27, 13, 20, 238);
        *Menu::bgCol = rgba(43, 24, 31);
        *Menu::childCol = rgba(48, 27, 37);
        *Menu::childCol1 = rgba(56, 31, 42);
        *Menu::btnActiveCol = rgba(255, 105, 92);
        *Menu::btnHoverCol = rgba(192, 70, 74);
        *Menu::itemCol = rgba(255, 142, 82);
        *Menu::itemActiveCol = rgba(246, 88, 103);
        BackgroundManager::i().setPalette(rgba(35, 16, 31, 248), rgba(126, 45, 53, 220));
        break;

    case 4: // Mono
        *Menu::winCol = rgba(14, 14, 14, 240);
        *Menu::bgCol = rgba(28, 28, 28);
        *Menu::childCol = rgba(34, 34, 34);
        *Menu::childCol1 = rgba(42, 42, 42);
        *Menu::btnActiveCol = rgba(205, 205, 205);
        *Menu::btnHoverCol = rgba(120, 120, 120);
        *Menu::itemCol = rgba(225, 225, 225);
        *Menu::itemActiveCol = rgba(190, 190, 190);
        BackgroundManager::i().setPalette(rgba(12, 12, 12, 248), rgba(55, 55, 55, 225));
        break;

    case 5: // Neon
        *Menu::winCol = rgba(7, 10, 17, 238);
        *Menu::bgCol = rgba(16, 22, 32);
        *Menu::childCol = rgba(18, 27, 39);
        *Menu::childCol1 = rgba(23, 33, 48);
        *Menu::btnActiveCol = rgba(0, 214, 255);
        *Menu::btnHoverCol = rgba(136, 46, 191);
        *Menu::itemCol = rgba(0, 230, 208);
        *Menu::itemActiveCol = rgba(192, 64, 255);
        BackgroundManager::i().setPalette(rgba(8, 12, 27, 248), rgba(64, 10, 92, 220));
        break;
    }

    Menu::setColors();
}

void ThemeManager::applyLiveStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = windowRounding_->x;
    style.ChildRounding = childRounding_->x;
    style.FrameRounding = frameRounding_->x;
    style.GrabRounding = frameRounding_->x;
    style.PopupRounding = childRounding_->x;

    ImGui::GetIO().FontGlobalScale = uiScale_->x;
}

void ThemeManager::renderMenu() {
    static const char* presets[] = {
        "Classic Red",
        "Midnight",
        "Emerald",
        "Sunset",
        "Mono",
        "Neon"
    };

    ImGui::TextUnformatted("Theme preset");
    ImGui::SetNextItemWidth(240.0f);
    const int before = *preset_;
    if (ImGui::Combo("##theme-preset", preset_, presets, IM_ARRAYSIZE(presets)) && before != *preset_)
        applyPreset(*preset_);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    BackgroundManager::i().renderSettings();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextUnformatted("Interface");
    ImGui::SetNextItemWidth(240.0f);
    ImGui::SliderFloat("Window rounding", &windowRounding_->x, windowRounding_->y, windowRounding_->z, "%.0f");
    ImGui::SetNextItemWidth(240.0f);
    ImGui::SliderFloat("Panel rounding", &childRounding_->x, childRounding_->y, childRounding_->z, "%.0f");
    ImGui::SetNextItemWidth(240.0f);
    ImGui::SliderFloat("Control rounding", &frameRounding_->x, frameRounding_->y, frameRounding_->z, "%.0f");
    ImGui::SetNextItemWidth(240.0f);
    ImGui::SliderFloat("UI scale", &uiScale_->x, uiScale_->y, uiScale_->z, "%.2fx");

    if (ImGui::Button("Reset preset colors"))
        applyPreset(*preset_);

    applyLiveStyle();
}
