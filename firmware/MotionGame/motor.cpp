#include "motor.h"
#include <Arduino.h>
#include <math.h>

/* ================= 震动状态（供外部访问）================= */
bool motor_vibrating = false;
uint32_t motor_vibrate_deadline = 0;

/* ================= 状态变量 ================= */
static MotorState motor_a_state = MOTOR_STATE_STOP;
static MotorState motor_b_state = MOTOR_STATE_STOP;
static uint32_t vibrate_deadline = 0;

/* ================= 工具函数 ================= */
static void write_motor_a(MotorState state, uint8_t speed) {
    motor_a_state = state;
    
    switch (state) {
        case MOTOR_STATE_STOP:
            ledcWrite(MOTOR_A_IN1, 0);
            digitalWrite(MOTOR_A_IN2, LOW);
            break;
            
        case MOTOR_STATE_FORWARD:
            // IN1 = PWM (调速), IN2 = LOW (正转)
            ledcWrite(MOTOR_A_IN1, speed);
            digitalWrite(MOTOR_A_IN2, LOW);
            break;
            
        case MOTOR_STATE_BRAKE:
            // IN1 = HIGH (255), IN2 = HIGH (刹车)
            ledcWrite(MOTOR_A_IN1, 255);
            digitalWrite(MOTOR_A_IN2, HIGH);
            break;
    }
}

static void write_motor_b(MotorState state, uint8_t speed) {
    motor_b_state = state;
    
    switch (state) {
        case MOTOR_STATE_STOP:
            ledcWrite(MOTOR_B_IN3, 0);
            digitalWrite(MOTOR_B_IN4, LOW);
            break;
            
        case MOTOR_STATE_FORWARD:
            // IN3 = PWM (调速), IN4 = LOW (正转)
            ledcWrite(MOTOR_B_IN3, speed);
            digitalWrite(MOTOR_B_IN4, LOW);
            break;
            
        case MOTOR_STATE_BRAKE:
            // IN3 = HIGH (255), IN4 = HIGH (刹车)
            ledcWrite(MOTOR_B_IN3, 255);
            digitalWrite(MOTOR_B_IN4, HIGH);
            break;
    }
}

/* ================= 初始化 ================= */
void Motor_Init(void) {
    // 设置所有引脚为输出
    pinMode(MOTOR_A_IN1, OUTPUT);
    pinMode(MOTOR_A_IN2, OUTPUT);
    pinMode(MOTOR_B_IN3, OUTPUT);
    pinMode(MOTOR_B_IN4, OUTPUT);
    
    // 初始化 PWM 通道 (GPIO12, GPIO19)
    ledcAttach(MOTOR_A_IN1, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
    ledcAttach(MOTOR_B_IN3, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
    
    // 初始状态：停止
    Motor_Stop(MOTOR_BOTH);
    
    motor_vibrating = false;
    motor_vibrate_deadline = 0;
}

/* ================= 停止 ================= */
void Motor_Stop(MotorSelect motor) {
    if (motor == MOTOR_A || motor == MOTOR_BOTH) {
        write_motor_a(MOTOR_STATE_STOP, 0);
    }
    if (motor == MOTOR_B || motor == MOTOR_BOTH) {
        write_motor_b(MOTOR_STATE_STOP, 0);
    }
}

/* ================= 刹车 ================= */
void Motor_Brake(MotorSelect motor) {
    if (motor == MOTOR_A || motor == MOTOR_BOTH) {
        write_motor_a(MOTOR_STATE_BRAKE, 0);
    }
    if (motor == MOTOR_B || motor == MOTOR_BOTH) {
        write_motor_b(MOTOR_STATE_BRAKE, 0);
    }
}

/* ================= 正转调速 ================= */
void Motor_Forward(MotorSelect motor, uint8_t speed) {
    if (speed == 0) {
        Motor_Stop(motor);
        return;
    }
    
    if (motor == MOTOR_A || motor == MOTOR_BOTH) {
        write_motor_a(MOTOR_STATE_FORWARD, speed);
    }
    if (motor == MOTOR_B || motor == MOTOR_BOTH) {
        write_motor_b(MOTOR_STATE_FORWARD, speed);
    }
}

/* ================= 震动反馈 ================= */
void Motor_Vibrate(uint8_t speed) {
    Motor_Forward(MOTOR_BOTH, speed);
    motor_vibrating = true;
}

void Motor_VibrateShort(void) {
    Motor_Vibrate(HAPTIC_SPEED_MEDIUM);
    motor_vibrate_deadline = millis() + 50;
}

void Motor_VibrateMedium(void) {
    Motor_Vibrate(HAPTIC_SPEED_MEDIUM);
    motor_vibrate_deadline = millis() + 100;
}

void Motor_VibrateLong(void) {
    Motor_Vibrate(HAPTIC_SPEED_MEDIUM);
    motor_vibrate_deadline = millis() + 200;
}

/* ================= 非阻塞更新 ================= */
void Motor_Update(void) {
    if (motor_vibrating && millis() >= motor_vibrate_deadline) {
        Motor_Stop(MOTOR_BOTH);
        motor_vibrating = false;
    }
}

/* ================= 查询状态 ================= */
bool Motor_isRunning(void) {
    return (motor_a_state != MOTOR_STATE_STOP) || 
           (motor_b_state != MOTOR_STATE_STOP);
}

/* ================= 正弦波调速测试 ================= */
#define SINE_TEST_PERIOD_MS   2000    // 周期 2 秒
#define SINE_TEST_MAX_SPEED   255      // 最大速度

static bool sine_test_active = false;
static uint32_t sine_test_start = 0;

void Motor_TestSineWave(void) {
    if (!sine_test_active) {
        sine_test_active = true;
        sine_test_start = millis();
        Serial.println(">> 正弦波调速测试开始 (2秒周期)");
    }
    
    uint32_t elapsed = millis() - sine_test_start;
    float t = (float)(elapsed % SINE_TEST_PERIOD_MS) / SINE_TEST_PERIOD_MS;  // 0 ~ 1
    float sine_value = sin(t * 2 * PI);  // -1 ~ 1
    
    // 映射到速度 0-255 (去掉负半波，只正转)
    uint8_t speed = (uint8_t)((sine_value + 1.0f) / 2.0f * SINE_TEST_MAX_SPEED);
    
    Motor_Forward(MOTOR_BOTH, speed);
    
    // 每 500ms 打印一次状态
    static uint32_t last_print = 0;
    if (millis() - last_print > 500) {
        last_print = millis();
        Serial.print(">> 正弦波 t=");
        Serial.print(t, 2);
        Serial.print(" speed=");
        Serial.println(speed);
    }
}

void Motor_TestSineWave_Stop(void) {
    sine_test_active = false;
    Motor_Stop(MOTOR_BOTH);
    Serial.println(">> 正弦波测试停止");
}