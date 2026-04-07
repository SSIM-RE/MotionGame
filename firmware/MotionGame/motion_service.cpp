#include "motion_service.h"
#include "input.h"
#include <math.h>



/* ================= 内部状态 ================= */
IMU IMU_me ;
static MotionType_t last_motion = MOTION_NONE;
static uint32_t last_motion_time = 0;

static float ref_roll  = 0.0f;
static float ref_pitch = 0.0f;
static bool  ref_valid = false;

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
void MotionService_UpdateReference(void)
{
    float acc_norm = sqrt(
        IMU_me.ax*IMU_me.ax +
        IMU_me.ay*IMU_me.ay +
        IMU_me.az*IMU_me.az
    );

    bool stable =
        fabs(IMU_me.gx) < 10.0f &&
        fabs(IMU_me.gy) < 10.0f &&
        fabs(acc_norm - 1.0f) < 0.05f;

    if (stable)
    {
        IMU_me.ref_pitch = IMU_me.ref_pitch * 0.989f + IMU_me.pitch * 0.011f;
        IMU_me.ref_roll  = IMU_me.ref_roll  * 0.989f + IMU_me.roll  * 0.011f;
        IMU_me.ref_ax = IMU_me.ref_ax * 0.989f + IMU_me.ax * 0.011f;
        IMU_me.ref_ay = IMU_me.ref_ay * 0.989f + IMU_me.ay * 0.011f;
        IMU_me.ref_az = IMU_me.ref_az * 0.989f + IMU_me.az  * 0.011f;
    }
}
static const char* motion_to_str(MotionType_t m)
{
    switch (m)
    {
        case MOTION_TILT_LEFT:   return "TILT_LEFT";
        case MOTION_TILT_RIGHT:  return "TILT_RIGHT";
        case MOTION_ROLL_FRONT:  return "ROLL_FRONT";
        case MOTION_ROLL_BACK:   return "ROLL_BACK";       
        case MOTION_SHAKE:       return "SHAKE";
        default:                 return "NONE";
    }
}


MotionType_t MotionService_Update(void)
{

    
    MPU6050_GetAngle(&IMU_me.roll, &IMU_me.pitch);
    MPU6050_GetAccel(&IMU_me.ax, &IMU_me.ay, &IMU_me.az);
    MPU6050_GetGyro(&IMU_me.gx, &IMU_me.gy, &IMU_me.gz);

    /* ---------- 基准校准 ---------- */


    MotionService_UpdateReference();
    IMU_me.rel_roll  = IMU_me.roll-IMU_me.ref_roll;
    IMU_me.rel_pitch = IMU_me.pitch-IMU_me.ref_pitch;
    

    MotionType_t detected = MOTION_NONE;

    /* ---------- 左右倾斜检测（相对 Pitch） ---------- */
    if (IMU_me.rel_pitch > PITCH_THRESHOLD_DEG)
    {
        detected = MOTION_TILT_RIGHT;
    }
    else if (IMU_me.rel_pitch < -PITCH_THRESHOLD_DEG)
    {
        detected = MOTION_TILT_LEFT;
    }
    /* ---------- 前后倾斜检测（相对 roll） ---------- */
    // else if (IMU_me.rel_roll > ROLL_THRESHOLD_DEG)
    // {
    //     detected = MOTION_ROLL_FRONT;
    // }
    // else if (IMU_me.rel_roll < -ROLL_THRESHOLD_DEG)
    // {
    //     detected = MOTION_ROLL_BACK;
    // }
    /* ---------- 晃动检测（加速度模长） ---------- */
    else
    {
        float acc_norm = sqrt(
            IMU_me.ax * IMU_me.ax +
            IMU_me.ay * IMU_me.ay +
            IMU_me.az * IMU_me.az
        );

        if (fabs(acc_norm - 1.0f) > SHAKE_ACC_THRESHOLD)
        {
            detected = MOTION_SHAKE;
        }
    }

    /* ---------- 去重 & 冷却 ---------- */
    if (detected != MOTION_NONE)
    {
        if (detected != last_motion && MotionCooldownPassed())
        {
            last_motion = detected;
            last_motion_time = millis();
            //Serial.printf(motion_to_str(detected));
            IMU_me.motion = detected;  // 记录当前动作
            return detected;
        }
        else
        {
            // 冷却中或相同动作，不触发
            IMU_me.motion = MOTION_NONE;
        }
    }
    else
    {
        last_motion = MOTION_NONE;
        IMU_me.motion = MOTION_NONE;
    }

    return MOTION_NONE;
}

