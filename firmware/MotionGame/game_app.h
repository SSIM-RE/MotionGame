// game_app.h
#ifndef GAME_APP_H
#define GAME_APP_H

#include "system_state.h"

// 游戏应用接口
void GameApp_Init(GameID_t game_id);
SystemState_t GameApp_Update(void);

// 游戏控制函数
void GameApp_Pause(void);
void GameApp_Resume(void);
int GameApp_GetScore(void);
int GameApp_GetHighScore(void);
void GameApp_Reset(void);

#endif // GAME_APP_H