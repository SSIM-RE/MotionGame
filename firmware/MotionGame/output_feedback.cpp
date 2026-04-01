#include "output_feedback.h"
#include "motor.h"
#include "buzzer.h"
#include <Arduino.h>

/* ================= 内部函数前向声明 ================= */
static void play_note(const BuzzerSequence* seq, uint8_t index);

/* ================= 马达多段波形定义 ================= */

static const MotorWaveform MOTOR_WAVE_RUMBLE = {
    .name = "RUMBLE",
    .segments = 5,
    .data = {
        { 80, 100},
        {160, 100},
        {255, 150},
        {160, 100},
        { 80, 100},
    }
};

static const MotorWaveform MOTOR_WAVE_EAT = {
    .name = "EAT",
    .segments = 2,
    .data = {
        {192, 30},
        { 96, 20},
    }
};

static const MotorWaveform MOTOR_WAVE_HIT = {
    .name = "HIT",
    .segments = 1,
    .data = {
        {200, 80},
    }
};

static const MotorWaveform MOTOR_WAVE_DIE = {
    .name = "DIE",
    .segments = 4,
    .data = {
        {255, 40},
        {  0, 80},
        {255, 40},
        {  0, 60},
    }
};

static const MotorWaveform MOTOR_WAVE_LEVELUP = {
    .name = "LEVELUP",
    .segments = 6,
    .data = {
        {192, 50},
        {  0, 60},
        {192, 50},
        {  0, 60},
        {192, 50},
        {  0, 50},
    }
};

static const MotorWaveform MOTOR_WAVE_SUCCESS = {
    .name = "SUCCESS",
    .segments = 3,
    .data = {
        {128, 50},
        {255, 80},
        { 96, 40},
    }
};

static const MotorWaveform MOTOR_WAVE_ERROR = {
    .name = "ERROR",
    .segments = 6,
    .data = {
        {255, 30},
        {  0, 40},
        {255, 30},
        {  0, 40},
        {255, 30},
        {  0, 50},
    }
};

/* ================= 蜂鸣器序列定义 ================= */

static const BuzzerSequence BUZZER_SEQ_EAT = {
    .name = "EAT",
    .count = 1,
    .notes = {
        {1200, 40, 60},
    }
};

static const BuzzerSequence BUZZER_SEQ_HIT = {
    .name = "HIT",
    .count = 1,
    .notes = {
        {800, 100, 80},
    }
};

static const BuzzerSequence BUZZER_SEQ_DIE = {
    .name = "DIE",
    .count = 2,
    .notes = {
        {400, 40, 80},
        {400, 40, 80},
    }
};

static const BuzzerSequence BUZZER_SEQ_LEVELUP = {
    .name = "LEVELUP",
    .count = 3,
    .notes = {
        {1200, 50, 70},
        {1400, 50, 70},
        {1600, 80, 80},
    }
};

static const BuzzerSequence BUZZER_SEQ_SUCCESS = {
    .name = "SUCCESS",
    .count = 2,
    .notes = {
        {1200, 80, 80},
        {1500, 120, 90},
    }
};

static const BuzzerSequence BUZZER_SEQ_ERROR = {
    .name = "ERROR",
    .count = 3,
    .notes = {
        {300, 30, 80},
        {300, 30, 80},
        {300, 30, 80},
    }
};

static const BuzzerSequence BUZZER_SEQ_WARNING = {
    .name = "WARNING",
    .count = 2,
    .notes = {
        {1000, 40, 80},
        {1000, 40, 80},
    }
};

static const BuzzerSequence BUZZER_SEQ_GAME_OVER = {
    .name = "GAME_OVER",
    .count = 3,
    .notes = {
        {600, 100, 70},
        {400, 100, 70},
        {200, 200, 70},
    }
};

static const BuzzerSequence BUZZER_SEQ_CLICK = {
    .name = "CLICK",
    .count = 1,
    .notes = {
        {1200, 15, 50},
    }
};

static const BuzzerSequence BUZZER_SEQ_SELECT = {
    .name = "SELECT",
    .count = 1,
    .notes = {
        {1000, 40, 60},
    }
};

static const BuzzerSequence BUZZER_SEQ_BACK = {
    .name = "BACK",
    .count = 1,
    .notes = {
        {600, 60, 60},
    }
};

static const BuzzerSequence BUZZER_SEQ_BEEP = {
    .name = "BEEP",
    .count = 1,
    .notes = {
        {1000, 100, 80},
    }
};

static const BuzzerSequence BUZZER_SEQ_POWER_ON = {
    .name = "POWER_ON",
    .count = 2,
    .notes = {
        {1000, 80, 70},
        {1500, 120, 80},
    }
};

