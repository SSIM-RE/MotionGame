// game_balance.cpp - 平衡球游戏实现
#include "game_balance.h"
#include "motion_service.h"
#include "display_driver.h"
#include "output_feedback.h"
#include "mpu6050.h"
#include <Arduino.h>
#include <math.h>

static BalanceGame_t game;
static uint32_t last_haptic_time = 0;
static bool success_triggered = false;
static uint32_t win_display_end = 0;  // 胜利显示结束时间（非阻塞）

#define ARENA_OFFSET_X  9
#define ARENA_OFFSET_Y  25

void GameBalance_Init(void) {
    game.ball_x = BALANCE_ARENA_WIDTH / 2;
    game.ball_y = BALANCE_ARENA_HEIGHT / 2;
    game.ball_vx = 0;
    game.ball_vy = 0;
    
    game.goal_x = 15 + random(BALANCE_ARENA_WIDTH - 30);
    game.goal_y = 15 + random(BALANCE_ARENA_HEIGHT - 30);
    
    game.score = 0;
    game.start_time = millis();
    game.state = 0;
    game.level = 1;
    
    last_haptic_time = 0;
    success_triggered = false;
    win_display_end = 0;
    
    Serial.println("Balance Game Started");
    GameBalance_Draw();
    delay(500);
}

SystemState_t GameBalance_Update(void) {
    // 胜利状态：等待 2 秒后返回游戏选择
    if (game.state == 1) {
        if (win_display_end == 0) {
            win_display_end = millis() + 2000;
        }
        if (millis() >= win_display_end) {
            win_display_end = 0;
            return SYS_GAME_SELECT;
        }
        return SYS_NONE;
    }
    
    // 使用 IMU_me.rel_pitch 和 IMU_me.rel_roll（连续控制）
    float speed_factor_x = constrain(IMU_me.pitch, -45, 45) / 45.0f;
    float speed_factor_y = -constrain(IMU_me.roll, -45, 45) / 45.0f;
    
    game.ball_vx += speed_factor_x * BALL_SPEED_BASE * 0.3f;
    game.ball_vy += speed_factor_y * BALL_SPEED_BASE * 0.3f;
    
    game.ball_vx *= FRICTION;
    game.ball_vy *= FRICTION;
    
    game.ball_x += game.ball_vx;
    game.ball_y += game.ball_vy;
    
    if (game.ball_x < BALL_RADIUS) {
        game.ball_x = BALL_RADIUS;
        game.ball_vx = -game.ball_vx * 0.5f;
        if (millis() - last_haptic_time > 100) {
            Output_Feedback(FEEDBACK_HIT);
            last_haptic_time = millis();
        }
    }
    if (game.ball_x > BALANCE_ARENA_WIDTH - BALL_RADIUS) {
        game.ball_x = BALANCE_ARENA_WIDTH - BALL_RADIUS;
        game.ball_vx = -game.ball_vx * 0.5f;
        if (millis() - last_haptic_time > 100) {
            Output_Feedback(FEEDBACK_HIT);
            last_haptic_time = millis();
        }
    }
    if (game.ball_y < BALL_RADIUS) {
        game.ball_y = BALL_RADIUS;
        game.ball_vy = -game.ball_vy * 0.5f;
        if (millis() - last_haptic_time > 100) {
            Output_Feedback(FEEDBACK_HIT);
            last_haptic_time = millis();
        }
    }
    if (game.ball_y > BALANCE_ARENA_HEIGHT - BALL_RADIUS) {
        game.ball_y = BALANCE_ARENA_HEIGHT - BALL_RADIUS;
        game.ball_vy = -game.ball_vy * 0.5f;
        if (millis() - last_haptic_time > 100) {
            Output_Feedback(FEEDBACK_HIT);
            last_haptic_time = millis();
        }
    }
    
    float dx = game.ball_x - game.goal_x;
    float dy = game.ball_y - game.goal_y;
    float dist = sqrt(dx*dx + dy*dy);
    
    if (dist < GOAL_SIZE / 2 + BALL_RADIUS) {
        game.state = 1;
        uint32_t elapsed = (millis() - game.start_time) / 1000;
        
        if (elapsed < 60) {
            game.score = 100 + (60 - elapsed) * 3;
        } else {
            game.score = 100;
        }
        
        Serial.printf("Game Won! Score: %d, Time: %ds\n", game.score, elapsed);
        
        if (!success_triggered) {
            Output_Feedback(FEEDBACK_SUCCESS);
            success_triggered = true;
        }
    }
    
    GameBalance_Draw();
    
    return SYS_NONE;
}

void GameBalance_Draw(void) {
    Display_ClearBuffer(0x0000);
    
    Display_DrawRect(ARENA_OFFSET_X, ARENA_OFFSET_Y, 
                     BALANCE_ARENA_WIDTH, BALANCE_ARENA_HEIGHT, 0xFFFF);
    
    Display_FillRect(
        ARENA_OFFSET_X + (int)game.goal_x - GOAL_SIZE/2,
        ARENA_OFFSET_Y + (int)game.goal_y - GOAL_SIZE/2,
        GOAL_SIZE, GOAL_SIZE,
        0xF800
    );
    
    Display_DrawCircle(
        ARENA_OFFSET_X + (int)game.goal_x,
        ARENA_OFFSET_Y + (int)game.goal_y,
        GOAL_SIZE/3,
        0xFFFF
    );
    
    Display_FillCircle(
        ARENA_OFFSET_X + (int)game.ball_x,
        ARENA_OFFSET_Y + (int)game.ball_y,
        BALL_RADIUS,
        0x07FF
    );
    
    TextParams_t params = {
        .font = DISPLAY_FONT_5x7,
        .color = 0xFFFF,
        .bg_color = 0x0000,
        .size = 1,
        .h_align = TEXT_ALIGN_LEFT,
        .v_align = TEXT_ALIGN_TOP,
        .transparent = true
    };
    
    char score_text[30];
    snprintf(score_text, sizeof(score_text), "Score: %d", game.score);
    Display_DrawString(5, 5, score_text, &params);
    
    uint32_t elapsed = (millis() - game.start_time) / 1000;
    char time_text[20];
    snprintf(time_text, sizeof(time_text), "Time: %ds", elapsed);
    Display_DrawString(5, 15, time_text, &params);
    
    if (game.state == 0) {
        TextParams_t hint_params = {
            .font = DISPLAY_FONT_5x7,
            .color = 0x7BEF,
            .bg_color = 0x0000,
            .size = 1,
            .h_align = TEXT_ALIGN_CENTER,
            .v_align = TEXT_ALIGN_BOTTOM,
            .transparent = true
        };
        Display_DrawString(64, 155, "Tilt to move the ball", &hint_params);
    } else {
        TextParams_t win_params = {
            .font = DISPLAY_FONT_5x7,
            .color = 0x07FF,
            .bg_color = 0x0000,
            .size = 2,
            .h_align = TEXT_ALIGN_CENTER,
            .v_align = TEXT_ALIGN_MIDDLE,
            .transparent = true
        };
        Display_DrawString(64, 80, "YOU WIN!", &win_params);
    }
    
    Display_EndFrame();
}

int GameBalance_GetScore(void) {
    return game.score;
}