#pragma once
#include <Arduino.h>
#include "audio_type.h"
/* ================= 硬件配置 ================= */
#define BUZZER_PIN     0
#define BUZZER_BASE_FREQ 2000
#define BUZZER_PWM_RES   8   // 8-bit: 0~255


/* ================= API ================= */
void Buzzer_Init(void);
void Buzzer_Play(const BuzzerSequence*);
void Buzzer_Update(void);
void Buzzer_Stop(void);
bool Buzzer_isPlaying(void);