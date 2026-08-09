#include "ImageLoader.hpp"

#include "../resources/RevivalHeaderData.hpp"

#include <string>
#include <vector>

namespace {
    int base64Value(unsigned char c) {
        if (c >= 'A' && c <= 'Z')
            return c - 'A';
        if (c >= 'a' && c <= 'z')
            return c - 'a' + 26;
        if (c >= '0' && c <= '9')
            return c - '0' + 52;
        if (c == '+')
            return 62;
        if (c == '/')
            return 63;
        return -1;
    }

    std::vector<unsigned char> decodeVerifiedRevivalHeader() {
        std::string encoded;
        std::size_t totalSize = 0;
        for (std::size_t i = 0; i < RevivalHeaderData::PartCount; ++i)
            totalSize += std::char_traits<char>::length(RevivalHeaderData::Parts[i]);

        encoded.reserve(totalSize);
        for (std::size_t i = 0; i < RevivalHeaderData::PartCount; ++i)
            encoded += RevivalHeaderData::Parts[i];

        std::vector<unsigned char> decoded;
        decoded.reserve((encoded.size() * 3U) / 4U);

        unsigned int buffer = 0;
        int bits = -8;
        for (unsigned char c : encoded) {
            if (c == '=')
                break;

            const int value = base64Value(c);
            if (value < 0)
                continue;

            buffer = (buffer << 6U) | static_cast<unsigned int>(value);
            bits += 6;
            if (bits >= 0) {
                decoded.push_back(static_cast<unsigned char>((buffer >> bits) & 0xFFU));
                bits -= 8;
            }
        }

        return decoded;
    }

    const std::vector<unsigned char>& verifiedRevivalHeaderBytes() {
        static const std::vector<unsigned char> bytes = decodeVerifiedRevivalHeader();
        return bytes;
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

    texture->setSmooth(smooth);
    texture->setRepeated(repeated);
    textures_[id] = std::move(texture);
    return true;
}

sf::Texture* ImageLoader::get(const std::string& id) {
    // The original GitHub binary for the banner was corrupted during upload.
    // Always install the verified artwork bytes the first time this texture is
    // requested, overriding any bad RCDATA/disk version that may have loaded.
    if (id == "revival.header") {
        static sf::Texture* verifiedTexture = nullptr;
        const auto current = textures_.find(id);

        if (current == textures_.end() || current->second.get() != verifiedTexture) {
            const auto& bytes = verifiedRevivalHeaderBytes();
            if (!bytes.empty() && loadFromMemory(id, bytes.data(), bytes.size(), true, false))
                verifiedTexture = textures_[id].get();
        }
    }

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
