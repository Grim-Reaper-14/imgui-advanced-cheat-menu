#include "BackgroundManager.hpp"

#include "ImageLoader.hpp"
#include "Menu.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <string>

namespace {
    constexpr const char* kBackgroundTextureId = "background.current";

    ImVec4 rgba(int r, int g, int b, int a = 255) {
        return ImVec4(
            static_cast<float>(r) / 255.0f,
            static_cast<float>(g) / 255.0f,
            static_cast<float>(b) / 255.0f,
            static_cast<float>(a) / 255.0f);
    }

    ImVec4 lerpColor(const ImVec4& a, const ImVec4& b, float t) {
        t = std::clamp(t, 0.0f, 1.0f);
        return ImVec4(
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t,
            a.w + (b.w - a.w) * t);
    }

    float fract(float value) {
        return value - std::floor(value);
    }

    float hash01(int value) {
        return fract(std::sin(static_cast<float>(value) * 12.9898f) * 43758.5453f);
    }

    ImTextureID textureId(const sf::Texture& texture) {
        const auto nativeHandle = texture.getNativeHandle();
        static_assert(sizeof(nativeHandle) <= sizeof(ImTextureID), "ImTextureID cannot hold an SFML texture handle");

        ImTextureID id = nullptr;
        std::memcpy(&id, &nativeHandle, sizeof(nativeHandle));
        return id;
    }

    bool isSupportedImage(const std::filesystem::path& path) {
        std::string extension = path.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });

        return extension == ".png" || extension == ".jpg" || extension == ".jpeg" ||
               extension == ".bmp" || extension == ".tga";
    }
}

BackgroundManager::BackgroundManager() {
    style_ = &SetManager::i().add(new Set(static_cast<int>(Style::AnimatedGradient), "backgroundStyle", "Appearance")).getIVal();
    imageFit_ = &SetManager::i().add(new Set(static_cast<int>(ImageFit::Cover), "backgroundImageFit", "Appearance")).getIVal();
    imageIndex_ = &SetManager::i().add(new Set(0, "backgroundImageIndex", "Appearance")).getIVal();

    animate_ = &SetManager::i().add(new Set(true, "animateBackground", "Appearance")).getBVal();
    accentPulse_ = &SetManager::i().add(new Set(true, "accentPulse", "Appearance")).getBVal();
    parallax_ = &SetManager::i().add(new Set(true, "backgroundParallax", "Appearance")).getBVal();

    speed_ = &SetManager::i().add(new Set(0.35f, 0.0f, 2.0f, "backgroundSpeed", "Appearance")).getVec3f();
    intensity_ = &SetManager::i().add(new Set(0.55f, 0.0f, 1.0f, "backgroundIntensity", "Appearance")).getVec3f();
    imageOpacity_ = &SetManager::i().add(new Set(0.78f, 0.0f, 1.0f, "backgroundImageOpacity", "Appearance")).getVec3f();
    imageDarken_ = &SetManager::i().add(new Set(0.28f, 0.0f, 0.90f, "backgroundImageDarken", "Appearance")).getVec3f();

    primary_ = &SetManager::i().add(new Set(rgba(15, 16, 24, 245), "backgroundPrimary", "Appearance")).getVec4();
    secondary_ = &SetManager::i().add(new Set(rgba(75, 25, 88, 220), "backgroundSecondary", "Appearance")).getVec4();
    imageTint_ = &SetManager::i().add(new Set(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "backgroundImageTint", "Appearance")).getVec4();
}

void BackgroundManager::initialize() {
    if (initialized_)
        return;

    backgroundsPath_ = std::filesystem::current_path() / "assets" / "backgrounds";
    std::error_code error;
    std::filesystem::create_directories(backgroundsPath_, error);
    refreshImages();
    initialized_ = true;
}

