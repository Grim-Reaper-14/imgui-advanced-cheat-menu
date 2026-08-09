#pragma once

#include "imgui.h"
#include "imgui_helper.hpp"

#include <string>

namespace ImGui {
    IMGUI_API bool Hotkey(
        const char* label,
        int& key,
        float samelineOffset = 0.0f,
        const ImVec2& size = ImVec2(100.0f, 0.0f));

    // Compatibility names kept so existing modules do not need a rewrite yet.
    IMGUI_API bool Checkbox_(const char* label, bool* value);
    IMGUI_API bool SliderInt_(
        const char* label,
        int* value,
        int minValue,
        int maxValue,
        const char* format = "%d",
        ImGuiSliderFlags flags = 0);
    IMGUI_API bool SliderFloat_(
        const char* label,
        float* value,
        float minValue,
        float maxValue,
        const char* format = "%.3f",
        ImGuiSliderFlags flags = 0);

    IMGUI_API void chromaText(
        const std::string& text,
        float saturation,
        float value,
        float alpha,
        float offset,
        float speed,
        float range);
}

struct ImGuiTextFilter2 : public ImGuiTextFilter {
    IMGUI_API bool Draw2(const char* label = "Filter (inc,-exc)", float width = 0.0f);
};
