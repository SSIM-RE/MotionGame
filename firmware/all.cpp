
//audio_service.cpp文件如下：
#include "buzzer.h"

static const BuzzerNote attack_notes[] = {
    { NOTE_E5, 60, 60 },
    { NOTE_G5, 70, 60 },
    { NOTE_A5, 80, 80 },
    { NOTE_E5, 60, 60 },
    { NOTE_G5, 70, 60 },
    { NOTE_A5, 80, 80 }   
};

static const BuzzerNote hit_notes[] = {
    { NOTE_C5, 80, 100 },
    { NOTE_G4, 60, 120 },
};

static const BuzzerNote startup_notes[] = {
    { NOTE_C4, 40, 80 },
    { NOTE_E4, 50, 80 },
    { NOTE_G4, 60, 80 },
    { NOTE_C5, 70, 120 },
};

static const BuzzerSequence buzzer_table[] = {
    { attack_notes,  sizeof(attack_notes)  / sizeof(BuzzerNote) },
    { hit_notes,     sizeof(hit_notes)     / sizeof(BuzzerNote) },
    { startup_notes, sizeof(startup_notes) / sizeof(BuzzerNote) },
};
void Audio_Play(BuzzerSound sound)
{
    if (sound >= BUZZER_SOUND_COUNT) return;
        Buzzer_Play(&buzzer_table[sound]);
}

bool Audio_isPlaying(BuzzerSound sound)
{
    return Buzzer_isPlaying();
}

bool Audio_Stop(BuzzerSound sound)
{
    Buzzer_Stop();
}
//audio_service.h文件如下：
#pragma once
#include <Arduino.h>
#include "audio_type.h"
void Audio_Play(BuzzerSound sound);
bool Audio_isPlaying(BuzzerSound sound);
bool Audio_Stop(BuzzerSound sound);
//audio_type.h文件如下：
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

//buzzer.cpp文件如下：
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
//input.cpp文件如下：
#include "input.h"
#include "mpu6050.h"
//#include "motion_detect.h"

void Input_Init(void)
{
    MPU6050_Init();
    //MotionDetect_Init();
}

void Input_Update(void)
{
    MPU6050_Update();
   // MotionDetect_Update();
}
//input.h文件如下：
#pragma once
#include <Arduino.h>
#include "mpu6050.h"
/* 输入层初始化 */
void Input_Init(void);

/* 输入层周期更新（loop 中调用） */
void Input_Update(void);
//lcd.cpp文件如下：
#include "lcd.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

/* ===== 硬件引脚 ===== */
#define TFT_CS   7
#define TFT_RST  10
#define TFT_DC   6
#define TFT_MOSI 3
#define TFT_SCLK 2

/* ===== 颜色定义 ===== */
#define LCD_BLACK   0x0000
#define LCD_YELLOW  0x07FF
#define LCD_TEXT    LCD_BLACK
#define LCD_BG      LCD_YELLOW
#define Display_Color_Black    0x0000    //黑色
#define Display_Color_Red      0x001F    //红色
#define Display_Color_Blue     0xF800    //蓝色
#define Display_Color_Green    0x07E0    //绿色
#define Display_Color_Yellow   0x07FF    //黄色
#define Display_Color_Magenta  0xF81F    //洋红
#define Display_Color_Cyan     0xFFE0    //青色
#define Display_Color_White    0xFFFF    //白色

#define DOT_RADIUS 3
#define BG_COLOR Display_Color_White
#define DOT_COLOR Display_Color_Red


static Adafruit_ST7735 tft =
  Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

void lcd_init(void) {
  tft.initR(INITR_BLACKTAB);
  tft.setFont();
  tft.setTextSize(1);
  tft.fillScreen(LCD_BG);
  tft.setTextColor(LCD_TEXT, LCD_BG);
}

void lcd_enable(bool on) {
  tft.enableDisplay(on);
}

void lcd_clear(uint16_t color) {
  tft.fillScreen(color);
}

void lcd_set_cursor(int x, int y) {
  tft.setCursor(x, y);
}

