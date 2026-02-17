// game_app.cpp
#include "game_app.h"
#include "game_balance.h"
#include "display_driver.h"
#include "motion_service.h"
#include "cloud_api.h"
#include <Arduino.h>

// 当前运行的游戏ID
static GameID_t current_game = GAME_NONE;

// 游戏状态
static bool game_is_running = false;
static bool game_is_paused = false;
static int game_score = 0;
static int game_high_score = 0;

// 简单的贪吃蛇游戏实现
static int snake_x[100];
static int snake_y[100];
static int snake_length = 3;
static int food_x = 0;
static int food_y = 0;
static int direction = 0; // 0:右, 1:下, 2:左, 3:上

static void Snake_Init(void) {
    snake_length = 3;
    for (int i = 0; i < snake_length; i++) {
        snake_x[i] = 64 - i * 4;
        snake_y[i] = 32;
    }
    food_x = random(4, 124);
    food_y = random(4, 124);
    direction = 0;
    game_score = 0;
}

static void Snake_Update(void) {
    // 移动蛇身
    for (int i = snake_length - 1; i > 0; i--) {
        snake_x[i] = snake_x[i-1];
        snake_y[i] = snake_y[i-1];
    }
    
    // 移动蛇头
    switch (direction) {
        case 0: snake_x[0] += 4; break; // 右
        case 1: snake_y[0] += 4; break; // 下
        case 2: snake_x[0] -= 4; break; // 左
        case 3: snake_y[0] -= 4; break; // 上
    }
    
    // 边界检查
    if (snake_x[0] < 0) snake_x[0] = 124;
    if (snake_x[0] >= 128) snake_x[0] = 0;
    if (snake_y[0] < 0) snake_y[0] = 124;
    if (snake_y[0] >= 128) snake_y[0] = 0;
    
    // 吃食物检查
    if (abs(snake_x[0] - food_x) < 4 && abs(snake_y[0] - food_y) < 4) {
        snake_length++;
        game_score += 10;
        food_x = random(4, 124);
        food_y = random(4, 124);
    }
}

static void Snake_Draw(void) {
    Display_ClearBuffer(0x0000);
    
    // 绘制蛇
    for (int i = 0; i < snake_length; i++) {
        Display_FillRect(snake_x[i], snake_y[i], 4, 4, 0x07E0);
    }
    
    // 绘制食物
    Display_FillRect(food_x, food_y, 4, 4, 0xF800);
    
    // 绘制分数
    TextParams_t params = {
        .font = DISPLAY_FONT_5x7,
        .color = 0xFFFF,
        .bg_color = 0x0000,
        .size = 1,
        .h_align = TEXT_ALIGN_LEFT,
        .v_align = TEXT_ALIGN_TOP,
        .transparent = true
    };
    
    char score_text[20];
    snprintf(score_text, sizeof(score_text), "SCORE: %d", game_score);
    Display_DrawString(5, 5, score_text, &params);
}

// 俄罗斯方块简单实现
static int tetris_board[10][20];
static int current_piece = 0;
static int piece_x = 4;
static int piece_y = 0;

static void Tetris_Init(void) {
    for (int x = 0; x < 10; x++) {
        for (int y = 0; y < 20; y++) {
            tetris_board[x][y] = 0;
        }
    }
    current_piece = random(0, 7);
    piece_x = 4;
    piece_y = 0;
    game_score = 0;
}

static void Tetris_Update(void) {
    piece_y++;
    if (piece_y > 18) {
        piece_y = 0;
        current_piece = random(0, 7);
    }
}

static void Tetris_Draw(void) {
    Display_ClearBuffer(0x0000);
    Display_DrawRect(20, 10, 80, 120, 0xFFFF);
    
    TextParams_t params = {
        .font = DISPLAY_FONT_5x7,
        .color = 0xFFFF,
        .bg_color = 0x0000,
        .size = 1,
        .h_align = TEXT_ALIGN_CENTER,
        .v_align = TEXT_ALIGN_TOP,
        .transparent = true
    };
    Display_DrawString(64, 5, "TETRIS", &params);
}

