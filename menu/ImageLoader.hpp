#pragma once

#include "../util/Singleton.hpp"

#include <SFML/Graphics/Texture.hpp>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

class ImageLoader : public Singleton<ImageLoader> {
    friend class Singleton<ImageLoader>;

public:
    bool loadFromFile(
        const std::string& id,
        const std::filesystem::path& path,
        bool smooth = true,
        bool repeated = false);

    bool loadFromMemory(
        const std::string& id,
        const void* data,
        std::size_t size,
        bool smooth = true,
        bool repeated = false);

    sf::Texture* get(const std::string& id);
    const sf::Texture* get(const std::string& id) const;

    bool contains(const std::string& id) const;
    void unload(const std::string& id);
    void clear();

private:
    ImageLoader() = default;

    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> textures_;
};
