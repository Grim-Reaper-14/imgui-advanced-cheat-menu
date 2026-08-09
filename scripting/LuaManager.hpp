#pragma once

#include "../util/Singleton.hpp"

#include <memory>

class LuaManager : public Singleton<LuaManager> {
    friend class Singleton<LuaManager>;

public:
    void initialize();
    void shutdown();
    void update(float deltaSeconds);
    void renderMenu();

    bool isAvailable() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;

    LuaManager();
    ~LuaManager();
};
