#pragma once
#include <Arduino.h>
#include "mpu6050.h"
/* 输入层初始化 */
void Input_Init(void);

/* 输入层周期更新（loop 中调用） */
void Input_Update(void);
