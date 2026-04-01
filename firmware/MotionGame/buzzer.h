#pragma once
#include <Arduino.h>

/* ================= 硬件配置 ================= */
#define BUZZER_PIN         0
#define BUZZER_BASE_FREQ   2000
#define BUZZER_PWM_RES     8       // 8-bit: 0~255

/* ================= 蜂鸣器 API ================= */
void Buzzer_Init(void);
void Buzzer_PlayTone(uint16_t freq_hz, uint16_t duration_ms, uint8_t volume);
void Buzzer_Stop(void);
void Buzzer_Update(void);
bool Buzzer_isPlaying(void);