void lcd_set_text_color(uint16_t fg, uint16_t bg) {
  tft.setTextColor(fg, bg);
}

void lcd_print(const char* text) {
  tft.print(text);
}
void lcd_draw_dot(int x, int y) {
    tft.fillCircle(x, y, DOT_RADIUS, DOT_COLOR);
}

void lcd_clear_dot(int x, int y) {
    tft.fillCircle(x, y, DOT_RADIUS, BG_COLOR);
}
//lcd.h文件如下：
#pragma once
#include <stdint.h>
#include <stdbool.h>

void lcd_init(void);
void lcd_enable(bool on);

void lcd_clear(uint16_t color);
void lcd_set_cursor(int x, int y);
void lcd_set_text_color(uint16_t fg, uint16_t bg);
void lcd_print(const char* text);
void lcd_draw_dot(int x, int y);
void lcd_clear_dot(int x, int y);
//motion_service.cpp文件如下：
#include "motion_service.h"
#include "input.h"
#include <math.h>



/* ================= 内部状态 ================= */
IMU IMU_me ;
static MotionType_t last_motion = MOTION_NONE;
static uint32_t last_motion_time = 0;

/* ================= 初始化 ================= */

void MotionService_Init(void)
{
    last_motion = MOTION_NONE;
    last_motion_time = 0;
}

/* ================= 内部工具 ================= */

static bool MotionCooldownPassed(void)
{
    return (millis() - last_motion_time) > MOTION_COOLDOWN_MS;
}

/* ================= 核心更新 ================= */

MotionType_t MotionService_Update(void)
{
    float roll, pitch;
    float gx, gy, gz;

    MPU6050_GetAngle(&roll, &pitch);
    MPU6050_GetGyro(&gx, &gy, &gz);
    MPU6050_GetAngle(&IMU_me.roll, &IMU_me.pitch);
    MPU6050_GetAccel(&IMU_me.ax, &IMU_me.ay, &IMU_me.az);
    MPU6050_GetGyro(&IMU_me.gx, &IMU_me.gy, &IMU_me.gz);  
    MotionType_t detected = MOTION_NONE;

    /* ---------- 倾斜检测 ---------- */
    if (roll > TILT_THRESHOLD_DEG)
    {
        detected = MOTION_TILT_RIGHT;
    }
    else if (roll < -TILT_THRESHOLD_DEG)
    {
        detected = MOTION_TILT_LEFT;
    }

    /* ---------- 挥动检测 ---------- */
    else if (gy > SWING_GYRO_THRESHOLD)
    {
        detected = MOTION_SWING_UP;
    }
    else if (gy < -SWING_GYRO_THRESHOLD)
    {
        detected = MOTION_SWING_DOWN;
    }

    /* ---------- 晃动检测 ---------- */
    else if (fabs(gx) > SHAKE_GYRO_THRESHOLD ||
             fabs(gy) > SHAKE_GYRO_THRESHOLD)
    {
        detected = MOTION_SHAKE;
    }

    /* ---------- 去重 & 冷却 ---------- */
    if (detected != MOTION_NONE)
    {
        if (detected != last_motion && MotionCooldownPassed())
        {
            last_motion = detected;
            last_motion_time = millis();
            return detected;   // 只在“新动作”时返回
        }
    }
    else
    {
        /* 动作结束，允许下次触发 */
        last_motion = MOTION_NONE;
    }

    return MOTION_NONE;
}
//motion_service.h文件如下：
#pragma once
#include <Arduino.h>
#include "motion_type.h"

/* ================= 接口 ================= */
extern IMU IMU_me ;
/* 初始化（在 setup 调用） */
void MotionService_Init(void);

/* 更新（在 loop 中周期调用）
 * 返回：本周期检测到的“新动作事件”
 */
MotionType_t MotionService_Update(void);
//motion_type.h文件如下：
#pragma once
/* ================= 参数配置 ================= */

/* 倾斜阈值（角度） */
#define TILT_THRESHOLD_DEG        20.0f

