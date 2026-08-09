#include "ImageLoader.hpp"

#include <SFML/Graphics/Image.hpp>

#include <algorithm>
#include <string>

namespace {
    bool shouldPurpleTint(const std::string& id) {
        return id == "revival.scripts" || id == "revival.themes";
    }

    void applyPurpleTint(sf::Texture& texture) {
        sf::Image image = texture.copyToImage();
        const sf::Vector2u size = image.getSize();

        for (unsigned int y = 0; y < size.y; ++y) {
            for (unsigned int x = 0; x < size.x; ++x) {
                sf::Color pixel = image.getPixel(x, y);
                if (pixel.a == 0)
                    continue;

                const float luminance =
                    (0.2126f * static_cast<float>(pixel.r) +
                     0.7152f * static_cast<float>(pixel.g) +
                     0.0722f * static_cast<float>(pixel.b)) /
                    255.0f;

                const float strength = luminance < 0.10f
                    ? 0.10f
                    : std::clamp(0.42f + luminance * 0.32f, 0.42f, 0.74f);

                const float keepOriginal = luminance > 0.82f ? 0.34f : 0.08f;
                const float purpleMix = strength * (1.0f - keepOriginal);

                const float targetR = 174.0f;
                const float targetG = 72.0f;
                const float targetB = 255.0f;

                pixel.r = static_cast<sf::Uint8>(std::clamp(
                    static_cast<float>(pixel.r) * (1.0f - purpleMix) + targetR * purpleMix,
                    0.0f,
                    255.0f));
                pixel.g = static_cast<sf::Uint8>(std::clamp(
                    static_cast<float>(pixel.g) * (1.0f - purpleMix) + targetG * purpleMix,
                    0.0f,
                    255.0f));
                pixel.b = static_cast<sf::Uint8>(std::clamp(
                    static_cast<float>(pixel.b) * (1.0f - purpleMix) + targetB * purpleMix,
                    0.0f,
                    255.0f));

                image.setPixel(x, y, pixel);
            }
        }

        texture.loadFromImage(image);
    }
}

bool ImageLoader::loadFromFile(
    const std::string& id,
    const std::filesystem::path& path,
    bool smooth,
    bool repeated) {
    if (id.empty() || path.empty())
        return false;

    auto texture = std::make_unique<sf::Texture>();
    if (!texture->loadFromFile(path.string()))
        return false;

    if (shouldPurpleTint(id))
        applyPurpleTint(*texture);

    texture->setSmooth(smooth);
    texture->setRepeated(repeated);
    textures_[id] = std::move(texture);
    return true;
}

bool ImageLoader::loadFromMemory(
    const std::string& id,
    const void* data,
    std::size_t size,
    bool smooth,
    bool repeated) {
    if (id.empty() || data == nullptr || size == 0)
        return false;

    auto texture = std::make_unique<sf::Texture>();
    if (!texture->loadFromMemory(data, size))
        return false;

    if (shouldPurpleTint(id))
        applyPurpleTint(*texture);

    texture->setSmooth(smooth);
    texture->setRepeated(repeated);
    textures_[id] = std::move(texture);
    return true;
}

sf::Texture* ImageLoader::get(const std::string& id) {
    const auto it = textures_.find(id);
    return it == textures_.end() ? nullptr : it->second.get();
}

const sf::Texture* ImageLoader::get(const std::string& id) const {
    const auto it = textures_.find(id);
    return it == textures_.end() ? nullptr : it->second.get();
}

bool ImageLoader::contains(const std::string& id) const {
    return textures_.find(id) != textures_.end();
}

void ImageLoader::unload(const std::string& id) {
    textures_.erase(id);
}

void ImageLoader::clear() {
    textures_.clear();
}
