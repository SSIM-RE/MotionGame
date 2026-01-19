#ifndef MENU_APP_H
#define MENU_APP_H

#include <stdint.h>
#include <stdbool.h>

// 菜单结果枚举
typedef enum {
    MENU_NONE = 0,
    MENU_ENTER_GAME,
    MENU_ENTER_THEME,
    MENU_ENTER_SETTINGS,
    MENU_ENTER_ABOUT
} MenuResult_t;

// 初始化菜单应用
void MenuApp_Init(void);

// 更新菜单应用，返回菜单操作结果
MenuResult_t MenuApp_Update(void);

// 参数控制函数
void MenuApp_SetDampingStrength(float strength);
void MenuApp_SetAnimationDuration(float duration);
float MenuApp_GetDampingStrength(void);
float MenuApp_GetAnimationDuration(void);

// 状态获取函数
int MenuApp_GetCurrentIndex(void);
void MenuApp_SetCurrentIndex(int index);
bool MenuApp_IsAnimating(void);
void MenuApp_ForceRedraw(void);

// 预设效果
void MenuApp_ApplyAnimationPreset(int preset);

#endif // MENU_APP_H