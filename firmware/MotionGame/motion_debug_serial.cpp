#include "motion_debug_serial.h"
#include "motion_service.h"
#include <Arduino.h>
#include <math.h>
/* ================= 配置 ================= */

#define PRINT_INTERVAL_MS 100   // 10Hz

static bool debug_enable = true;
static uint32_t last_print_ms = 0;

/* ================= 工具 ================= */

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

/* ================= 接口实现 ================= */

void MotionDebugSerial_Init(void)
{
    Serial.println("Motion Debug Serial ON");
}

void MotionDebugSerial_Enable(bool en)
{
    debug_enable = en;
}

void  MotionDebugSerial_Update(void)
{
    if (!debug_enable) return;

    uint32_t now = millis();
    if (now - last_print_ms < PRINT_INTERVAL_MS) return;
    last_print_ms = now;

    /* 核心判据数据 */
    Serial.print("roll:");
    Serial.print(IMU_me.roll, 1);
    Serial.print(" roll_ref:");
    Serial.print(IMU_me.ref_roll, 1);
    Serial.print(" roll_rel:");
    Serial.print(IMU_me.rel_roll, 1);

    Serial.print(" pitch:");
    Serial.print(IMU_me.pitch, 1);
    Serial.print(" pitch_ref:");
    Serial.print(IMU_me.ref_pitch, 1);
    Serial.print(" pitch_rel:");
    Serial.print(IMU_me.rel_pitch, 1);
    Serial.print("\n");

}

void MotionDebugSerial_OnMotion(MotionType_t motion)
{
    if (!debug_enable) return;
    if (motion == MOTION_NONE) return;
    Serial.print("[MOTION] ");
    Serial.print(motion_to_str(motion));
    Serial.print(" @ ");
    Serial.print(millis());
    Serial.println(" ms");
}
