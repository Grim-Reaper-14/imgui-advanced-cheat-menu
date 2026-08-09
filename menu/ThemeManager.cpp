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
    childRounding_ = &SetManager::i().add(new Set(9.0f, 0.0f, 24.0f, "childRounding", "Appearance")).getVec3f();
    frameRounding_ = &SetManager::i().add(new Set(6.0f, 0.0f, 16.0f, "frameRounding", "Appearance")).getVec3f();
    uiScale_ = &SetManager::i().add(new Set(1.0f, 0.80f, 1.35f, "uiScale", "Appearance")).getVec3f();
}

int ThemeManager::currentPreset() const {
    return preset_ ? *preset_ : 0;
}

void ThemeManager::applyPreset(int preset) {
    *preset_ = preset;

    switch (preset) {
    default:
    case 0:
        *Menu::winCol = rgba(5, 7, 13, 248);
        *Menu::bgCol = rgba(12, 15, 26, 252);
        *Menu::childCol = rgba(10, 13, 23, 244);
        *Menu::childCol1 = rgba(15, 18, 31, 248);
        *Menu::notSelectedTextColor = rgba(178, 170, 196);
        *Menu::textCol = rgba(245, 240, 252);
        *Menu::btnActiveCol = rgba(92, 31, 151);
        *Menu::btnHoverCol = rgba(68, 28, 111);
        *Menu::frameCol = rgba(31, 24, 47);
        *Menu::hoverCol = rgba(26, 20, 40);
        *Menu::itemCol = rgba(148, 57, 224);
        *Menu::itemActiveCol = rgba(183, 72, 255);
        *Menu::resizeGripCol = rgba(126, 42, 196, 155);
        *Menu::resizeGripHoverCol = rgba(184, 73, 255, 210);
        BackgroundManager::i().setPalette(rgba(5, 7, 14, 250), rgba(49, 19, 75, 225));
        break;
    case 1:
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
    case 2:
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
    case 3:
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
    case 4:
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
    case 5:
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
        "Gothic Purple",
        "Midnight",
        "Emerald",
        "Sunset",
        "Mono",
        "Neon"
    };

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.035f, 0.045f, 0.075f, 0.92f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.34f, 0.18f, 0.48f, 0.72f));

    ImGui::BeginChild("##theme-preset-card", ImVec2(0.0f, 112.0f), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::TextUnformatted("Theme Preset");
    ImGui::TextDisabled("Choose the base palette for Revival V2");
    ImGui::Spacing();
    ImGui::SetNextItemWidth(320.0f);
    const int before = *preset_;
    if (ImGui::Combo("##theme-preset", preset_, presets, IM_ARRAYSIZE(presets)) && before != *preset_)
        applyPreset(*preset_);
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::BeginChild("##theme-background-card", ImVec2(0.0f, 500.0f), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::TextUnformatted("Background");
    ImGui::TextDisabled("Image, gradient, animation and tint controls");
    ImGui::Spacing();
    BackgroundManager::i().renderSettings();
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::BeginChild("##theme-interface-card", ImVec2(0.0f, 260.0f), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::TextUnformatted("Interface");
    ImGui::TextDisabled("Shape, spacing and scale");
    ImGui::Spacing();

    ImGui::SetNextItemWidth(360.0f);
    ImGui::SliderFloat("Window rounding", &windowRounding_->x, windowRounding_->y, windowRounding_->z, "%.0f");
    ImGui::SetNextItemWidth(360.0f);
    ImGui::SliderFloat("Panel rounding", &childRounding_->x, childRounding_->y, childRounding_->z, "%.0f");
    ImGui::SetNextItemWidth(360.0f);
    ImGui::SliderFloat("Control rounding", &frameRounding_->x, frameRounding_->y, frameRounding_->z, "%.0f");
    ImGui::SetNextItemWidth(360.0f);
    ImGui::SliderFloat("UI scale", &uiScale_->x, uiScale_->y, uiScale_->z, "%.2fx");

    ImGui::Spacing();
    if (ImGui::Button("Reset preset colors", ImVec2(200.0f, 36.0f)))
        applyPreset(*preset_);

    ImGui::EndChild();
    ImGui::PopStyleColor(2);

    ImGui::Dummy(ImVec2(0.0f, 18.0f));
    applyLiveStyle();
}