/* 挥动阈值（角速度 deg/s） */
#define SWING_GYRO_THRESHOLD      180.0f

/* 晃动阈值 */
#define SHAKE_GYRO_THRESHOLD      250.0f

/* 动作最小间隔（ms）——防止重复触发 */
#define MOTION_COOLDOWN_MS        300

/* ================= 动作类型定义 ================= */

typedef enum
{
    MOTION_NONE = 0,

    MOTION_TILT_LEFT,
    MOTION_TILT_RIGHT,

    MOTION_SWING_UP,
    MOTION_SWING_DOWN,

    MOTION_SHAKE,

} MotionType_t;

typedef struct {
    float roll; 
    float pitch;
    float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
} IMU;
//MotionGame.ino文件如下：
#include "output.h"
#include "input.h"
#include "motion_service.h"
#include "motion_type.h"
#include "audio_service.h"
#include "audio_type.h"
#include "lcd.h"

void setup() {
    Serial.begin(115200);
    lcd_init();
    Input_Init();
    Output_Init();
    lcd_draw_dot(64, 80);
}

void loop() {
    Input_Update();



    // 映射到屏幕
    int x = 64 + IMU_me.ax  * 50;
    int y = 80 + IMU_me.ay * 50;   // 注意 Y 轴反向
    int last_x =64;
    int last_y =80;
    x = constrain(x, 10, 118);
    y = constrain(y, 10, 150);

    // 擦除旧点
    lcd_clear_dot(last_x, last_y);

    // 画新点
    lcd_draw_dot(x, y);
    last_x = x;
    last_y = y;


    MotionType_t motion = MotionService_Update();
    if (motion != MOTION_NONE)
    {
        Serial.println(motion);
        Audio_Play(BUZZER_ATTACK);
    }
    Output_Update();
    delay(20);
}
//mpu6050.cpp文件如下：
#include "mpu6050.h"
#include <Wire.h>
#include <math.h>

/* ================== MPU6050 寄存器 ================== */
#define MPU6050_ADDR       0x68
#define REG_PWR_MGMT_1     0x6B
#define REG_ACCEL_XOUT_H   0x3B
#define REG_GYRO_XOUT_H    0x43

/* ================== 参数配置 ================== */
#define ACC_SCALE   16384.0f   // ±2g
#define GYRO_SCALE  131.0f     // ±250 dps

#define LPF_ALPHA  0.8f        // 低通滤波系数（0.7~0.9 常用）

/* ================== 内部状态（私有） ================== */
static float acc_f[3]  = {0};
static float gyro_f[3] = {0};

static float roll_f  = 0.0f;
static float pitch_f = 0.0f;

static uint32_t last_update_ms = 0;

/* ================== 工具函数 ================== */
static inline float lpf(float prev, float input)
{
    return LPF_ALPHA * prev + (1.0f - LPF_ALPHA) * input;
}

static int16_t read16(uint8_t reg)
{
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU6050_ADDR, (uint8_t)2);

    return (Wire.read() << 8) | Wire.read();
}

/* ================== 初始化 ================== */
void MPU6050_Init(void)
{
    Wire.begin(SDA,SCL);

    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(REG_PWR_MGMT_1);
    Wire.write(0x00);   // 唤醒
    Wire.endTransmission();

    delay(100);
    last_update_ms = millis();
}

