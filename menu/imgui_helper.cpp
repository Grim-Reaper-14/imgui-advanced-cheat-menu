#include "imgui_helper.hpp"

#include "Menu.hpp"

#include <algorithm>

float ImGuiHelper::getWidth() {
    return ImGui::GetContentRegionAvail().x;
}

float ImGuiHelper::getHeight() {
    return ImGui::GetContentRegionAvail().y;
}

void ImGuiHelper::drawTabHorizontally(
    const std::string& childName,
    const ImVec2& childSize,
    const std::vector<std::string>& tabNames,
    int& selectedSubTab) {
    if (tabNames.empty())
        return;

    selectedSubTab = std::clamp(selectedSubTab, 0, static_cast<int>(tabNames.size()) - 1);

    ImGui::BeginChild(childName.c_str(), childSize, true, ImGuiWindowFlags_NoScrollbar);

    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const float buttonWidth = std::clamp(
        (availableWidth - spacing * static_cast<float>(tabNames.size() - 1)) /
            static_cast<float>(tabNames.size()),
        80.0f,
        220.0f);
    const float buttonHeight = std::clamp(ImGui::GetContentRegionAvail().y, 28.0f, 44.0f);

    const float rowWidth = buttonWidth * static_cast<float>(tabNames.size()) +
        spacing * static_cast<float>(tabNames.size() - 1);
    if (rowWidth < availableWidth)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availableWidth - rowWidth) * 0.5f);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

    for (int i = 0; i < static_cast<int>(tabNames.size()); ++i) {
        const bool selected = selectedSubTab == i;
        ImGui::PushStyleColor(
            ImGuiCol_Button,
            selected ? ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] : ImGui::GetStyle().Colors[ImGuiCol_Button]);
        ImGui::PushStyleColor(
            ImGuiCol_Text,
            selected ? ImGui::GetStyle().Colors[ImGuiCol_Text] : *Menu::notSelectedTextColor);

        if (ImGui::Button(tabNames[i].c_str(), ImVec2(buttonWidth, buttonHeight)))
            selectedSubTab = i;

        ImGui::PopStyleColor(2);

        if (i + 1 < static_cast<int>(tabNames.size()))
            ImGui::SameLine();
    }

    ImGui::PopStyleVar();
    ImGui::EndChild();
}

ImVec4 ImGuiHelper::rgbaToVec4(float r, float g, float b, float a) {
    return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

ImVec4 ImGuiHelper::rgbaToVec4(const ImColor& color) {
    return color.Value;
}

ImVec2 ImGuiHelper::getTextLength(const std::string& text) {
    return ImGui::CalcTextSize(text.c_str());
}

void ImGuiHelper::renderCombo(
    const std::string& title,
    const std::vector<std::string>& items,
    int& index,
    float comboWidth) {
    if (items.empty())
        return;

    index = std::clamp(index, 0, static_cast<int>(items.size()) - 1);

    ImGui::PushItemWidth(comboWidth);
    if (ImGui::BeginCombo(title.c_str(), items[index].c_str())) {
        for (int i = 0; i < static_cast<int>(items.size()); ++i) {
            const bool selected = index == i;
            if (ImGui::Selectable(items[i].c_str(), selected))
                index = i;
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
}
