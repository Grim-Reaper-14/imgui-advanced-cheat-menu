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
    int* statusCorner = nullptr;

    ImVec4* colML = nullptr;
    ImVec4* colML_Bg = nullptr;
    ImVec4* statusAccent = nullptr;
    ImVec4* statusBg = nullptr;

    Vec3f* speedML = nullptr;
    Vec3f* offsetML = nullptr;
    Vec3f* rangeML = nullptr;
    Vec3f* hudScale = nullptr;
    Vec3f* hudAlpha = nullptr;

    bool* isML = nullptr;
    bool* isMLRainbow = nullptr;
    bool* showWatermark = nullptr;
    bool* showFps = nullptr;
    bool* showClock = nullptr;
    bool* showLuaStatus = nullptr;
    bool* showAccentLine = nullptr;
    bool* compactStatus = nullptr;

    HUD();

    void renderImGui();
    void render();
    void renderML();
    void renderStatus();

    void onEnable() override {}
    void onDisable() override {}
};
