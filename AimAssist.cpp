#include "AimAssist.hpp"

#include "SetManager.hpp"
#include "menu/Menu.hpp"
#include "menu/imgui_custom.hpp"
#include "menu/imgui_helper.hpp"

namespace {
    void beginCard(const char* id, const char* title, const ImVec2& size) {
        ImGui::BeginChild(id, size, true, ImGuiWindowFlags_NoScrollbar);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.90f, 1.0f, 1.0f));
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

    silentAim = &SetManager::i().add(new Set(false, obf("silentAim"), getName())).getBVal();
    aimOnKey = &SetManager::i().add(new Set(true, obf("aimOnKey"), getName())).getBVal();
    visibleCheck = &SetManager::i().add(new Set(true, obf("visibleCheck"), getName())).getBVal();
    prediction = &SetManager::i().add(new Set(true, obf("prediction"), getName())).getBVal();
    latencyCompensation = &SetManager::i().add(new Set(true, obf("latencyCompensation"), getName())).getBVal();
    bulletTravelTime = &SetManager::i().add(new Set(true, obf("bulletTravelTime"), getName())).getBVal();

    usePistols = &SetManager::i().add(new Set(true, obf("usePistols"), getName())).getBVal();
    useRifles = &SetManager::i().add(new Set(true, obf("useRifles"), getName())).getBVal();
    useSMGs = &SetManager::i().add(new Set(true, obf("useSMGs"), getName())).getBVal();
    useSnipers = &SetManager::i().add(new Set(true, obf("useSnipers"), getName())).getBVal();
    useShotguns = &SetManager::i().add(new Set(false, obf("useShotguns"), getName())).getBVal();

    hitbox = &SetManager::i().add(new Set(0, obf("hitbox"), getName())).getIVal();
    targetSelection = &SetManager::i().add(new Set(0, obf("targetSelection"), getName())).getIVal();
    hitboxPriority = &SetManager::i().add(new Set(0, obf("hitboxPriority"), getName())).getIVal();
    onHold = &SetManager::i().add(new Set(0, obf("onHold"), getName())).getIVal();

    reactTime = &SetManager::i().add(new Set(200, 0, 1000, obf("reactionTime"), getName())).getVec3i();
    fov = &SetManager::i().add(new Set(40, 0, 360, obf("fov"), getName())).getVec3i();
    triggerDelay = &SetManager::i().add(new Set(0, 0, 500, obf("triggerDelay"), getName())).getVec3i();

    maxDistance = &SetManager::i().add(new Set(350.0f, 25.0f, 1000.0f, obf("maxDistance"), getName())).getVec3f();
    smoothAmount = &SetManager::i().add(new Set(6.0f, 1.0f, 20.0f, obf("smoothAmount"), getName())).getVec3f();
    interpolation = &SetManager::i().add(new Set(0.5f, 0.0f, 1.0f, obf("interpolation"), getName())).getVec3f();
    randomization = &SetManager::i().add(new Set(0.0f, 0.0f, 1.0f, obf("randomization"), getName())).getVec3f();
}

