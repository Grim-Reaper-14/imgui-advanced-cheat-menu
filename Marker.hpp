#pragma once

#include "Module.hpp"
#include "imgui.h"
#include "util/Obf.hpp"
#include "util/Singleton.hpp"

class Marker : public Singleton<Marker>, public Module {
    friend class Singleton<Marker>;

public:
    ImVec4* col = nullptr;
    bool* renderBehind = nullptr;
    bool* renderInfront = nullptr;

    Marker();
    void renderImGui();

    void onEnable() override {}
    void onDisable() override {}
};
