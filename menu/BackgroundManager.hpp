#pragma once

#include "../SetManager.hpp"
#include "../util/Singleton.hpp"
#include "imgui.h"

#include <filesystem>
#include <vector>

class BackgroundManager : public Singleton<BackgroundManager> {
    friend class Singleton<BackgroundManager>;

public:
    enum class Style : int {
        None = 0,
        Solid,
        Gradient,
        AnimatedGradient,
        Grid,
        Particles,
        Image
    };

    enum class ImageFit : int {
        Stretch = 0,
        Fit,
        Cover,
        Center,
        Tile
    };

    void initialize();
    void refreshImages();
    void render(const ImVec2& min, const ImVec2& max);
    void renderSettings();

    void setPalette(const ImVec4& primary, const ImVec4& secondary);

private:
    BackgroundManager();

    void renderProcedural(const ImVec2& min, const ImVec2& max);
    void renderImage(const ImVec2& min, const ImVec2& max);
    void ensureSelectedImageLoaded();

    std::filesystem::path backgroundsPath_;
    std::vector<std::filesystem::path> images_;
    int loadedImageIndex_ = -1;
    bool initialized_ = false;

    int* style_ = nullptr;
    int* imageFit_ = nullptr;
    int* imageIndex_ = nullptr;

    bool* animate_ = nullptr;
    bool* accentPulse_ = nullptr;
    bool* parallax_ = nullptr;

    Vec3f* speed_ = nullptr;
    Vec3f* intensity_ = nullptr;
    Vec3f* imageOpacity_ = nullptr;
    Vec3f* imageDarken_ = nullptr;

    ImVec4* primary_ = nullptr;
    ImVec4* secondary_ = nullptr;
    ImVec4* imageTint_ = nullptr;
};
