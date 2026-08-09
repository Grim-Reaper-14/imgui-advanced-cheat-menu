#pragma once

#include "imgui.h"
#include "util/Obf.hpp"
#include "util/Singleton.hpp"
#include "Module.hpp"

class AimAssist : public Singleton<AimAssist>, public Module {
    friend class Singleton<AimAssist>;

public:
    bool* isEnemy;
    bool* isMate;
    bool* isBot;
    bool* relativeFOV;
    bool* noSnap;
    bool* noLock;

    bool* silentAim;
    bool* aimOnKey;
    bool* visibleCheck;
    bool* prediction;
    bool* latencyCompensation;
    bool* bulletTravelTime;

    bool* usePistols;
    bool* useRifles;
    bool* useSMGs;
    bool* useSnipers;
    bool* useShotguns;

    int* hitbox;
    int* targetSelection;
    int* hitboxPriority;
    int* onHold;

    Vec3i* reactTime;
    Vec3i* fov;
    Vec3i* triggerDelay;

    Vec3f* maxDistance;
    Vec3f* smoothAmount;
    Vec3f* interpolation;
    Vec3f* randomization;

    AimAssist();
    void renderImGui();

    void onEnable() override {}
    void onDisable() override {}
};