void BackgroundManager::refreshImages() {
    images_.clear();

    if (backgroundsPath_.empty())
        backgroundsPath_ = std::filesystem::current_path() / "assets" / "backgrounds";

    std::error_code error;
    std::filesystem::create_directories(backgroundsPath_, error);

    for (const auto& entry : std::filesystem::directory_iterator(backgroundsPath_, error)) {
        if (error)
            break;
        if (!entry.is_regular_file())
            continue;
        if (!isSupportedImage(entry.path()))
            continue;
        images_.push_back(entry.path());
    }

    std::sort(images_.begin(), images_.end(), [](const auto& a, const auto& b) {
        return a.filename().string() < b.filename().string();
    });

    if (images_.empty()) {
        *imageIndex_ = 0;
    }
    else {
        *imageIndex_ = std::clamp(*imageIndex_, 0, static_cast<int>(images_.size()) - 1);
    }

    loadedImageIndex_ = -1;
    ImageLoader::i().unload(kBackgroundTextureId);
}

void BackgroundManager::setPalette(const ImVec4& primary, const ImVec4& secondary) {
    *primary_ = primary;
    *secondary_ = secondary;
}

void BackgroundManager::render(const ImVec2& min, const ImVec2& max) {
    initialize();

    if (static_cast<Style>(*style_) == Style::Image)
        renderImage(min, max);
    else
        renderProcedural(min, max);
}

void BackgroundManager::renderProcedural(const ImVec2& min, const ImVec2& max) {
    const auto style = static_cast<Style>(*style_);
    if (style == Style::None)
        return;

    ImDrawList* draw = ImGui::GetWindowDrawList();
    if (!draw)
        return;

    const float intensity = std::clamp(intensity_->x, 0.0f, 1.0f);
    const float speed = *animate_ ? speed_->x : 0.0f;
    const float time = static_cast<float>(ImGui::GetTime()) * speed;

    ImVec4 primary = *primary_;
    ImVec4 secondary = *secondary_;
    primary.w *= intensity;
    secondary.w *= intensity;

    if (style == Style::AnimatedGradient) {
        const float mix = 0.5f + 0.5f * std::sin(time);
        const ImVec4 a = lerpColor(primary, secondary, mix * 0.35f);
        const ImVec4 b = lerpColor(secondary, primary, (1.0f - mix) * 0.25f);
        draw->AddRectFilledMultiColor(
            min, max,
            ImGui::ColorConvertFloat4ToU32(a),
            ImGui::ColorConvertFloat4ToU32(b),
            ImGui::ColorConvertFloat4ToU32(primary),
            ImGui::ColorConvertFloat4ToU32(secondary));
    }
    else if (style == Style::Gradient || style == Style::Grid || style == Style::Particles) {
        draw->AddRectFilledMultiColor(
            min, max,
            ImGui::ColorConvertFloat4ToU32(primary),
            ImGui::ColorConvertFloat4ToU32(secondary),
            ImGui::ColorConvertFloat4ToU32(secondary),
            ImGui::ColorConvertFloat4ToU32(primary));
    }
    else {
        draw->AddRectFilled(min, max, ImGui::ColorConvertFloat4ToU32(primary));
    }

    if (style == Style::Grid) {
        constexpr float spacing = 30.0f;
        const float offset = std::fmod(time * 24.0f, spacing);
        ImVec4 gridColor = *Menu::itemActiveCol;
        gridColor.w = 0.06f + intensity * 0.08f;
        const ImU32 grid = ImGui::ColorConvertFloat4ToU32(gridColor);

        for (float x = min.x - spacing + offset; x < max.x; x += spacing)
            draw->AddLine(ImVec2(x, min.y), ImVec2(x, max.y), grid, 1.0f);
        for (float y = min.y - spacing + offset; y < max.y; y += spacing)
            draw->AddLine(ImVec2(min.x, y), ImVec2(max.x, y), grid, 1.0f);
    }

    if (style == Style::Particles) {
        const float width = max.x - min.x;
        const float height = max.y - min.y;
        ImVec4 particleColor = *Menu::itemActiveCol;
        particleColor.w = 0.10f + intensity * 0.20f;
        const ImU32 color = ImGui::ColorConvertFloat4ToU32(particleColor);

        for (int i = 0; i < 64; ++i) {
            const float x = hash01(i * 17 + 3);
            float y = hash01(i * 31 + 9);
            y = std::fmod(y + time * (0.012f + hash01(i + 70) * 0.018f), 1.0f);

            const float radius = 0.75f + hash01(i * 11 + 2) * 1.75f;
            draw->AddCircleFilled(
                ImVec2(min.x + x * width, min.y + y * height),
                radius,
                color,
                10);
        }
    }

    if (*accentPulse_) {
        const float pulse = 0.5f + 0.5f * std::sin(time * 1.5f);
        ImVec4 glow = *Menu::itemActiveCol;
        glow.w = (0.025f + pulse * 0.035f) * intensity;
        draw->AddCircleFilled(
            ImVec2(max.x - 55.0f, min.y + 48.0f),
            80.0f + pulse * 20.0f,
            ImGui::ColorConvertFloat4ToU32(glow),
            48);
    }
}

