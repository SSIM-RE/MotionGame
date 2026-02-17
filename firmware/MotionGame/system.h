// system.h
#pragma once

#include "system_state.h"

void System_Init(void);
void System_Update(void);
void System_EnterMenu(int index);

// 状态获取函数
SystemState_t System_GetCurrentState(void);
SystemState_t System_GetPreviousState(void);