#pragma once

#include "util/Singleton.hpp"

#include <string>
#include <vector>

class Config : public Singleton<Config> {
    friend class Singleton<Config>;

public:
    Config();

    void checkCfgs();
    void renderImGui();

    bool save();
    bool save(const std::string& filePath);
    bool load();
    bool load(const std::string& filePath);

private:
    std::vector<std::string> cfgs_;
};
