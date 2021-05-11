#include "handmade.h"

internal void RenderWeirdGradient(GameOffscreenBuffer *buffer, int blueOffset, int greenOffset) {
  // TODO: See what optimizer does in value vs reference
  u8 *row = (u8 *)buffer->memory;

  for (int y = 0; y < buffer->height; ++y) {
    u32 *pixel = (u32 *)row;
    for (int x = 0; x < buffer->width; ++x) {
      u8 green = (u8)(x + greenOffset);
      u8 blue = (u8)(y + blueOffset);

      *pixel = (green << 8) + blue;
      pixel += 1;
    }
    row += buffer->pitch;
  }
}

void GameUpdateAndRender(GameOffscreenBuffer *buffer, int blueOffset, int greenOffset) {
  RenderWeirdGradient(buffer, blueOffset, greenOffset);
}
