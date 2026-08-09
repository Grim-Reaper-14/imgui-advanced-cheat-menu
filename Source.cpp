#include "imgui.h"
#include "imgui-SFML.h"
#include "util/Obf.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics.hpp>

#include "menu/Console.hpp"
#include "menu/Menu.hpp"
#include "menu/imgui_notify.h"
#include "ModuleManager.hpp"
#include "scripting/LuaManager.hpp"

#include <Windows.h>

#include <array>
#include <filesystem>

namespace {
    constexpr unsigned int kWindowWidth = 1180;
    constexpr unsigned int kWindowHeight = 860;

    std::filesystem::path executableDirectory() {
        std::array<wchar_t, 32768> buffer{};
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0 || length >= buffer.size())
            return {};
        return std::filesystem::path(buffer.data(), buffer.data() + length).parent_path();
    }

    std::filesystem::path findProjectRoot(std::filesystem::path base) {
        std::error_code ec;
        for (int depth = 0; depth < 10 && !base.empty(); ++depth) {
            if (std::filesystem::is_directory(base / "assets", ec) && !ec)
                return base;
            ec.clear();
            const auto parent = base.parent_path();
            if (parent == base)
                break;
            base = parent;
        }
        return {};
    }

    void normalizeWorkingDirectory() {
        std::error_code ec;
        const auto cwd = std::filesystem::current_path(ec);
        if (!ec) {
            if (const auto root = findProjectRoot(cwd); !root.empty()) {
                std::filesystem::current_path(root, ec);
                return;
            }
        }

        if (const auto root = findProjectRoot(executableDirectory()); !root.empty())
            std::filesystem::current_path(root, ec);
    }

    void setWindowVisible(sf::RenderWindow& window, bool visible) {
        HWND hwnd = reinterpret_cast<HWND>(window.getSystemHandle());
        if (!hwnd)
            return;

        ShowWindow(hwnd, visible ? SW_SHOW : SW_HIDE);
        if (visible) {
            ShowWindow(hwnd, SW_RESTORE);
            SetForegroundWindow(hwnd);
        }
    }

    void handleHotkeys(sf::RenderWindow& window) {
        if (GetAsyncKeyState(VK_INSERT) & 1) {
            Menu::isGUIVisible = !Menu::isGUIVisible;
            setWindowVisible(window, Menu::isGUIVisible);
        }

        for (Module* module : ModuleManager::i().modules) {
            if (!module)
                continue;

            const int key = module->getKey();
            if (key <= 0)
                continue;

            if (GetAsyncKeyState(key) & 1) {
                module->toggle();

                ImGuiToast toast(ImGuiToastType_Info, 1200);
                toast.set_title((module->getName() + (module->isToggled() ? " enabled" : " disabled")).c_str());
                ImGui::InsertNotification(toast);
            }
        }
    }

    void renderNotifications() {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(43.0f / 255.0f, 43.0f / 255.0f, 43.0f / 255.0f, 0.82f));
        ImGui::RenderNotifications();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    normalizeWorkingDirectory();

    sf::RenderWindow window(
        sf::VideoMode(kWindowWidth, kWindowHeight),
        obf("Revival V2"),
        sf::Style::Titlebar | sf::Style::Close);

    window.setFramerateLimit(144);
    ImGui::SFML::Init(window);

    Menu::loadTheme();
    LuaManager::i().initialize();
    Console::i().logInfo(obf("Revival V2 standalone UI initialized"));

    sf::Clock deltaClock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(window, event);
            if (event.type == sf::Event::Closed)
                window.close();
        }

        handleHotkeys(window);

        if (!Menu::isGUIVisible) {
            Sleep(16);
            deltaClock.restart();
            continue;
        }

        const sf::Time frameTime = deltaClock.restart();
        ImGui::SFML::Update(window, frameTime);

        LuaManager::i().update(frameTime.asSeconds());

        Menu::render();
        Console::i().render();
        renderNotifications();

        window.clear(sf::Color(5, 7, 13, 255));
        ImGui::SFML::Render(window);
        window.display();
    }

    LuaManager::i().shutdown();
    ImGui::SFML::Shutdown();
    return 0;
}

// Validate the corrected Revival V2 header asset on Windows CI.