void BackgroundManager::ensureSelectedImageLoaded() {
    if (images_.empty())
        return;

    *imageIndex_ = std::clamp(*imageIndex_, 0, static_cast<int>(images_.size()) - 1);
    if (loadedImageIndex_ == *imageIndex_ && ImageLoader::i().contains(kBackgroundTextureId))
        return;

    ImageLoader::i().unload(kBackgroundTextureId);
    if (ImageLoader::i().loadFromFile(kBackgroundTextureId, images_[*imageIndex_], true, true))
        loadedImageIndex_ = *imageIndex_;
    else
        loadedImageIndex_ = -1;
}

void BackgroundManager::renderImage(const ImVec2& min, const ImVec2& max) {
    ensureSelectedImageLoaded();

    sf::Texture* texture = ImageLoader::i().get(kBackgroundTextureId);
    if (!texture)
        return;

    ImDrawList* draw = ImGui::GetWindowDrawList();
    if (!draw)
        return;

    const auto textureSize = texture->getSize();
    if (textureSize.x == 0 || textureSize.y == 0)
        return;

    const float areaWidth = std::max(1.0f, max.x - min.x);
    const float areaHeight = std::max(1.0f, max.y - min.y);
    const float textureWidth = static_cast<float>(textureSize.x);
    const float textureHeight = static_cast<float>(textureSize.y);

    ImVec2 drawMin = min;
    ImVec2 drawMax = max;
    ImVec2 uv0(0.0f, 0.0f);
    ImVec2 uv1(1.0f, 1.0f);

    const auto fit = static_cast<ImageFit>(*imageFit_);
    if (fit == ImageFit::Fit) {
        const float scale = std::min(areaWidth / textureWidth, areaHeight / textureHeight);
        const ImVec2 size(textureWidth * scale, textureHeight * scale);
        drawMin = ImVec2(min.x + (areaWidth - size.x) * 0.5f, min.y + (areaHeight - size.y) * 0.5f);
        drawMax = ImVec2(drawMin.x + size.x, drawMin.y + size.y);
    }
    else if (fit == ImageFit::Cover) {
        const float areaAspect = areaWidth / areaHeight;
        const float textureAspect = textureWidth / textureHeight;

        if (textureAspect > areaAspect) {
            const float visible = areaAspect / textureAspect;
            uv0.x = (1.0f - visible) * 0.5f;
            uv1.x = 1.0f - uv0.x;
        }
        else {
            const float visible = textureAspect / areaAspect;
            uv0.y = (1.0f - visible) * 0.5f;
            uv1.y = 1.0f - uv0.y;
        }

        if (*parallax_) {
            const ImVec2 mouse = ImGui::GetIO().MousePos;
            const float nx = std::clamp((mouse.x - min.x) / areaWidth, 0.0f, 1.0f) - 0.5f;
            const float ny = std::clamp((mouse.y - min.y) / areaHeight, 0.0f, 1.0f) - 0.5f;
            const float shift = 0.018f;

            const float width = uv1.x - uv0.x;
            const float height = uv1.y - uv0.y;
            uv0.x = std::clamp(uv0.x + nx * shift, 0.0f, 1.0f - width);
            uv1.x = uv0.x + width;
            uv0.y = std::clamp(uv0.y + ny * shift, 0.0f, 1.0f - height);
            uv1.y = uv0.y + height;
        }
    }
    else if (fit == ImageFit::Center) {
        const float scale = std::min(1.0f, std::min(areaWidth / textureWidth, areaHeight / textureHeight));
        const ImVec2 size(textureWidth * scale, textureHeight * scale);
        drawMin = ImVec2(min.x + (areaWidth - size.x) * 0.5f, min.y + (areaHeight - size.y) * 0.5f);
        drawMax = ImVec2(drawMin.x + size.x, drawMin.y + size.y);
    }
    else if (fit == ImageFit::Tile) {
        uv1 = ImVec2(areaWidth / textureWidth, areaHeight / textureHeight);
    }

    ImVec4 tint = *imageTint_;
    tint.w *= std::clamp(imageOpacity_->x, 0.0f, 1.0f);
    draw->AddImage(
        textureId(*texture),
        drawMin,
        drawMax,
        uv0,
        uv1,
        ImGui::ColorConvertFloat4ToU32(tint));

    const float darken = std::clamp(imageDarken_->x, 0.0f, 0.90f);
    if (darken > 0.0f)
        draw->AddRectFilled(min, max, IM_COL32(0, 0, 0, static_cast<int>(darken * 255.0f)));
}

