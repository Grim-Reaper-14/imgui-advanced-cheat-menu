#include "ImageLoader.hpp"

#include "../resources/RevivalHeaderData.hpp"

#include <SFML/Graphics/Image.hpp>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace {
    constexpr const char* kRevivalHeaderId = "revival.header";

    int base64Value(char value) {
        if (value >= 'A' && value <= 'Z')
            return value - 'A';
        if (value >= 'a' && value <= 'z')
            return value - 'a' + 26;
        if (value >= '0' && value <= '9')
            return value - '0' + 52;
        if (value == '+')
            return 62;
        if (value == '/')
            return 63;
        return -1;
    }

    std::vector<unsigned char> decodeRevivalHeader() {
        std::vector<unsigned char> decoded;
        decoded.reserve(64000);

        std::uint32_t accumulator = 0;
        int bits = -8;

        for (const char* part : RevivalHeaderData::Parts) {
            for (const char* cursor = part; *cursor != '\0'; ++cursor) {
                if (*cursor == '=')
                    return decoded;

                const int value = base64Value(*cursor);
                if (value < 0)
                    continue;

                accumulator = (accumulator << 6) | static_cast<std::uint32_t>(value);
                bits += 6;
                if (bits >= 0) {
                    decoded.push_back(static_cast<unsigned char>((accumulator >> bits) & 0xffu));
                    bits -= 8;
                }
            }
        }

        return decoded;
    }

    const std::vector<unsigned char>& revivalHeaderBytes() {
        static const std::vector<unsigned char> bytes = decodeRevivalHeader();
        return bytes;
    }

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

    bool loadTextureFromMemory(
        std::unique_ptr<sf::Texture>& texture,
        const void* data,
        std::size_t size,
        bool smooth,
        bool repeated) {
        if (data == nullptr || size == 0)
            return false;

        texture = std::make_unique<sf::Texture>();
        if (!texture->loadFromMemory(data, size))
            return false;

        texture->setSmooth(smooth);
        texture->setRepeated(repeated);
        return true;
    }
}

bool ImageLoader::loadFromFile(
    const std::string& id,
    const std::filesystem::path& path,
    bool smooth,
    bool repeated) {
    if (id.empty())
        return false;

    auto texture = std::make_unique<sf::Texture>();

    if (id == kRevivalHeaderId) {
        const auto& bytes = revivalHeaderBytes();
        if (!loadTextureFromMemory(texture, bytes.data(), bytes.size(), smooth, repeated))
            return false;
    }
    else {
        if (path.empty() || !texture->loadFromFile(path.string()))
            return false;

        if (shouldPurpleTint(id))
            applyPurpleTint(*texture);

        texture->setSmooth(smooth);
        texture->setRepeated(repeated);
    }

    textures_[id] = std::move(texture);
    return true;
}

bool ImageLoader::loadFromMemory(
    const std::string& id,
    const void* data,
    std::size_t size,
    bool smooth,
    bool repeated) {
    if (id.empty())
        return false;

    auto texture = std::make_unique<sf::Texture>();

    if (id == kRevivalHeaderId) {
        const auto& bytes = revivalHeaderBytes();
        if (!loadTextureFromMemory(texture, bytes.data(), bytes.size(), smooth, repeated))
            return false;
    }
    else {
        if (!loadTextureFromMemory(texture, data, size, smooth, repeated))
            return false;

        if (shouldPurpleTint(id))
            applyPurpleTint(*texture);
    }

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
