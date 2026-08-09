# Revival Lua API v1.0

Revival V2 exposes a versioned Lua API through the global `revival` table.

```lua
print(revival.api_version) -- 1.0.0
print(revival.version)     -- 2.1
```

Legacy globals `menu`, `modules`, and `ui` remain available as compatibility aliases.

## Lifecycle callbacks

A script can define any of these global callbacks:

```lua
function on_load()
end

function on_update(dt)
end

function on_render()
end

function on_unload()
end
```

`on_update(dt)` receives frame delta time in seconds. `on_render()` runs while the script is loaded and the Scripts page is rendering.

## `revival.app`

- `revival.app.name`
- `revival.app.version`
- `revival.app.api_version`
- `revival.app.log(text)`
- `revival.app.log_info(text)`
- `revival.app.log_error(text)`
- `revival.app.fps()`
- `revival.app.scripts_path()`
- `revival.app.open_scripts_folder()`

## `revival.modules`

- `exists(name)`
- `is_enabled(name)`
- `set_enabled(name, enabled)`
- `toggle(name)`
- `get_key(name)`
- `set_key(name, virtual_key)`
- `list()`

Example:

```lua
if revival.modules.exists("HUD") then
    revival.modules.set_enabled("HUD", true)
end
```

## `revival.settings`

Settings are addressed by their setting ID and owning group/module name.

- `exists(id, group)`
- `get_bool(id, group)`
- `set_bool(id, group, value)`
- `get_int(id, group)`
- `set_int(id, group, value)`
- `get_float(id, group)`
- `set_float(id, group, value)`
- `list(group)`

Slider writes are clamped to the setting's configured range.

Example:

```lua
revival.settings.set_bool("showFps", "HUD", true)
revival.settings.set_float("hudAlpha", "HUD", 0.85)
```

## `revival.ui`

These helpers are intended for `on_render()`:

- `text(text)`
- `text_disabled(text)`
- `text_colored(text, r, g, b, a)`
- `separator()`
- `spacing()`
- `same_line()`
- `button(label)` -> bool
- `checkbox(label, value)` -> updated bool
- `slider_int(label, value, min, max)` -> updated int
- `slider_float(label, value, min, max)` -> updated float
- `collapsing_header(label)` -> bool

Example:

```lua
local enabled = true

function on_render()
    revival.ui.text("My Revival script")
    enabled = revival.ui.checkbox("Enabled", enabled)
end
```

## `revival.time`

- `seconds()` - ImGui/application uptime in seconds
- `unix_ms()` - Unix time in milliseconds

## `revival.events`

Revival v1 uses lifecycle callback names rather than arbitrary event registration.

- `supported()` -> table containing `on_load`, `on_update`, `on_render`, `on_unload`
- `has(name)` -> bool

This keeps v1 deterministic while leaving room for a registered event bus in a later API revision.

## Script safety

Revival disables Lua `dofile`, `loadfile`, and `require` in the managed script state. Scripts should use the exposed `revival` API rather than reaching into application internals.
