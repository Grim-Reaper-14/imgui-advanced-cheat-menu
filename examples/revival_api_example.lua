local show_panel = true
local pulse = 0.0

function on_load()
    revival.app.log_info("Revival API example loaded (API " .. revival.api_version .. ")")
end

function on_update(dt)
    pulse = pulse + dt
end

function on_render()
    revival.ui.text_colored("Revival Lua API", 0.75, 0.42, 1.0, 1.0)
    revival.ui.text_disabled("API " .. revival.api_version .. " | Revival " .. revival.version)
    revival.ui.separator()

    show_panel = revival.ui.checkbox("Show demo controls", show_panel)
    if not show_panel then
        return
    end

    revival.ui.text("FPS: " .. string.format("%.0f", revival.app.fps()))
    revival.ui.text("Uptime: " .. string.format("%.1f", revival.time.seconds()) .. "s")

    if revival.modules.exists("HUD") then
        local hud_enabled = revival.modules.is_enabled("HUD")
        if revival.ui.button(hud_enabled and "Disable HUD" or "Enable HUD") then
            revival.modules.set_enabled("HUD", not hud_enabled)
        end
    end

    if revival.ui.button("Turn on HUD FPS") then
        revival.settings.set_bool("showFps", "HUD", true)
        revival.settings.set_bool("showWatermark", "HUD", true)
        revival.app.log_info("HUD FPS enabled from Lua")
    end
end

function on_unload()
    revival.app.log_info("Revival API example unloaded")
end
