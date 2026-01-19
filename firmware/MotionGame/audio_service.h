#pragma once
#include <Arduino.h>
#include "audio_type.h"
void Audio_Play(BuzzerSound sound);
bool Audio_isPlaying(void);
void Audio_Stop(void);
