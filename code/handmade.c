#include "handmade.h"

// TODO(casey): Allow sample offsets here for more robust platform options
internal void GameOutputSound(GameSoundBuffer *sound, f32 waveFrequency) {
  local_persist f32 tSine;
  int toneVolume = 3000;
  f32 wavePeriod = sound->samplesPerSec / waveFrequency;

  i16 *samples = sound->samples;
  for (int frameIndex = 0; frameIndex < sound->outputFramesCount; ++frameIndex) {
    f32 sineValue = sinf(tSine);
    i16 sampleValue = (i16)(sineValue * toneVolume);
    *samples++ = sampleValue;
    *samples++ = sampleValue;

    tSine += 2.0f * PI_32 / wavePeriod;
  }
}

internal void RenderWeirdGradient(GameOffscreenBuffer *screen, int blueOffset, int greenOffset) {
  // TODO: See what optimizer does in value vs reference
  u8 *row = (u8 *)screen->memory;

  for (int y = 0; y < screen->height; ++y) {
    u32 *pixel = (u32 *)row;
    for (int x = 0; x < screen->width; ++x) {
      u8 green = (u8)(x + greenOffset);
      u8 blue = (u8)(y + blueOffset);

      *pixel = (green << 8) + blue;
      pixel += 1;
    }
    row += screen->pitch;
  }
}

void GameUpdateAndRender(
    GameOffscreenBuffer *screen, int blueOffset, int greenOffset, GameSoundBuffer *sound, f32 waveFrequency) {
  // TODO(casey): Allow sample offsets here for more robust platform options
  GameOutputSound(sound, waveFrequency);
  RenderWeirdGradient(screen, blueOffset, greenOffset);
}
