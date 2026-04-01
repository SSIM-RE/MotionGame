#include "buzzer.h"

/* ================= 工具函数 ================= */
static uint8_t volume_to_duty(uint8_t volume) {
    if (volume > 100) volume = 100;
    return map(volume, 0, 100, 0, 255);
}

/* ================= 播放状态 ================= */
static uint32_t buzzer_deadline = 0;
static bool playing = false;

/* ================= 初始化 ================= */
void Buzzer_Init(void) {
    ledcAttach(BUZZER_PIN, BUZZER_BASE_FREQ, BUZZER_PWM_RES);
    ledcWrite(BUZZER_PIN, 0);
}

/* ================= 播放单音调 ================= */
void Buzzer_PlayTone(uint16_t freq_hz, uint16_t duration_ms, uint8_t volume) {
    if (freq_hz == 0 || volume == 0) {
        Buzzer_Stop();
        return;
    }
    
    playing = true;
    ledcWriteTone(BUZZER_PIN, freq_hz);
    ledcWrite(BUZZER_PIN, volume_to_duty(volume));
    buzzer_deadline = millis() + duration_ms;
}

/* ================= 停止 ================= */
void Buzzer_Stop(void) {
    ledcWrite(BUZZER_PIN, 0);
    playing = false;
}

/* ================= 非阻塞更新 ================= */
void Buzzer_Update(void) {
    if (!playing) return;
    
    if (millis() >= buzzer_deadline) {
        Buzzer_Stop();
    }
}

/* ================= 查询状态 ================= */
bool Buzzer_isPlaying(void) {
    return playing;
}