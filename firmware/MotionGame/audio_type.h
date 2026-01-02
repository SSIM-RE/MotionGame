#pragma once
typedef struct {
    uint16_t freq;      // Hz
    uint8_t  volume;    // 0~100
    uint16_t duration;  // ms
} BuzzerNote;

typedef struct {
    const BuzzerNote* notes;
    uint8_t count;
} BuzzerSequence;


/* ================= 音阶定义 ================= */
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494

#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880

#define NOTE_REST 0



/* ================= 音效类型 ================= */
typedef enum {
    BUZZER_ATTACK = 0,
    BUZZER_HIT,
    BUZZER_STARTUP,
    BUZZER_SOUND_COUNT
} BuzzerSound;


