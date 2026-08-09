#pragma once

#include "../SetManager.hpp"
#include "../util/Singleton.hpp"
#include "imgui.h"

class ThemeManager : public Singleton<ThemeManager> {
    friend class Singleton<ThemeManager>;

public:
    void applyPreset(int preset);
    void applyLiveStyle();
    void renderMenu();

    int currentPreset() const;

private:
    ThemeManager();

    int* preset_ = nullptr;
    Vec3f* windowRounding_ = nullptr;
    Vec3f* childRounding_ = nullptr;
    Vec3f* frameRounding_ = nullptr;
    Vec3f* uiScale_ = nullptr;
};
