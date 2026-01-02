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