void AimAssist::renderImGui() {
    const float gap = 12.0f;
    const float width = ImGui::GetContentRegionAvail().x;
    const float columnWidth = (width - gap) * 0.5f;

    beginCard("##aim-main-card", "Aimbot", ImVec2(columnWidth, 330.0f));
    ImGui::Checkbox_(obf("Enable Aim Assist").c_str(), &isToggled());
    ImGui::Checkbox_(obf("Silent Aim").c_str(), silentAim);
    ImGui::Checkbox_(obf("Aim on Key").c_str(), aimOnKey);
    if (*aimOnKey)
        ImGui::Hotkey(obf("Aim Key").c_str(), getKey());
    ImGui::Checkbox_(obf("Visible Check").c_str(), visibleCheck);
    ImGui::Checkbox_(obf("No Snap").c_str(), noSnap);
    ImGui::Checkbox_(obf("No Lock").c_str(), noLock);
    ImGui::SetNextItemWidth(190.0f);
    ImGui::SliderInt_(obf("Reaction ms").c_str(), &reactTime->x, reactTime->y, reactTime->z);
    ImGui::SetNextItemWidth(190.0f);
    ImGui::SliderInt_(obf("Trigger Delay").c_str(), &triggerDelay->x, triggerDelay->y, triggerDelay->z);
    endCard();

    ImGui::SameLine(0.0f, gap);

    beginCard("##aim-target-card", "Target Selection", ImVec2(columnWidth, 330.0f));
    ImGuiHelper::renderCombo(
        obf("Target Type"),
        { obf("Closest"), obf("Lowest Health") },
        *targetSelection,
        190.0f);

    ImGuiHelper::renderCombo(
        obf("Target Bone"),
        { obf("Head"), obf("Neck"), obf("Body"), obf("Arms"), obf("Hip"), obf("Legs") },
        *hitbox,
        190.0f);

    ImGuiHelper::renderCombo(
        obf("Hitbox Priority"),
        { obf("Head > Neck > Chest"), obf("Chest > Head > Neck"), obf("Closest Bone") },
        *hitboxPriority,
        190.0f);

    ImGui::SetNextItemWidth(190.0f);
    ImGui::SliderFloat_(obf("Max Distance").c_str(), &maxDistance->x, maxDistance->y, maxDistance->z, "%.0f");
    ImGui::SetNextItemWidth(190.0f);
    ImGui::SliderInt_(obf("FOV Radius").c_str(), &fov->x, fov->y, fov->z);
    ImGui::SetNextItemWidth(190.0f);
    ImGui::SliderFloat_(obf("Smooth Amount").c_str(), &smoothAmount->x, smoothAmount->y, smoothAmount->z, "%.1f");
    ImGui::Checkbox_(obf("Relative FOV").c_str(), relativeFOV);
    endCard();

    ImGui::Spacing();

    beginCard("##aim-weapons-card", "Weapons", ImVec2(columnWidth, 250.0f));
    ImGui::Checkbox_(obf("Pistols").c_str(), usePistols);
    ImGui::Checkbox_(obf("Rifles").c_str(), useRifles);
    ImGui::Checkbox_(obf("SMGs").c_str(), useSMGs);
    ImGui::Checkbox_(obf("Snipers").c_str(), useSnipers);
    ImGui::Checkbox_(obf("Shotguns").c_str(), useShotguns);
    endCard();

    ImGui::SameLine(0.0f, gap);

    beginCard("##aim-advanced-card", "Advanced", ImVec2(columnWidth, 250.0f));
    ImGui::Checkbox_(obf("Prediction").c_str(), prediction);
    ImGui::Checkbox_(obf("Latency Compensation").c_str(), latencyCompensation);
    ImGui::Checkbox_(obf("Bullet Travel Time").c_str(), bulletTravelTime);
    ImGui::SetNextItemWidth(190.0f);
    ImGui::SliderFloat_(obf("Interpolation").c_str(), &interpolation->x, interpolation->y, interpolation->z, "%.2f");
    ImGui::SetNextItemWidth(190.0f);
    ImGui::SliderFloat_(obf("Randomization").c_str(), &randomization->x, randomization->y, randomization->z, "%.2f");
    endCard();

    ImGui::Spacing();
    ImGui::BeginChild("##aim-tip-card", ImVec2(0.0f, 82.0f), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.58f, 0.90f, 1.0f));
    ImGui::TextUnformatted("Revival Tip");
    ImGui::PopStyleColor();
    ImGui::TextWrapped("Use a smaller FOV, more smoothing and a higher reaction delay for subtler aim assistance.");
    ImGui::EndChild();

    ImGui::Dummy(ImVec2(0.0f, 24.0f));
}
