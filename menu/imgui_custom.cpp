#include "imgui_custom.hpp"

#include "../util/ColorH.hpp"
#include "../util/StringH.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

#include <Windows.h>

#include <algorithm>
#include <string>

namespace {
    ImGuiID g_activeHotkey = 0;
    bool g_waitForRelease = false;

    bool anyBindableKeyDown() {
        for (int key = VK_LBUTTON; key <= VK_PACKET; ++key) {
            if (GetAsyncKeyState(key) & 0x8000)
                return true;
        }
        return false;
    }
}

bool ImGui::Checkbox_(const char* label, bool* value) {
    // Keep the old public name, but use the supported Dear ImGui widget path.
    return ImGui::Checkbox(label, value);
}

bool ImGui::SliderFloat_(
    const char* label,
    float* value,
    float minValue,
    float maxValue,
    const char* format,
    ImGuiSliderFlags flags) {
    return ImGui::SliderFloat(label, value, minValue, maxValue, format, flags);
}

bool ImGui::SliderInt_(
    const char* label,
    int* value,
    int minValue,
    int maxValue,
    const char* format,
    ImGuiSliderFlags flags) {
    return ImGui::SliderInt(label, value, minValue, maxValue, format, flags);
}

bool ImGuiTextFilter2::Draw2(const char* label, float width) {
    if (width != 0.0f)
        ImGui::SetNextItemWidth(width);

    const std::string id = std::string("##Input_") + label;
    const bool changed = ImGui::InputTextWithHint(id.c_str(), label, InputBuf, IM_ARRAYSIZE(InputBuf));
    if (changed)
        Build();
    return changed;
}

bool ImGui::Hotkey(const char* label, int& key, float samelineOffset, const ImVec2& size) {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(label);

    TextUnformatted(label);
    SameLine(samelineOffset);

    std::string buttonText;
    if (g_activeHotkey == id)
        buttonText = "Press a key...";
    else if (key == 0)
        buttonText = "None";
    else
        buttonText = StringH::vkToString(key);

    bool changed = false;
    if (Button(buttonText.c_str(), size)) {
        g_activeHotkey = id;
        g_waitForRelease = true;
    }

    if (g_activeHotkey != id)
        return changed;

    if (g_waitForRelease) {
        if (!anyBindableKeyDown())
            g_waitForRelease = false;
        return changed;
    }

    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        g_activeHotkey = 0;
        return false;
    }

    if ((GetAsyncKeyState(VK_BACK) & 0x8000) || (GetAsyncKeyState(VK_DELETE) & 0x8000)) {
        key = 0;
        g_activeHotkey = 0;
        return true;
    }

    for (int candidate = VK_LBUTTON; candidate <= VK_PACKET; ++candidate) {
        if (candidate == VK_ESCAPE || candidate == VK_BACK || candidate == VK_DELETE)
            continue;

        if (GetAsyncKeyState(candidate) & 0x8000) {
            key = candidate;
            g_activeHotkey = 0;
            changed = true;
            break;
        }
    }

    return changed;
}

void ImGui::chromaText(
    const std::string& text,
    float saturation,
    float value,
    float alpha,
    float offset,
    float speed,
    float range) {
    for (std::size_t i = 0; i < text.size(); ++i) {
        const std::string character(1, text[i]);

        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(-0.5f, 4.0f));

        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;
        ColorH::HSVtoRGB(
            ColorH::getTimeHue(static_cast<float>(i) * range, speed, offset),
            saturation,
            value,
            r,
            g,
            b);

        PushStyleColor(ImGuiCol_Text, ImVec4(r, g, b, alpha));
        TextUnformatted(character.c_str());
        PopStyleColor();

        if (i + 1 < text.size())
            SameLine();

        PopStyleVar();
    }
}
