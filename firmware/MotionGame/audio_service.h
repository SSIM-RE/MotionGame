#pragma once
#include <Arduino.h>
#include "audio_type.h"
void Audio_Play(BuzzerSound sound);
bool Audio_isPlaying(BuzzerSound sound);
bool Audio_Stop(BuzzerSound sound);