/* ================== 周期更新 ================== */
void MPU6050_Update(void)
{
    uint32_t now = millis();
    float dt = (now - last_update_ms) * 0.001f;
    if (dt <= 0.0f || dt > 0.1f) dt = 0.01f;
    last_update_ms = now;

    /* ---- 读取原始数据 ---- */
    int16_t ay_raw = read16(REG_ACCEL_XOUT_H);
    int16_t ax_raw = read16(REG_ACCEL_XOUT_H + 2);
    int16_t az_raw = read16(REG_ACCEL_XOUT_H + 4);

    int16_t gx_raw = read16(REG_GYRO_XOUT_H);
    int16_t gy_raw = read16(REG_GYRO_XOUT_H + 2);
    int16_t gz_raw = read16(REG_GYRO_XOUT_H + 4);

    /* ---- 单位转换 ---- */
    float ax = ax_raw / ACC_SCALE;
    float ay = ay_raw / ACC_SCALE;
    float az = az_raw / ACC_SCALE;

    float gx = gx_raw / GYRO_SCALE;
    float gy = gy_raw / GYRO_SCALE;
    float gz = gz_raw / GYRO_SCALE;

    /* ---- 低通滤波 ---- */
    acc_f[0]  = lpf(acc_f[0], ax);
    acc_f[1]  = lpf(acc_f[1], ay);
    acc_f[2]  = lpf(acc_f[2], az);

    gyro_f[0] = lpf(gyro_f[0], gx);
    gyro_f[1] = lpf(gyro_f[1], gy);
    gyro_f[2] = lpf(gyro_f[2], gz);

    /* ---- 姿态解算（互补滤波）---- */
    float roll_acc  = atan2(acc_f[1], acc_f[2]) * RAD_TO_DEG;
    float pitch_acc = atan2(-acc_f[0],
                        sqrt(acc_f[1]*acc_f[1] + acc_f[2]*acc_f[2])) * RAD_TO_DEG;

    roll_f  = 0.98f * (roll_f  + gyro_f[0] * dt) + 0.02f * roll_acc;
    pitch_f = 0.98f * (pitch_f + gyro_f[1] * dt) + 0.02f * pitch_acc;
}

/* ================== 对外接口 ================== */
void MPU6050_GetAccel(float* ax, float* ay, float* az)
{
    *ax = acc_f[0];
    *ay = acc_f[1];
    *az = acc_f[2];
}

void MPU6050_GetGyro(float* gx, float* gy, float* gz)
{
    *gx = gyro_f[0];
    *gy = gyro_f[1];
    *gz = gyro_f[2];
}

void MPU6050_GetAngle(float* roll, float* pitch)
{
    *roll  = roll_f;
    *pitch = pitch_f;
}
//mpu6050.h文件如下：
#pragma once
#include <Arduino.h>

#define SCL 5
#define SDA 4

/* ========== 初始化 & 更新 ========== */
void MPU6050_Init(void);
void MPU6050_Update(void);   // 必须周期性调用（如 100~500Hz）

/* ========== 对外数据接口（已滤波） ========== */
void MPU6050_GetAccel(float* ax, float* ay, float* az); // g
void MPU6050_GetGyro(float* gx, float* gy, float* gz);  // deg/s
void MPU6050_GetAngle(float* roll, float* pitch);       // deg
//output.h文件如下：
#pragma once
#include <stdint.h>
#include "buzzer.h"


void Output_Init(void);
void Output_Update(void);
//output.cpp文件如下：
#include "buzzer.h"
#include "output.h"


void Output_Init(void) {
    Buzzer_Init();

}

void Output_Update(void) {
    Buzzer_Update();
}
//ui.cpp文件如下：
#include "ui.h"
#include "lcd.h"
#include <Arduino.h>
#include <string.h>
#include <stdio.h>

static char oldTime[16] = {0};

void ui_init(void) {
  lcd_clear(0x07FF);
}

void ui_draw_uptime(void) {
  unsigned long sec = millis() / 1000;

  unsigned long d = sec / 86400;
  sec %= 86400;
  unsigned long h = sec / 3600;
  sec %= 3600;
  unsigned long m = sec / 60;
  sec %= 60;

  char buf[16];
  sprintf(buf, "%lu %02lu:%02lu:%02lu", d, h, m, sec);

  if (strcmp(buf, oldTime) != 0) {
    lcd_set_cursor(10, 80);
    lcd_print(oldTime);   // 擦除
    lcd_set_cursor(10, 80);
    lcd_print(buf);
    strcpy(oldTime, buf);
  }
}
//ui.h文件如下：
#pragma once
void ui_init(void);
void ui_draw_uptime(void);