void BackgroundManager::renderSettings() {
    initialize();

    static const char* styles[] = {
        "None",
        "Solid",
        "Gradient",
        "Animated Gradient",
        "Grid",
        "Particles",
        "Image"
    };

    static const char* fitModes[] = {
        "Stretch",
        "Fit",
        "Cover",
        "Center",
        "Tile"
    };

    ImGui::TextUnformatted("Background");
    ImGui::SetNextItemWidth(240.0f);
    ImGui::Combo("##background-style", style_, styles, IM_ARRAYSIZE(styles));

    if (static_cast<Style>(*style_) == Style::Image) {
        if (ImGui::Button("Refresh images"))
            refreshImages();

        ImGui::SameLine();
        ImGui::TextDisabled("assets/backgrounds");

        if (images_.empty()) {
            ImGui::TextWrapped("Drop a PNG/JPG/JPEG/BMP/TGA into assets/backgrounds and press Refresh images.");
        }
        else {
            *imageIndex_ = std::clamp(*imageIndex_, 0, static_cast<int>(images_.size()) - 1);
            const std::string selectedName = images_[*imageIndex_].filename().string();
            ImGui::SetNextItemWidth(240.0f);
            if (ImGui::BeginCombo("Image", selectedName.c_str())) {
                for (int i = 0; i < static_cast<int>(images_.size()); ++i) {
                    const bool selected = i == *imageIndex_;
                    const std::string name = images_[i].filename().string();
                    if (ImGui::Selectable(name.c_str(), selected)) {
                        *imageIndex_ = i;
                        loadedImageIndex_ = -1;
                    }
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }

        ImGui::SetNextItemWidth(240.0f);
        ImGui::Combo("Fit", imageFit_, fitModes, IM_ARRAYSIZE(fitModes));
        ImGui::Checkbox("Parallax", parallax_);

        ImGui::SetNextItemWidth(240.0f);
        ImGui::SliderFloat("Opacity", &imageOpacity_->x, imageOpacity_->y, imageOpacity_->z, "%.2f");
        ImGui::SetNextItemWidth(240.0f);
        ImGui::SliderFloat("Darken", &imageDarken_->x, imageDarken_->y, imageDarken_->z, "%.2f");
        ImGui::ColorEdit4("Tint", reinterpret_cast<float*>(imageTint_), ImGuiColorEditFlags_AlphaBar);
        return;
    }

    ImGui::Checkbox("Animate background", animate_);
    ImGui::Checkbox("Accent pulse", accentPulse_);

    ImGui::SetNextItemWidth(240.0f);
    ImGui::SliderFloat("Speed", &speed_->x, speed_->y, speed_->z, "%.2f");
    ImGui::SetNextItemWidth(240.0f);
    ImGui::SliderFloat("Intensity", &intensity_->x, intensity_->y, intensity_->z, "%.2f");
    ImGui::ColorEdit4("Primary", reinterpret_cast<float*>(primary_), ImGuiColorEditFlags_AlphaBar);
    ImGui::ColorEdit4("Secondary", reinterpret_cast<float*>(secondary_), ImGuiColorEditFlags_AlphaBar);
}
