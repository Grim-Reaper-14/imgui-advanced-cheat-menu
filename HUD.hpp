#pragma once

#include "Module.hpp"
#include "imgui.h"
#include "util/Obf.hpp"
#include "util/Singleton.hpp"

class HUD : public Singleton<HUD>, public Module {
    friend class Singleton<HUD>;

public:
    int* alignML = nullptr;
    int* sortML = nullptr;

    ImVec4* colML = nullptr;
    ImVec4* colML_Bg = nullptr;

    Vec3f* speedML = nullptr;
    Vec3f* offsetML = nullptr;
    Vec3f* rangeML = nullptr;

    bool* isML = nullptr;
    bool* isMLRainbow = nullptr;

    HUD();

    void renderImGui();
    void render();
    void renderML();

    void onEnable() override {}
    void onDisable() override {}
};
