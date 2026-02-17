// menu_app.h
#ifndef MENU_APP_H
#define MENU_APP_H

#include <stdint.h>
#include <stdbool.h>
#include "system_state.h"
#include "common_colors.h"

// 初始化主菜单应用
void MenuApp_Init(void);

// 更新主菜单应用，返回系统状态
SystemState_t MenuApp_Update(void);

// 应用主题颜色到主菜单
void MenuApp_ApplyThemeColors(const AppColors_t* theme_colors);

// 控制函数
void MenuApp_SetDampingStrength(float strength);
void MenuApp_SetAnimationDuration(float duration);
float MenuApp_GetDampingStrength(void);
float MenuApp_GetAnimationDuration(void);
int MenuApp_GetCurrentIndex(void);
void MenuApp_SetCurrentIndex(int index);
bool MenuApp_IsAnimating(void);
void MenuApp_ForceRedraw(void);
void MenuApp_ApplyAnimationPreset(int preset);

#endif // MENU_APP_H