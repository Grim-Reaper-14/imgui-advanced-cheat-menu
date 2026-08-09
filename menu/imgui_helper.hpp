#pragma once

#include "imgui.h"

#include <string>
#include <vector>

namespace ImGuiHelper {
    void drawTabHorizontally(
        const std::string& childName,
        const ImVec2& childSize,
        const std::vector<std::string>& tabNames,
        int& selectedSubTab);

    float getWidth();
    float getHeight();

    ImVec4 rgbaToVec4(float r, float g, float b, float a);
    ImVec4 rgbaToVec4(const ImColor& color);

    ImVec2 getTextLength(const std::string& text);

    void renderCombo(
        const std::string& title,
        const std::vector<std::string>& items,
        int& index,
        float comboWidth);
}
