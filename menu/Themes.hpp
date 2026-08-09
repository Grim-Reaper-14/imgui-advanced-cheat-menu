#pragma once

#include "ThemeManager.hpp"

// Revival V2 compatibility name. ThemeManager remains the implementation;
// Themes provides the expected public filename/type without duplicating state.
using Themes = ThemeManager;
