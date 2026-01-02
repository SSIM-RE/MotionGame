#include "buzzer.h"




/* ================= 工具函数 ================= */
static uint8_t volume_to_duty(uint8_t volume)
{
    if (volume > 100) volume = 100;
    return map(volume, 0, 100, 0, 255);
}

/* ================= 音效资源 ================= */

/* ================= 播放状态 ================= */
static const BuzzerSequence* current_seq = nullptr;
static uint8_t note_index = 0;
static uint32_t note_deadline = 0;
static bool playing = false;

/* ================= 初始化 ================= */
void Buzzer_Init(void)
{
    /* Arduino-ESP32 3.x 正确方式 */
    ledcAttach(BUZZER_PIN, BUZZER_BASE_FREQ, BUZZER_PWM_RES);
    ledcWrite(BUZZER_PIN, 0);   // 静音
}

/* ================= 停止 ================= */
void Buzzer_Stop(void)
{
    ledcWrite(BUZZER_PIN, 0);
    playing = false;
}

/* ================= 触发音效 ================= */
void Buzzer_Play(const BuzzerSequence* sound)
{
    current_seq = sound;
    note_index = 0;
    playing = true;

    const BuzzerNote* n = &current_seq->notes[0];

    if (n->freq == NOTE_REST) {
        ledcWrite(BUZZER_PIN, 0);
    } else {
        ledcWriteTone(BUZZER_PIN, n->freq);
        ledcWrite(BUZZER_PIN, volume_to_duty(n->volume));
    }

    note_deadline = millis() + n->duration;
}

/* ================= 非阻塞更新 ================= */
void Buzzer_Update(void)
{
    if (!playing || !current_seq) return;
    if (millis() < note_deadline) return;

    note_index++;

    if (note_index >= current_seq->count) {
        Buzzer_Stop();
        return;
    }

    const BuzzerNote* n = &current_seq->notes[note_index];

    if (n->freq == NOTE_REST) {
        ledcWrite(BUZZER_PIN, 0);
    } else {
        ledcWriteTone(BUZZER_PIN, n->freq);
        ledcWrite(BUZZER_PIN, volume_to_duty(n->volume));
    }

    note_deadline = millis() + n->duration;
}
bool Buzzer_isPlaying(void)
{
    return playing;
}