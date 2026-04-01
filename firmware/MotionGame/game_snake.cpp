// game_snake.cpp - 贪吃蛇游戏
#include "game_snake.h"
#include "game_app.h"
#include "display_driver.h"
#include "motion_service.h"
#include "output_feedback.h"
#include <Arduino.h>

static SnakeGame_t snake;
static int game_score = 0;
static uint32_t last_haptic_time = 0;

/* ================= 内部函数 ================= */
static void spawn_food(void) {
    snake.food_x = random(4, 124);
    snake.food_y = random(4, 124);
}

static void draw_score(void) {
    char score_text[20];
    snprintf(score_text, sizeof(score_text), "SCORE: %d", game_score);
    
    TextParams_t params = {
        .font = DISPLAY_FONT_5x7,
        .color = 0xFFFF,
        .bg_color = 0x0000,
        .size = 1,
        .h_align = TEXT_ALIGN_LEFT,
        .v_align = TEXT_ALIGN_TOP,
        .transparent = true
    };
    Display_DrawString(5, 5, score_text, &params);
}

/* ================= 游戏初始化 ================= */
void GameSnake_Init(void) {
    snake.length = 3;
    for (int i = 0; i < snake.length; i++) {
        snake.x[i] = 64 - i * SNAKE_GRID_SIZE;
        snake.y[i] = 32;
    }
    snake.direction = 0;  // 向右
    snake.last_move = millis();
    game_score = 0;
    last_haptic_time = 0;
    
    spawn_food();
    Serial.println("Snake Game Init");
}

/* ================= 游戏更新 ================= */
SystemState_t GameSnake_Update(void) {
    // 处理方向控制（使用 IMU_me.motion）
    switch (IMU_me.motion) {
        case MOTION_TILT_LEFT:
            if (snake.direction != 0) snake.direction = 2;  // 左
            break;
        case MOTION_TILT_RIGHT:
            if (snake.direction != 2) snake.direction = 0;  // 右
            break;
        case MOTION_ROLL_FRONT:
            if (snake.direction != 1) snake.direction = 3;  // 上
            break;
        case MOTION_ROLL_BACK:
            if (snake.direction != 3) snake.direction = 1;  // 下
            break;
        default:
            break;
    }
    
    // 非阻塞移动
    if (millis() - snake.last_move >= SNAKE_MOVE_INTERVAL) {
        snake.last_move = millis();
        
        // 移动蛇身
        for (int i = snake.length - 1; i > 0; i--) {
            snake.x[i] = snake.x[i-1];
            snake.y[i] = snake.y[i-1];
        }
        
        // 移动蛇头
        switch (snake.direction) {
            case 0: snake.x[0] += SNAKE_GRID_SIZE; break;
            case 1: snake.y[0] += SNAKE_GRID_SIZE; break;
            case 2: snake.x[0] -= SNAKE_GRID_SIZE; break;
            case 3: snake.y[0] -= SNAKE_GRID_SIZE; break;
        }
        
        // 边界检测（穿墙）
        if (snake.x[0] < 0) snake.x[0] = 124;
        if (snake.x[0] >= 128) snake.x[0] = 0;
        if (snake.y[0] < 0) snake.y[0] = 124;
        if (snake.y[0] >= 128) snake.y[0] = 0;
        
        // ★ 自碰撞检测（撞到自己 Game Over）
        for (int i = 1; i < snake.length; i++) {
            if (snake.x[0] == snake.x[i] && snake.y[0] == snake.y[i]) {
                Output_Feedback(FEEDBACK_DIE);
                delay(500);
                return SYS_GAME_SELECT;
            }
        }
        
        // 吃食物检测
        if (abs(snake.x[0] - snake.food_x) < SNAKE_GRID_SIZE && 
            abs(snake.y[0] - snake.food_y) < SNAKE_GRID_SIZE) {
            snake.length++;
            game_score += 10;
            spawn_food();
            
            // 震动反馈
            if (millis() - last_haptic_time > 100) {
                Output_Feedback(FEEDBACK_EAT);
                last_haptic_time = millis();
            }
            
            // ★ 每吃5个食物升级反馈
            if (game_score % 50 == 0) {
                Output_Feedback(FEEDBACK_LEVEL_UP);
            }
        }
    }
    
    // 绘制游戏画面
    Display_ClearBuffer(0x0000);
    
    // 绘制蛇
    for (int i = 0; i < snake.length; i++) {
        uint16_t color = (i == 0) ? 0x07FF : 0x07E0;  // 蛇头青色，身体绿色
        Display_FillRect(snake.x[i], snake.y[i], SNAKE_GRID_SIZE, SNAKE_GRID_SIZE, color);
    }
    
    // 绘制食物
    Display_FillRect(snake.food_x, snake.food_y, SNAKE_GRID_SIZE, SNAKE_GRID_SIZE, 0xF800);
    
    // 绘制分数
    draw_score();
    
    Display_EndFrame();
    
    return SYS_NONE;
}

/* ================= 获取分数 ================= */
int GameSnake_GetScore(void) {
    return game_score;
}