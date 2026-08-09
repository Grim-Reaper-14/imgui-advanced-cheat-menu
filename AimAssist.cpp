#include "AimAssist.hpp"

#include "SetManager.hpp"
#include "menu/Menu.hpp"
#include "menu/imgui_custom.hpp"
#include "menu/imgui_helper.hpp"

namespace {
    void beginCard(const char* id, const char* title, const ImVec2& size) {
        ImGui::BeginChild(id, size, true, ImGuiWindowFlags_NoScrollbar);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.86f, 1.0f, 1.0f));
        ImGui::TextUnformatted(title);
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();
    }

    void endCard() {
        ImGui::EndChild();
    }
}

AimAssist::AimAssist() : Module(obf("AimAssist"), obf("Helps with aiming")) {
    isEnemy = &SetManager::i().add(new Set(true, obf("isEnemy"), getName())).getBVal();
    isMate = &SetManager::i().add(new Set(false, obf("isMate"), getName())).getBVal();
    isBot = &SetManager::i().add(new Set(false, obf("isBot"), getName())).getBVal();
    relativeFOV = &SetManager::i().add(new Set(false, obf("relativeFOV"), getName())).getBVal();
    noSnap = &SetManager::i().add(new Set(false, obf("noSnap"), getName())).getBVal();
    noLock = &SetManager::i().add(new Set(false, obf("noLock"), getName())).getBVal();

    hitbox = &SetManager::i().add(new Set(0, obf("hitbox"), getName())).getIVal();
    targetSelection = &SetManager::i().add(new Set(0, obf("targetSelection"), getName())).getIVal();
    onHold = &SetManager::i().add(new Set(0, obf("onHold"), getName())).getIVal();
    reactTime = &SetManager::i().add(new Set(200, 0, 1000, obf("reactionTime"), getName())).getVec3i();
    fov = &SetManager::i().add(new Set(40, 0, 360, obf("fov"), getName())).getVec3i();
}

void AimAssist::renderImGui() {
    const float gap = 12.0f;
    const float width = ImGui::GetContentRegionAvail().x;
    const float columnWidth = (width - gap) * 0.5f;

    beginCard("##aim-main-card", "Aiming", ImVec2(columnWidth, 330.0f));
    ImGui::Checkbox_(obf("Enable Aim Assist").c_str(), &isToggled());
    ImGui::Spacing();

    ImGui::TextDisabled("Activation");
    ImGuiHelper::renderCombo("Mode", { obf("Toggle"), obf("Hold") }, *onHold, 170.0f);
    ImGui::Hotkey(obf("Aim Key").c_str(), getKey());

    ImGui::Spacing();
    ImGui::TextDisabled("Behavior");
    ImGui::Checkbox_(obf("No Snap").c_str(), noSnap);
    ImGui::Checkbox_(obf("No Lock").c_str(), noLock);

    ImGui::Spacing();
    ImGui::SetNextItemWidth(190.0f);
    ImGui::SliderInt_(obf("Reaction ms").c_str(), &reactTime->x, reactTime->y, reactTime->z);
    endCard();

    ImGui::SameLine(0.0f, gap);

    beginCard("##aim-target-card", "Target Selection", ImVec2(columnWidth, 330.0f));
    ImGuiHelper::renderCombo(
        obf("Priority"),
        { obf("By Distance"), obf("By Health") },
        *targetSelection,
        190.0f);

    std::string targets;
    if (*isEnemy || *isMate || *isBot) {
        if (*isEnemy && *isMate && *isBot) {
            targets = obf("All");
        }
        else {
            if (*isEnemy)
                targets += obf("Enemy,");
            if (*isMate)
                targets += obf("Mate,");
            if (*isBot)
                targets += obf("Bots,");
            if (!targets.empty() && targets.back() == ',')
                targets.pop_back();
        }
    }
    else {
        targets = obf("None");
    }

    ImGui::SetNextItemWidth(190.0f);
    if (ImGui::BeginCombo(obf("Targets").c_str(), targets.c_str())) {
        ImGui::Checkbox_(obf("Enemy").c_str(), isEnemy);
        ImGui::Checkbox_(obf("Mate").c_str(), isMate);
        ImGui::Checkbox_(obf("Bot").c_str(), isBot);
        ImGui::EndCombo();
    }

    ImGuiHelper::renderCombo(
        obf("Hitbox"),
        { obf("Head"), obf("Neck"), obf("Body"), obf("Arms"), obf("Hip"), obf("Legs") },
        *hitbox,
        190.0f);

    ImGui::SetNextItemWidth(190.0f);
    ImGui::SliderInt_(obf("FOV Radius").c_str(), &fov->x, fov->y, fov->z);
    ImGui::Checkbox_(obf("Relative FOV").c_str(), relativeFOV);
    endCard();

    ImGui::Spacing();
    ImGui::BeginChild("##aim-tip-card", ImVec2(0.0f, 78.0f), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.58f, 0.90f, 1.0f));
    ImGui::TextUnformatted("Revival Tip");
    ImGui::PopStyleColor();
    ImGui::TextWrapped("Use a smaller FOV and a higher reaction delay for subtler aim assistance.");
    ImGui::EndChild();
}
