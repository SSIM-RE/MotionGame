#pragma once
#include <Arduino.h>
#include "motion_type.h"

/* ================= 接口 ================= */
extern IMU IMU_me ;
/* 初始化（在 setup 调用） */
void MotionService_Init(void);

/* 更新（在 loop 中周期调用）
 * 返回：本周期检测到的“新动作事件”
 */
MotionType_t MotionService_Update(void);
