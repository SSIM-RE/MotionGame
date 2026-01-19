#pragma once
#include "motion_type.h"

/* 初始化 */
void MotionDebugSerial_Init(void);

/* 周期打印（低频） */
void MotionDebugSerial_Update(void);

/* 动作触发日志 */
void MotionDebugSerial_OnMotion(MotionType_t motion);

/* 开关 */
void MotionDebugSerial_Enable(bool en);
