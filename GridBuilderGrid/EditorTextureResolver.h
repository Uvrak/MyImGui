#pragma once
#include <imgui.h>
#include <functional>
#include <string>

// The host owns returned textures and keeps them alive while the grid is used.
using EditorTextureResolver = std::function<ImTextureID(const std::string&, int, int)>;
