# Revival Lua API v1.1

Revival V2 exposes a versioned Lua API through the global `revival` table.

```lua
print(revival.api_version) -- 1.1.0
print(revival.version)     -- 2.1
```

Legacy globals `menu`, `modules`, and `ui` remain available as compatibility aliases.

## Runtime architecture

The scripting subsystem is split into dedicated components:

- `Lua_run_time` - owns the Lua 5.4 + Sol2 state and standard-library policy.
- `Lua_Scripts_Manager` - scans, loads, reloads, unloads and auto-runs `.lua` scripts.
- `Lua_Bindings` - builds the public `revival.*` namespace.
- `Lua_binding_library` - shared typed access to Revival settings.
- `Lua_Module` - safe module lookup, toggle and hotkey access.
- `Lua_Commands` - text-command registry used by Lua and the Scripts-page command console.
- `LuaManager` - compatibility facade used by the rest of the application.

## Lifecycle callbacks

A script can define any of these callbacks:

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

Settings are addressed by setting ID and owning group/module name.

- `exists(id, group)`
- `get_bool(id, group)`
- `set_bool(id, group, value)`
- `get_int(id, group)`
- `set_int(id, group, value)`
- `get_float(id, group)`
- `set_float(id, group, value)`
- `list(group)`

Slider writes are clamped to the setting's configured range.

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

## `revival.commands`

`Lua_Commands` provides a small text-command layer that can be used from Lua or from the Scripts-page command console.

- `revival.commands.run(command)` -> `ok, output`
- `revival.commands.list()` -> table

Initial commands:

```text
help
log <text>
module.toggle <name>
module.set <name> <on|off>
module.key <name> <virtual-key>
```

Example:

```lua
local ok, output = revival.commands.run("module.toggle HUD")
revival.app.log_info(output)
```

## `revival.time`

- `seconds()` - ImGui/application uptime in seconds
- `unix_ms()` - Unix time in milliseconds

## `revival.events`

API 1.1 keeps lifecycle callbacks deterministic while the event bus is developed.

- `supported()` -> `on_load`, `on_update`, `on_render`, `on_unload`
- `has(name)` -> bool

A registered event bus can be added without breaking these lifecycle callbacks.

## Script manager

The Scripts page supports:

- refresh script directory
- load / unload / reload
- reload all currently loaded scripts
- auto-run persistence through `scripts/autoload.txt`
- per-script error reporting
- script-provided `on_render()` UI
- built-in Lua command console

## Script safety

Revival disables Lua `dofile`, `loadfile`, and `require` in the managed state. Scripts should use the public `revival` API instead of reaching directly into application internals.
