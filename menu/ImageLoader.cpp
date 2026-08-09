#include "ImageLoader.hpp"

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
