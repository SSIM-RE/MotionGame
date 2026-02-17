// game_select_app.h
#ifndef GAME_SELECT_APP_H
#define GAME_SELECT_APP_H

#include <stdint.h>
#include <stdbool.h>
#include "system_state.h"
#include "common_colors.h"

// 初始化游戏选择应用
void GameSelectApp_Init(void);

// 更新游戏选择应用
SystemState_t GameSelectApp_Update(GameID_t* selected_game);

// 应用主题颜色
void GameSelectApp_ApplyThemeColors(const AppColors_t* theme_colors);

// 强制重绘
void GameSelectApp_ForceRedraw(void);

// 控制函数
void GameSelectApp_SetDampingStrength(float strength);
void GameSelectApp_SetAnimationDuration(float duration);
float GameSelectApp_GetDampingStrength(void);
float GameSelectApp_GetAnimationDuration(void);
int GameSelectApp_GetCurrentIndex(void);
void GameSelectApp_SetCurrentIndex(int index);
bool GameSelectApp_IsAnimating(void);

#endif // GAME_SELECT_APP_H