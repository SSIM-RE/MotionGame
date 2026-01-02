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
