#include "Marker.hpp"

#include "SetManager.hpp"
#include "menu/imgui_custom.hpp"
#include "menu/imgui_helper.hpp"

Marker::Marker() : Module(obf("Marker"), obf("Renders markers at entity positions")) {
    col = &SetManager::i().add(new Set(ImGuiHelper::rgbaToVec4(60, 160, 200, 200), obf("col"), getName())).getVec4();
    renderBehind = &SetManager::i().add(new Set(true, obf("renderBehind"), getName())).getBVal();
    renderInfront = &SetManager::i().add(new Set(false, obf("renderInfront"), getName())).getBVal();
}

void Marker::renderImGui() {
    ImGui::Checkbox_(obf("Enable Markers").c_str(), &isToggled());
    ImGui::ColorEdit4(obf("Marker Color##1").c_str(), reinterpret_cast<float*>(col), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    ImGui::Checkbox_(obf("Render Behind").c_str(), renderBehind);
    ImGui::Checkbox_(obf("Render In Front").c_str(), renderInfront);
}
