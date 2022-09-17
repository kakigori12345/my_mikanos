#pragma once

#include "graphics.hpp"

extern unsigned int mouse_layer_id;

void InitializeMouse(PixelFormat pixel_format);
void DrawMouseCursor(PixelWriter* pixel_writer, Vector2D<int> position);
