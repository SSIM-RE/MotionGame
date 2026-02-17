// game_balance.cpp - 平衡球游戏实现
#include "game_balance.h"
#include "motion_service.h"
#include "display_driver.h"
#include "audio_service.h"
#include "mpu6050.h"
#include <Arduino.h>
#include <math.h>

static BalanceGame_t game;

// 场地偏移（屏幕中央）
#define ARENA_OFFSET_X  9
#define ARENA_OFFSET_Y  25

void GameBalance_Init(void) {
    // 初始化球位置（场地中央）
    game.ball_x = BALANCE_ARENA_WIDTH / 2;
    game.ball_y = BALANCE_ARENA_HEIGHT / 2;
    game.ball_vx = 0;
    game.ball_vy = 0;
    
    // 随机生成终点位置（避开边缘）
    game.goal_x = 15 + random(BALANCE_ARENA_WIDTH - 30);
    game.goal_y = 15 + random(BALANCE_ARENA_HEIGHT - 30);
    
    game.score = 0;
    game.start_time = millis();
    game.state = 0;
    game.level = 1;
    
    Serial.println("Balance Game Started");
    GameBalance_Draw();
    delay(500);
}

SystemState_t GameBalance_Update(void) {
    if (game.state == 1) {
        // 游戏已结束，显示胜利画面
        delay(2000);
        return SYS_GAME_SELECT;
    }
    
    // 1. 读取倾斜角度（使用相对角度）
    MotionService_Update();  // 更新基准
    float roll, pitch;
    MPU6050_GetAngle(&roll, &pitch);
    
    // 获取相对角度（相对于初始位置）
    float rel_roll = roll - IMU_me.ref_roll;
    float rel_pitch = pitch - IMU_me.ref_pitch;
    
    // 2. 根据倾斜更新球速度（倾斜角度映射到速度）
    // 限制最大倾斜角度为 45 度
    // 注意：向前倾 (pitch 正) → 球向上滚 (vy 负)，所以取反
    float speed_factor_x = constrain(rel_pitch, -45, 45) / 45.0f;
    float speed_factor_y = -constrain(rel_roll, -45, 45) / 45.0f;
    
    game.ball_vx += speed_factor_x * BALL_SPEED_BASE * 0.3f;
    game.ball_vy += speed_factor_y * BALL_SPEED_BASE * 0.3f;
    
    // 3. 应用摩擦力
    game.ball_vx *= FRICTION;
    game.ball_vy *= FRICTION;
    
    // 4. 更新球位置
    game.ball_x += game.ball_vx;
    game.ball_y += game.ball_vy;
    
    // 5. 边界检测（碰到边界就反弹）
    if (game.ball_x < BALL_RADIUS) {
        game.ball_x = BALL_RADIUS;
        game.ball_vx = -game.ball_vx * 0.5f;
        Audio_Play(BUZZER_HIT);
    }
    if (game.ball_x > BALANCE_ARENA_WIDTH - BALL_RADIUS) {
        game.ball_x = BALANCE_ARENA_WIDTH - BALL_RADIUS;
        game.ball_vx = -game.ball_vx * 0.5f;
        Audio_Play(BUZZER_HIT);
    }
    if (game.ball_y < BALL_RADIUS) {
        game.ball_y = BALL_RADIUS;
        game.ball_vy = -game.ball_vy * 0.5f;
        Audio_Play(BUZZER_HIT);
    }
    if (game.ball_y > BALANCE_ARENA_HEIGHT - BALL_RADIUS) {
        game.ball_y = BALANCE_ARENA_HEIGHT - BALL_RADIUS;
        game.ball_vy = -game.ball_vy * 0.5f;
        Audio_Play(BUZZER_HIT);
    }
    
    // 6. 胜利检测（球到达终点）
    float dx = game.ball_x - game.goal_x;
    float dy = game.ball_y - game.goal_y;
    float dist = sqrt(dx*dx + dy*dy);
    
    if (dist < GOAL_SIZE / 2 + BALL_RADIUS) {
        // 胜利！
        game.state = 1;
        uint32_t elapsed = (millis() - game.start_time) / 1000;
        
        // 计算分数：基础 100 分 + 时间奖励（60 秒内完成）
        if (elapsed < 60) {
            game.score = 100 + (60 - elapsed) * 3;
        } else {
            game.score = 100;
        }
        
        Serial.printf("Game Won! Score: %d, Time: %ds\n", game.score, elapsed);
        
        // 播放胜利音效
        Audio_Play(BUZZER_ATTACK);
        
        // TODO: Cloud_UploadScore() - 预留联网接口
    }
    
    // 7. 刷新显示
    GameBalance_Draw();
    
    return SYS_NONE;  // 继续游戏
}

void GameBalance_Draw(void) {
    // 清屏
    Display_ClearBuffer(0x0000);
    
    // 画场地边框
    Display_DrawRect(ARENA_OFFSET_X, ARENA_OFFSET_Y, 
                     BALANCE_ARENA_WIDTH, BALANCE_ARENA_HEIGHT, 0xFFFF);
    
    // 画终点（红色区域）
    Display_FillRect(
        ARENA_OFFSET_X + (int)game.goal_x - GOAL_SIZE/2,
        ARENA_OFFSET_Y + (int)game.goal_y - GOAL_SIZE/2,
        GOAL_SIZE, GOAL_SIZE,
        0xF800  // 红色
    );
    
    // 画终点标记（小圆圈）
    Display_DrawCircle(
        ARENA_OFFSET_X + (int)game.goal_x,
        ARENA_OFFSET_Y + (int)game.goal_y,
        GOAL_SIZE/3,
        0xFFFF
    );
    
    // 画球（青色）
    Display_FillCircle(
        ARENA_OFFSET_X + (int)game.ball_x,
        ARENA_OFFSET_Y + (int)game.ball_y,
        BALL_RADIUS,
        0x07FF  // 青色
    );
    
    // 显示分数
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
    
    // 显示时间
    uint32_t elapsed = (millis() - game.start_time) / 1000;
    char time_text[20];
    snprintf(time_text, sizeof(time_text), "Time: %ds", elapsed);
    Display_DrawString(5, 15, time_text, &params);
    
    // 显示提示
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
