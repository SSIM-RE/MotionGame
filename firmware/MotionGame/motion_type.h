#pragma once
/* ================= 参数配置 ================= */

/* 倾斜阈值（角度） */
#define ROLL_THRESHOLD_DEG        15.0f

#define PITCH_THRESHOLD_DEG        15.0f

/* 晃动阈值 */
#define SHAKE_ACC_THRESHOLD   0.35f

/* 动作最小间隔（ms）——防止重复触发 */
#define MOTION_COOLDOWN_MS        300

/* ================= 动作类型定义 ================= */

typedef enum
{
    MOTION_NONE = 0,

    MOTION_TILT_LEFT,
    MOTION_TILT_RIGHT,
    MOTION_ROLL_FRONT,
    MOTION_ROLL_BACK,
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
    float ref_ax  = 0.0f;
    float ref_ay  = 0.0f;
    float ref_az  = 0.0f;
    float ref_roll  = 0.0f;
    float ref_pitch = 0.0f;
    bool  ref_valid = false;
    float rel_ax  = 0.0f;
    float rel_ay  = 0.0f;
    float rel_az  = 0.0f;
    float rel_roll  = 0.0f;
    float rel_pitch = 0.0f;

} IMU;
