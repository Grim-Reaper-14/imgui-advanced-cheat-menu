local enabled = true
local strength = 0.50
local count = 4

function on_load()
    menu.log_info("example.lua loaded")
end

function on_update(dt)
    -- Per-frame logic goes here. Keep expensive work out of this callback.
end

function on_render()
    ui.text("Hello from Lua")
    ui.separator()

    enabled = ui.checkbox("Example enabled", enabled)
    strength = ui.slider_float("Strength", strength, 0.0, 1.0)
    count = ui.slider_int("Count", count, 1, 10)

    if ui.button("Print current state") then
        menu.log_info("enabled=" .. tostring(enabled) .. ", strength=" .. tostring(strength) .. ", count=" .. tostring(count))
    end

    ui.spacing()
    ui.text("Existing modules can be queried/toggled too:")
    if modules.exists("HUD") then
        local hud = modules.is_enabled("HUD")
        hud = ui.checkbox("HUD module", hud)
        modules.set_enabled("HUD", hud)
    end
end

function on_unload()
    menu.log_info("example.lua unloaded")
end
