#pragma once
#include <Arduino.h>

/* ================= 硬件配置 ================= */
// #define MOTOR_A_IN1    12
// #define MOTOR_A_IN2    12
// #define MOTOR_B_IN3    12
// #define MOTOR_B_IN4    12
#define MOTOR_A_IN1    12
#define MOTOR_A_IN2    18
#define MOTOR_B_IN3    8
#define MOTOR_B_IN4    13

#define MOTOR_PWM_FREQ     1000
#define MOTOR_PWM_RES      8

#define HAPTIC_SPEED_LOW     128
#define HAPTIC_SPEED_MEDIUM  192
#define HAPTIC_SPEED_HIGH    255

/* ================= 枚举定义 ================= */
typedef enum {
    MOTOR_STATE_STOP = 0,
    MOTOR_STATE_FORWARD,
    MOTOR_STATE_BRAKE
} MotorState;

typedef enum {
    MOTOR_A = 0,
    MOTOR_B = 1,
    MOTOR_BOTH = 2
} MotorSelect;

/* ================= API 函数 ================= */
void Motor_Init(void);
void Motor_Stop(MotorSelect motor);
void Motor_Brake(MotorSelect motor);
void Motor_Forward(MotorSelect motor, uint8_t speed);
void Motor_Vibrate(uint8_t speed);
void Motor_VibrateShort(void);
void Motor_VibrateMedium(void);
void Motor_VibrateLong(void);
void Motor_Update(void);
bool Motor_isRunning(void);
void Motor_TestSineWave(void);
void Motor_TestSineWave_Stop(void);