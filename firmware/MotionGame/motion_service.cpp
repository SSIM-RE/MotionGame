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

