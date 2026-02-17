// theme_app.h
#ifndef THEME_APP_H
#define THEME_APP_H

#include <stdint.h>
#include "system_state.h"

// 主题颜色结构体
typedef struct {
    uint16_t background;
    uint16_t foreground;
    uint16_t primary;
    uint16_t secondary;
    uint16_t accent;
    uint16_t text;
    uint16_t highlight;
    uint16_t border;
} ThemeColors_t;

// 全局主题接口
const ThemeColors_t* Theme_GetCurrentColors(void);
Theme_t Theme_GetCurrent(void);
const ThemeColors_t* Theme_GetAllThemes(void);
int Theme_GetThemeCount(void);

// 主题应用界面
void ThemeApp_Init(void);
SystemState_t ThemeApp_Update(void);

#endif // THEME_APP_H