// 通用游戏绘制
static void GameApp_Draw(void) {
    Display_StartFrame();
    
    switch (current_game) {
        case GAME_SNAKE:
            Snake_Draw();
            break;
        case GAME_TETRIS:
            Tetris_Draw();
            break;
        default:
            Display_ClearBuffer(0x0000);
            TextParams_t params = {
                .font = DISPLAY_FONT_5x7,
                .color = 0xFFFF,
                .bg_color = 0x0000,
                .size = 2,
                .h_align = TEXT_ALIGN_CENTER,
                .v_align = TEXT_ALIGN_MIDDLE,
                .transparent = true
            };
            const char* game_name = "GAME";
            switch (current_game) {
                case GAME_PUZZLE: game_name = "PUZZLE"; break;
                case GAME_FLAPPY: game_name = "FLAPPY BIRD"; break;
                default: break;
            }
            Display_DrawString(64, 64, game_name, &params);
            Display_DrawString(64, 90, "COMING SOON", &params);
            break;
    }
    
    if (game_is_paused) {
        TextParams_t pause_params = {
            .font = DISPLAY_FONT_5x7,
            .color = 0xFFFF,
            .bg_color = 0x0000,
            .size = 1,
            .h_align = TEXT_ALIGN_CENTER,
            .v_align = TEXT_ALIGN_BOTTOM,
            .transparent = true
        };
        Display_DrawString(64, 150, "PAUSED - SHAKE TO RESUME", &pause_params);
    }
    
    TextParams_t hint_params = {
        .font = DISPLAY_FONT_5x7,
        .color = 0x7BEF,
        .bg_color = 0x0000,
        .size = 1,
        .h_align = TEXT_ALIGN_CENTER,
        .v_align = TEXT_ALIGN_BOTTOM,
        .transparent = true
    };
    
    if (!game_is_paused) {
        Display_DrawString(64, 155, "DOUBLE TAP: PAUSE", &hint_params);
    }
    Display_DrawString(64, 165, "LONG PRESS: EXIT", &hint_params);
    
    Display_EndFrame();
}

// 接口函数
void GameApp_Init(GameID_t game_id) {
    current_game = game_id;
    game_is_running = true;
    game_is_paused = false;
    game_score = 0;
    
    switch (game_id) {
        case GAME_SNAKE:
            Snake_Init();
            break;
        case GAME_TETRIS:
            Tetris_Init();
            break;
        case GAME_BALANCE:
            GameBalance_Init();
            break;
        default:
            break;
    }
    
    Display_ClearScreen(0x0000);
}

SystemState_t GameApp_Update(void) {
    if (!game_is_running) {
        return SYS_NONE;
    }
    
    MotionType_t motion = MotionService_Update();
    
    // 处理游戏输入
    if (!game_is_paused) {
        switch (current_game) {
            case GAME_SNAKE:
                switch (motion) {
                    case MOTION_TILT_LEFT:
                        if (direction != 0) direction = 2;
                        break;
                    case MOTION_TILT_RIGHT:
                        if (direction != 2) direction = 0;
                        break;
                    case MOTION_ROLL_FRONT:
                        if (direction != 1) direction = 3;
                        break;
                    case MOTION_ROLL_BACK:
                        if (direction != 3) direction = 1;
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
    
    // // 处理控制输入
    // switch (motion) {
    //     case MOTION_DOUBLE_TAP:
    //         if (game_is_paused) {
    //             GameApp_Resume();
    //         } else {
    //             GameApp_Pause();
    //         }
    //         break;
            
    //     case MOTION_LONG_PRESS:
    //         return SYS_GAME_SELECT;
            
    //     case MOTION_SHAKE:
    //         if (game_is_paused) {
    //             GameApp_Resume();
    //         }
    //         break;
            
    //     default:
    //         break;
    // }
    
    // 平衡球游戏有独立的 Update 和 Draw
    if (current_game == GAME_BALANCE) {
        return GameBalance_Update();
    }
    
    if (!game_is_paused) {
        switch (current_game) {
            case GAME_SNAKE:
                Snake_Update();
                break;
            case GAME_TETRIS:
                Tetris_Update();
                break;
            default:
                break;
        }
    }
    
    GameApp_Draw();
    return SYS_NONE;
}

void GameApp_Pause(void) {
    game_is_paused = true;
}

void GameApp_Resume(void) {
    game_is_paused = false;
}

int GameApp_GetScore(void) {
    return game_score;
}

int GameApp_GetHighScore(void) {
    return game_high_score;
}

void GameApp_Reset(void) {
    game_is_running = false;
    GameApp_Init(current_game);
}