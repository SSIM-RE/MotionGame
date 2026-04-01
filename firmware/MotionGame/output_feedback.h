#pragma once
#include <Arduino.h>

/* ================= 统一反馈事件 ================= */

typedef enum {
    // ===== 组合反馈（马达+蜂鸣器）=====
    FEEDBACK_EAT,           // 吃到食物：轻震 + 短音
    FEEDBACK_HIT,           // 撞墙：强震 + 警告音
    FEEDBACK_DIE,           // 死亡：两下急促 + 低沉音
    FEEDBACK_LEVEL_UP,      // 升级：三下震动 + 欢快音
    FEEDBACK_SUCCESS,       // 成功：长震 + 上扬音
    FEEDBACK_ERROR,         // 错误：三短震动 + 错误音
    FEEDBACK_WARNING,       // 警告：脉冲震动 + 警告音
    FEEDBACK_GAME_OVER,     // 游戏结束：长震 + 结束音

    // ===== 操作反馈 =====
    FEEDBACK_CLICK,         // 点击：短震 + 短音
    FEEDBACK_SELECT,        // 选中：短震 + 确认音
    FEEDBACK_BACK,          // 返回：中等震动

    // ===== 纯马达震动 =====
    FEEDBACK_RUMBLE,        // 引擎轰鸣（多段波形）
    FEEDBACK_TICK,          // 时钟节拍（极短震）

    // ===== 纯蜂鸣器音效 =====
    FEEDBACK_BEEP,          // 简单提示音
    FEEDBACK_POWER_ON,      // 开机音效
    FEEDBACK_POWER_OFF,     // 关机音效
    FEEDBACK_MENU_MOVE,     // 菜单移动音效
    FEEDBACK_COUNTDOWN,     // 倒计时音效

    // ===== 特殊 =====
    FEEDBACK_NONE           // 无反馈
} FeedbackEvent;

/* ================= 马达多段波形配置 ================= */

#define MOTOR_WAVE_MAX_SEGMENTS 8

typedef struct {
    uint8_t speed;              // 速度 0-255
    uint16_t duration_ms;       // 持续时间
} MotorSegment;

typedef struct {
    const char* name;           // 波形名称
    uint8_t segments;           // 段数
    MotorSegment data[MOTOR_WAVE_MAX_SEGMENTS];
} MotorWaveform;

/* ================= 蜂鸣器序列配置 ================= */

#define BUZZER_MAX_NOTES 16

typedef enum {
    NOTE_REST = 0              // 休止符
} BuzzerNoteType;

typedef struct {
    uint16_t freq_hz;           // 频率 Hz (NOTE_REST=休止)
    uint16_t duration_ms;       // 持续时间
    uint8_t volume;             // 音量 0-100
} BuzzerNote;

typedef struct {
    const char* name;           // 序列名称
    uint8_t count;              // 音符数量
    BuzzerNote notes[BUZZER_MAX_NOTES];
} BuzzerSequence;

/* ================= 反馈配置 ================= */

// 马达震动配置
typedef struct {
    uint8_t speed;              // 速度 0-255
    uint16_t duration_ms;       // 持续时间
    uint8_t count;              // 次数
    uint16_t interval_ms;       // 间隔
} MotorConfig;

// 蜂鸣器音效配置（单音调）
typedef struct {
    uint16_t freq_hz;           // 频率 Hz
    uint16_t duration_ms;       // 持续时间
    uint8_t volume;             // 音量 0-100
} BuzzerConfig;

// 组合反馈配置
typedef struct {
    FeedbackEvent event;        // 事件类型
    const MotorWaveform* motor_wave;   // 马达波形（可为空）
    uint8_t motor_speed;        // 马达速度（简单模式）
    uint16_t motor_duration;    // 马达时长（简单模式）
    const BuzzerSequence* buzzer_seq; // 蜂鸣器序列（可为空）
    uint16_t buzzer_freq;       // 蜂鸣器频率（简单模式）
    uint16_t buzzer_duration;   // 蜂鸣器时长（简单模式）
    uint8_t buzzer_volume;      // 蜂鸣器音量
    bool motor_enable;          // 是否启用马达
    bool buzzer_enable;         // 是否启用蜂鸣器
} FeedbackConfig;

/* ================= 统一 API ================= */

// 统一反馈触发
void Output_Feedback(FeedbackEvent event);

// ===== 马达控制 =====
// 简单震动
void Output_MotorVibrate(uint8_t speed, uint16_t duration_ms);
// 多段波形震动
void Output_MotorWave(const MotorWaveform* wave);
// 停止马达
void Output_MotorStop(void);

// ===== 蜂鸣器控制 =====
// 单音调
void Output_BuzzerTone(uint16_t freq_hz, uint16_t duration_ms, uint8_t volume);
// 序列播放
void Output_BuzzerPlay(const BuzzerSequence* sequence);
// 停止蜂鸣器
void Output_BuzzerStop(void);

// ===== 统一控制 =====
// 停止所有
void Output_Stop(void);

// 查询状态
bool Output_IsMotorPlaying(void);
bool Output_IsBuzzerPlaying(void);
bool Output_IsPlaying(void);

// 总开关
void Output_SetMotorEnabled(bool enable);
void Output_SetBuzzerEnabled(bool enable);
void Output_SetEnabled(bool motor_enable, bool buzzer_enable);
bool Output_IsMotorEnabled(void);
bool Output_IsBuzzerEnabled(void);

// 非阻塞更新
void OutputFeedback_Update(void);