static const BuzzerSequence BUZZER_SEQ_POWER_OFF = {
    .name = "POWER_OFF",
    .count = 2,
    .notes = {
        {1200, 100, 70},
        { 800, 150, 60},
    }
};

static const BuzzerSequence BUZZER_SEQ_COUNTDOWN = {
    .name = "COUNTDOWN",
    .count = 3,
    .notes = {
        { 800, 100, 70},
        { 800, 100, 70},
        {1200, 150, 80},
    }
};

/* ================= 事件到配置映射表 ================= */

typedef struct {
    const MotorWaveform* motor_wave;
    const BuzzerSequence* buzzer_seq;
    bool motor_enable;
    bool buzzer_enable;
} FeedbackMapping;

static const FeedbackMapping feedback_map[] = {
    [FEEDBACK_EAT]     = {&MOTOR_WAVE_EAT,     &BUZZER_SEQ_EAT,     true,   true},
    [FEEDBACK_HIT]     = {&MOTOR_WAVE_HIT,     &BUZZER_SEQ_HIT,     true,   true},
    [FEEDBACK_DIE]     = {&MOTOR_WAVE_DIE,     &BUZZER_SEQ_DIE,     true,   true},
    [FEEDBACK_LEVEL_UP]= {&MOTOR_WAVE_LEVELUP, &BUZZER_SEQ_LEVELUP, true,   true},
    [FEEDBACK_SUCCESS] = {&MOTOR_WAVE_SUCCESS,  &BUZZER_SEQ_SUCCESS, true,   true},
    [FEEDBACK_ERROR]   = {&MOTOR_WAVE_ERROR,    &BUZZER_SEQ_ERROR,   true,   true},
    [FEEDBACK_WARNING] = {NULL,                 &BUZZER_SEQ_WARNING, true,   true},
    [FEEDBACK_GAME_OVER]={NULL,                 &BUZZER_SEQ_GAME_OVER,true,  true},
    [FEEDBACK_CLICK]   = {NULL,                 &BUZZER_SEQ_CLICK,   true,   true},
    [FEEDBACK_SELECT]  = {NULL,                 &BUZZER_SEQ_SELECT,  true,   true},
    [FEEDBACK_BACK]    = {NULL,                 &BUZZER_SEQ_BACK,    true,   true},
    [FEEDBACK_RUMBLE]  = {&MOTOR_WAVE_RUMBLE,   NULL,                true,   false},
    [FEEDBACK_TICK]    = {NULL,                 NULL,                true,   false},
    [FEEDBACK_BEEP]    = {NULL,                 &BUZZER_SEQ_BEEP,    false,  true},
    [FEEDBACK_POWER_ON]= {NULL,                 &BUZZER_SEQ_POWER_ON, false,  true},
    [FEEDBACK_POWER_OFF]={NULL,                &BUZZER_SEQ_POWER_OFF,false, true},
    [FEEDBACK_MENU_MOVE]={NULL,                &BUZZER_SEQ_CLICK,   false,  true},
    [FEEDBACK_COUNTDOWN]={NULL,                &BUZZER_SEQ_COUNTDOWN,false, true},
    [FEEDBACK_NONE]    = {NULL,                 NULL,                false,  false},
};

/* ================= 马达状态 ================= */
static const MotorWaveform* current_motor_wave = NULL;
static uint8_t motor_segment_index = 0;
static uint32_t motor_segment_deadline = 0;
static bool motor_wave_active = false;

/* ================= 蜂鸣器状态 ================= */
static const BuzzerSequence* current_buzzer_seq = NULL;
static uint8_t buzzer_note_index = 0;
static uint32_t buzzer_note_deadline = 0;
static bool buzzer_seq_active = false;

/* ================= 使能开关 ================= */
static bool motor_enabled = true;
static bool buzzer_enabled = true;

/* ================= 马达波形播放 ================= */
static void motor_wave_start(const MotorWaveform* wave) {
    if (wave == NULL || wave->segments == 0) return;
    if (!motor_enabled) return;
    if (motor_wave_active) return;
    
    current_motor_wave = wave;
    motor_segment_index = 0;
    motor_wave_active = true;
    
    const MotorSegment* seg = &wave->data[0];
    if (seg->speed > 0) {
        Motor_Forward(MOTOR_BOTH, seg->speed);
    } else {
        Motor_Stop(MOTOR_BOTH);
    }
    motor_segment_deadline = millis() + seg->duration_ms;
}

static void motor_wave_stop(void) {
    motor_wave_active = false;
    current_motor_wave = NULL;
    Motor_Stop(MOTOR_BOTH);
}

