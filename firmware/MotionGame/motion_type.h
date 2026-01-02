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