static void motor_wave_update(void) {
    if (!motor_wave_active) return;
    if (current_motor_wave == NULL) return;
    
    if (millis() >= motor_segment_deadline) {
        motor_segment_index++;
        
        if (motor_segment_index >= current_motor_wave->segments) {
            motor_wave_stop();
            return;
        }
        
        const MotorSegment* seg = &current_motor_wave->data[motor_segment_index];
        if (seg->speed > 0) {
            Motor_Forward(MOTOR_BOTH, seg->speed);
        } else {
            Motor_Stop(MOTOR_BOTH);
        }
        motor_segment_deadline = millis() + seg->duration_ms;
    }
}

/* ================= 蜂鸣器序列播放 ================= */
static void buzzer_seq_start(const BuzzerSequence* seq) {
    if (seq == NULL || seq->count == 0) return;
    if (!buzzer_enabled) return;
    if (buzzer_seq_active) return;
    
    current_buzzer_seq = seq;
    buzzer_note_index = 0;
    buzzer_seq_active = true;
    
    play_note(seq, 0);
}

static void buzzer_seq_stop(void) {
    buzzer_seq_active = false;
    current_buzzer_seq = NULL;
    Buzzer_Stop();
}

static void play_note(const BuzzerSequence* seq, uint8_t index) {
    if (index >= seq->count) {
        buzzer_seq_stop();
        return;
    }
    
    const BuzzerNote* note = &seq->notes[index];
    if (note->freq_hz == NOTE_REST || note->volume == 0) {
        Buzzer_Stop();
    } else {
        Buzzer_PlayTone(note->freq_hz, note->duration_ms, note->volume);
    }
    buzzer_note_deadline = millis() + note->duration_ms;
}

static void buzzer_seq_update(void) {
    if (!buzzer_seq_active) return;
    if (current_buzzer_seq == NULL) return;
    
    if (millis() >= buzzer_note_deadline) {
        buzzer_note_index++;
        play_note(current_buzzer_seq, buzzer_note_index);
    }
}

/* ================= 统一 API 实现 ================= */

void Output_Feedback(FeedbackEvent event) {
    if (event < 0 || event >= FEEDBACK_NONE) return;
    
    const FeedbackMapping* map = &feedback_map[event];
    
    if (map->motor_enable && map->motor_wave && motor_enabled) {
        motor_wave_start(map->motor_wave);
    }
    
    if (map->buzzer_enable && map->buzzer_seq && buzzer_enabled) {
        buzzer_seq_start(map->buzzer_seq);
    }
}

void Output_MotorVibrate(uint8_t speed, uint16_t duration_ms) {
    if (!motor_enabled) return;
    
    MotorWaveform w = {.name = "SIMPLE", .segments = 1, .data = {{speed, duration_ms}}};
    motor_wave_start(&w);
}

void Output_MotorWave(const MotorWaveform* wave) {
    motor_wave_start(wave);
}

void Output_MotorStop(void) {
    motor_wave_stop();
}

void Output_BuzzerTone(uint16_t freq_hz, uint16_t duration_ms, uint8_t volume) {
    if (!buzzer_enabled) return;
    
    BuzzerSequence s = {.name = "SIMPLE", .count = 1, .notes = {{freq_hz, duration_ms, volume}}};
    buzzer_seq_start(&s);
}

void Output_BuzzerPlay(const BuzzerSequence* sequence) {
    buzzer_seq_start(sequence);
}

void Output_BuzzerStop(void) {
    buzzer_seq_stop();
}

void Output_Stop(void) {
    motor_wave_stop();
    buzzer_seq_stop();
}

bool Output_IsMotorPlaying(void) {
    return motor_wave_active;
}

bool Output_IsBuzzerPlaying(void) {
    return buzzer_seq_active;
}

bool Output_IsPlaying(void) {
    return motor_wave_active || buzzer_seq_active;
}

void Output_SetMotorEnabled(bool enable) {
    motor_enabled = enable;
    if (!enable) motor_wave_stop();
}

void Output_SetBuzzerEnabled(bool enable) {
    buzzer_enabled = enable;
    if (!enable) buzzer_seq_stop();
}

void Output_SetEnabled(bool motor_enable, bool buzzer_enable) {
    Output_SetMotorEnabled(motor_enable);
    Output_SetBuzzerEnabled(buzzer_enable);
}

bool Output_IsMotorEnabled(void) {
    return motor_enabled;
}

bool Output_IsBuzzerEnabled(void) {
    return buzzer_enabled;
}

void OutputFeedback_Update(void) {
    motor_wave_update();
    buzzer_seq_update();
}