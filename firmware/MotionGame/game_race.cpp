#include "game_race.h"
#include "display_driver.h"
#include "motion_service.h"
#include "system_state.h"
#include <Arduino.h> 
#include <string.h>
#include <math.h>
#include "motor.h"
#include "output_feedback.h"
static RaceGame_t game;

// ==================== 数学与颜色工具 ====================
static uint32_t fast_seed = 12345;
static float randFloat(float min, float max) {
    fast_seed = fast_seed * 1103515245 + 12345;
    float r = (float)(fast_seed >> 16) / (float)0x7FFF;
    return min + r * (max - min);
}

static float easeInOut(float a, float b, float p) { 
    return a + (b - a) * (-0.5f * cosf(M_PI * p) + 0.5f); 
}

// 修复了负数溢出 Bug 的完美 Lerp
static uint16_t lerpColor(uint16_t c1, uint16_t c2, float t) {
    if (t <= 0.0f) return c1;
    if (t >= 1.0f) return c2;
    uint8_t r1 = (c1 >> 11) & 0x1F;
    uint8_t g1 = (c1 >> 5)  & 0x3F;
    uint8_t b1 = c1         & 0x1F;
    uint8_t r2 = (c2 >> 11) & 0x1F;
    uint8_t g2 = (c2 >> 5)  & 0x3F;
    uint8_t b2 = c2         & 0x1F;
    uint8_t r = (uint8_t)(r1 + (r2 - r1) * t);
    uint8_t g = (uint8_t)(g1 + (g2 - g1) * t);
    uint8_t b = (uint8_t)(b1 + (b2 - b1) * t);
    return (r << 11) | (g << 5) | b;
}

// ==================== 几何UI工具 ====================
static void drawRing(int cx, int cy, float r_outer, float r_inner, uint16_t color) {
    int segments = 16; 
    float step = (2.0f * M_PI) / segments;
    for (int i = 0; i < segments; i++) {
        float a1 = i * step;
        float a2 = (i + 1) * step;
        int x1_o = cx + (int)(cosf(a1) * r_outer);
        int y1_o = cy + (int)(sinf(a1) * r_outer);
        int x1_i = cx + (int)(cosf(a1) * r_inner);
        int y1_i = cy + (int)(sinf(a1) * r_inner);
        int x2_o = cx + (int)(cosf(a2) * r_outer);
        int y2_o = cy + (int)(sinf(a2) * r_outer);
        int x2_i = cx + (int)(cosf(a2) * r_inner);
        int y2_i = cy + (int)(sinf(a2) * r_inner);
        Display_FillTriangle(x1_o, y1_o, x2_o, y2_o, x1_i, y1_i, color);
        Display_FillTriangle(x1_i, y1_i, x2_o, y2_o, x2_i, y2_i, color);
    }
}

static void drawRotatedRect(int cx, int cy, int w, int h, float angle, uint16_t color) {
    float s = sinf(angle);
    float c = cosf(angle);
    float hw = w / 2.0f;
    float hh = h / 2.0f;
    int16_t x1 = cx + (int16_t)(-hw * c - (-hh) * s);
    int16_t y1 = cy + (int16_t)(-hw * s + (-hh) * c);
    int16_t x2 = cx + (int16_t)(hw * c - (-hh) * s);
    int16_t y2 = cy + (int16_t)(hw * s + (-hh) * c);
    int16_t x3 = cx + (int16_t)(hw * c - hh * s);
    int16_t y3 = cy + (int16_t)(hw * s + hh * c);
    int16_t x4 = cx + (int16_t)(-hw * c - hh * s);
    int16_t y4 = cy + (int16_t)(-hw * s + hh * c);
    Display_FillTriangle(x1, y1, x2, y2, x3, y3, color);
    Display_FillTriangle(x1, y1, x3, y3, x4, y4, color);
}

// ==================== 动态环境生成 ====================
static void updateEnvironment(void) {
    game.time_of_day += 0.005f; 
    if (game.time_of_day >= 24.0f) game.time_of_day -= 24.0f;
    float t = 0.0f;

    if (game.time_of_day < 4.0f) {
        game.current_sky_top = SKY_NIGHT_TOP;
        game.current_sky_bot = SKY_NIGHT_BOT;
    } else if (game.time_of_day < 6.0f) {
        t = (game.time_of_day - 4.0f) / 2.0f; 
        game.current_sky_top = lerpColor(SKY_NIGHT_TOP, SKY_RISE_TOP, t);
        game.current_sky_bot = lerpColor(SKY_NIGHT_BOT, SKY_RISE_BOT, t);
    } else if (game.time_of_day < 8.0f) {
        t = (game.time_of_day - 6.0f) / 2.0f;
        game.current_sky_top = lerpColor(SKY_RISE_TOP, SKY_NOON_TOP, t);
        game.current_sky_bot = lerpColor(SKY_RISE_BOT, SKY_NOON_BOT, t);
    } else if (game.time_of_day < 16.0f) {
        game.current_sky_top = SKY_NOON_TOP;
        game.current_sky_bot = SKY_NOON_BOT;
    } else if (game.time_of_day < 18.0f) {
        t = (game.time_of_day - 16.0f) / 2.0f;
        game.current_sky_top = lerpColor(SKY_NOON_TOP, SKY_SET_TOP, t);
        game.current_sky_bot = lerpColor(SKY_NOON_BOT, SKY_SET_BOT, t);
    } else if (game.time_of_day < 20.0f) {
        t = (game.time_of_day - 18.0f) / 2.0f;
        game.current_sky_top = lerpColor(SKY_SET_TOP, SKY_NIGHT_TOP, t);
        game.current_sky_bot = lerpColor(SKY_SET_BOT, SKY_NIGHT_BOT, t);
    } else {
        game.current_sky_top = SKY_NIGHT_TOP;
        game.current_sky_bot = SKY_NIGHT_BOT;
    }
}

static void drawSkyGradient(void) {
    int horizon_y = RACE_SCREEN_H / 2 + 20; 
    for (int y = 0; y < horizon_y; y++) {
        float t = (float)y / (float)horizon_y;
        uint16_t line_color = lerpColor(game.current_sky_top, game.current_sky_bot, t);
        Display_DrawLine(0, y, RACE_SCREEN_W - 1, y, line_color);
    }
    Display_FillRect(0, horizon_y, RACE_SCREEN_W, RACE_SCREEN_H - horizon_y, game.current_sky_bot);
}

static void drawSkyObjects(void) {
    if (game.time_of_day >= 6.0f && game.time_of_day <= 18.0f) {
        float sun_angle = ((game.time_of_day - 6.0f) / 12.0f) * M_PI; 
        int sun_y = RACE_SCREEN_H - 10 - (int)(sinf(sun_angle) * 140.0f); 
        int sun_x = (RACE_SCREEN_W / 2) - (int)(game.player_x * 0.005f); 
        Display_FillCircle(sun_x, sun_y, 14, 0xFFE0); 
    }
    if (game.time_of_day > 18.0f || game.time_of_day < 6.0f) {
        float moon_time = (game.time_of_day > 18.0f) ? (game.time_of_day - 18.0f) : (game.time_of_day + 6.0f);
        float moon_angle = (moon_time / 12.0f) * M_PI;
        int moon_y = RACE_SCREEN_H - 10 - (int)(sinf(moon_angle) * 140.0f);
        int moon_x = (RACE_SCREEN_W / 2) - (int)(game.player_x * 0.005f);
        Display_FillCircle(moon_x, moon_y, 12, TFT_WHITE);
    }
    if (game.time_of_day > 19.5f || game.time_of_day < 4.5f) {
        for (int i = 0; i < 20; i++) {
            int star_x = (i * 47 - (int)(game.player_x * 0.008f)) % RACE_SCREEN_W;
            if (star_x < 0) star_x += RACE_SCREEN_W;
            int star_y = (i * 13) % (RACE_SCREEN_H / 2 - 20);
            if ((i + (int)(game.time_of_day * 4)) % 5 != 0) { 
                Display_DrawPixel(star_x, star_y, TFT_WHITE);
            }
        }
    }
}

static void generateNextSegment(RaceSegment_t* seg) {
    if (game.gen_count >= game.gen_total) {
        game.gen_phase = (game.gen_phase + 1) % 3;
        game.gen_count = 0;
        if (game.gen_phase == 0) { 
            game.cur_curve = 0.0f;  
            game.cur_hill  = 0.0f;  
            game.next_curve = (randFloat(0,1) > 0.5f) ? randFloat(-8.0f, 8.0f) : 0.0f;
            game.next_hill = (randFloat(0,1) > 0.5f) ? randFloat(-1000.0f, 1200.0f) : 0.0f;
            game.gen_total = 10 + (int)randFloat(0, 20); 
        } else if (game.gen_phase == 1) { 
            game.gen_total = 15 + (int)randFloat(0, 25);
        } else { 
            game.gen_total = 10 + (int)randFloat(0, 20);
        }
    }
    float p = (float)game.gen_count / (float)game.gen_total;
    if (game.gen_phase == 0) {
        seg->curve = easeInOut(game.cur_curve, game.next_curve, p);
        seg->y = easeInOut(game.cur_hill, game.next_hill, p);
    } else if (game.gen_phase == 1) {
        seg->curve = game.next_curve;
        seg->y = game.next_hill;
    } else {
        seg->curve = easeInOut(game.next_curve, 0.0f, p);
        seg->y = easeInOut(game.next_hill, 0.0f, p);
    }
    game.gen_count++;
}

static void project(float world_x, float world_y, float world_z, float current_fov,
                   float* screen_x, float* screen_y, float* screen_w) {
    float d = 1.0f / tanf((current_fov * 0.5f) * M_PI / 180.0f);
    float scale = d / world_z;
    *screen_x = (RACE_SCREEN_W / 2.0f) * (1.0f + world_x * scale);
    *screen_y = (RACE_SCREEN_H / 2.0f) * (1.0f - world_y * scale);
    *screen_w = (RACE_SCREEN_W / 2.0f) * (RACE_ROAD_WIDTH * scale);
}

static void drawQuad(float x1, float x2, float y1, float y2, float w1, float w2, uint16_t color) {
    if (y1 < 0 && y2 < 0) return;
    if (y1 >= RACE_SCREEN_H && y2 >= RACE_SCREEN_H) return;
    Display_FillTriangle((int16_t)(x1-w1), (int16_t)y1, (int16_t)(x1+w1), (int16_t)y1, (int16_t)(x2-w2), (int16_t)y2, color);
    Display_FillTriangle((int16_t)(x1+w1), (int16_t)y1, (int16_t)(x2+w2), (int16_t)y2, (int16_t)(x2-w2), (int16_t)y2, color);
}

// ==================== 核心渲染引擎 ====================
static void render(void) {
    drawSkyGradient();
    drawSkyObjects();

    float dynamic_fov = game.fov + (game.speed / game.max_speed) * 20.0f;
    float percent = game.cam_z / RACE_SEGMENT_LENGTH;
    float road_y = game.segments[0].y + (game.segments[1].y - game.segments[0].y) * percent;
    float cam_y = game.cam_height + road_y;
    
    if (game.speed > 10.0f) {
        float shake = (game.speed / game.max_speed) * 4.0f; 
        cam_y += randFloat(-shake, shake);
    }
    
    float slope = game.segments[1].y - game.segments[0].y;
    float terrain_pitch = slope * 0.05f; 
    float inertia_pitch = -game.accel * 0.5f; 
    float final_pitch_offset = terrain_pitch + inertia_pitch;
    
    float current_dx = -(game.segments[0].curve * percent); 
    float current_x = 0.0f;
    
    float max_y = (float)RACE_SCREEN_H; 
    float clip_z = 1.0f; 
    
    for (uint16_t n = 1; n < RACE_DRAW_DISTANCE; n++) {
        RaceSegment_t* prev = &game.segments[n - 1];
        RaceSegment_t* curr = &game.segments[n];
        
        float pz = (n - 1) * RACE_SEGMENT_LENGTH - game.cam_z;
        float cz = n * RACE_SEGMENT_LENGTH - game.cam_z;
        if (cz < clip_z) continue; 

        float py = prev->y, cy = curr->y;
        float px = -current_x - game.player_x; 
        current_dx += prev->curve;             
        current_x += current_dx;               
        float cx = -current_x - game.player_x; 

        if (pz < clip_z) { 
            float t = (clip_z - pz) / (cz - pz);
            pz = clip_z;
            py = py + (cy - py) * t;
            px = px + (cx - px) * t;
        }
        
        float sx1, sy1, sw1, sx2, sy2, sw2;
        project(px, py - cam_y, pz, dynamic_fov, &sx1, &sy1, &sw1);
        project(cx, cy - cam_y, cz, dynamic_fov, &sx2, &sy2, &sw2);
        
        sy1 += final_pitch_offset;
        sy2 += final_pitch_offset;
        
        if (sy1 > max_y) {
            float t = (max_y - sy1) / (sy2 - sy1);
            sx1 = sx1 + (sx2 - sx1) * t;
            sw1 = sw1 + (sw2 - sw1) * t;
            sy1 = max_y;
        }
        if (sy2 >= max_y) continue; 
        max_y = sy2;
        
        uint32_t abs_idx = game.global_idx + n;
        uint16_t grass = (abs_idx / 3) % 2 ? RACE_COLOR_GRASS1 : RACE_COLOR_GRASS2;
        uint16_t road  = (abs_idx / 3) % 2 ? RACE_COLOR_ROAD1  : RACE_COLOR_ROAD2;
        uint16_t rum   = (abs_idx / 9) % 2 ? RACE_COLOR_RUMBLE1 : RACE_COLOR_RUMBLE2;
        uint16_t line  = (abs_idx / 6) % 2 ? 0 : RACE_COLOR_LINE;

        drawQuad(RACE_SCREEN_W/2.0f, RACE_SCREEN_W/2.0f, sy1, sy2, RACE_SCREEN_W, RACE_SCREEN_W, grass);
        drawQuad(sx1, sx2, sy1, sy2, sw1 * 1.15f, sw2 * 1.15f, rum);
        drawQuad(sx1, sx2, sy1, sy2, sw1, sw2, road);
        if (line) drawQuad(sx1, sx2, sy1, sy2, sw1 * 0.05f, sw2 * 0.05f, line);
    }
}

// ==================== 街机风 UI 系统 ====================
static void drawDashboard(void) {
    extern IMU IMU_me;
    
    // ★ 修正：所有的UI随动也使用扣除基准后的相对倾斜角
    float rel_pitch = IMU_me.pitch - game.base_pitch;
    
    int head_roll_offset = (int)(game.segments[0].curve * 1.5f);
    int hood_top_y = RACE_SCREEN_H - 45;
    int dash_y = RACE_SCREEN_H - 32; 
    int h_cx = (RACE_SCREEN_W / 2) + head_roll_offset;
    int w_top = 70, w_bot = 160;
    
    Display_FillTriangle(h_cx - w_top/2, hood_top_y, h_cx + w_top/2, hood_top_y, h_cx - w_bot/2, dash_y, 0xA000); 
    Display_FillTriangle(h_cx + w_top/2, hood_top_y, h_cx + w_bot/2, dash_y, h_cx - w_bot/2, dash_y, 0xA000);
    Display_FillTriangle(h_cx - 10, hood_top_y + 5, h_cx + 10, hood_top_y + 5, h_cx + 15, dash_y, 0x0000);
    Display_FillTriangle(h_cx - 10, hood_top_y + 5, h_cx + 15, dash_y, h_cx - 15, dash_y, 0x0000);

    Display_FillRect(-20 + head_roll_offset, dash_y, RACE_SCREEN_W + 40, 32, 0x2945); 
    Display_FillRect(-20 + head_roll_offset, dash_y, RACE_SCREEN_W + 40, 2, 0x07FF); 

    int rpm_cx = 30 + head_roll_offset;
    int meter_cy = RACE_SCREEN_H - 2;
    int meter_r = 22;
    
    Display_FillCircle(rpm_cx, meter_cy, meter_r, 0x0000);
    Display_DrawCircle(rpm_cx, meter_cy, meter_r, 0xCE79);
    
    float rpm_ratio = ((int)game.speed % 80) / 80.0f;
    if (game.speed < 5.0f) rpm_ratio = 0.0f;
    float angle_rpm = M_PI - (rpm_ratio * M_PI * 0.8f); 
    int nx1 = rpm_cx + (int)(cosf(angle_rpm) * (meter_r - 4));
    int ny1 = meter_cy - (int)(sinf(angle_rpm) * (meter_r - 4));
    
    Display_DrawLine(rpm_cx, meter_cy, nx1, ny1, 0xFFE0);
    Display_FillCircle(rpm_cx, meter_cy, 3, 0xFFFF);
    
    int spd_cx = RACE_SCREEN_W - 30 + head_roll_offset;
    Display_FillCircle(spd_cx, meter_cy, meter_r, 0x0000);
    Display_DrawCircle(spd_cx, meter_cy, meter_r, 0xCE79);
    
    float speed_ratio = game.speed / game.max_speed;
    if (speed_ratio > 1.0f) speed_ratio = 1.0f;
    float angle_spd = M_PI - (speed_ratio * M_PI * 0.8f);
    int nx2 = spd_cx + (int)(cosf(angle_spd) * (meter_r - 4));
    int ny2 = meter_cy - (int)(sinf(angle_spd) * (meter_r - 4));
    
    Display_DrawLine(spd_cx, meter_cy, nx2, ny2, 0xF800);
    Display_FillCircle(spd_cx, meter_cy, 3, 0xFFFF);

    Display_SetTextColor(0xFFFF, 0x0000); 
    Display_SetTextSize(1);                     
    int display_spd = (int)(game.speed * 0.5f); 
    Display_DrawTextF(spd_cx - 10, meter_cy - 12, "%3d", display_spd);

    // ★ 修正：方向盘绘制使用相对倾斜角
    int wheel_cx = (RACE_SCREEN_W / 2) + (int)(rel_pitch * 0.4f) + head_roll_offset;
    int wheel_cy = RACE_SCREEN_H - 14; 
    float wheel_angle = rel_pitch * 0.05f; 

    Display_FillRect(wheel_cx - 8, wheel_cy, 16, 16, 0xCE79);
    drawRing(wheel_cx, wheel_cy, 26.0f, 20.0f, 0x0000);      
    drawRing(wheel_cx, wheel_cy, 24.0f, 22.0f, 0xCE79);  
    drawRotatedRect(wheel_cx, wheel_cy, 48, 8, wheel_angle, 0x0000); 
    drawRotatedRect(wheel_cx, wheel_cy, 44, 4, wheel_angle, 0xCE79); 
    drawRotatedRect(wheel_cx, wheel_cy, 14, 12, wheel_angle, 0x0000);
    drawRotatedRect(wheel_cx, wheel_cy, 8, 6, wheel_angle, 0xF800); 
}

// ==================== 物理与输入处理 ====================
static void handleInput(void) {
    extern IMU IMU_me;
    float old_speed = game.speed;
    
    float rel_roll = IMU_me.roll - game.base_roll;
    float rel_pitch = IMU_me.pitch - game.base_pitch;
    
    // =======================================================
    // 1. 速度控制 (基于倾斜角度的线性油门/刹车控制)
    // =======================================================
    float deadzone_roll = 5.0f; // 设定 5 度的死区，防止手部自然微抖导致加减速
    
    if (rel_roll > deadzone_roll) {
        // 前倾踩油门：超过死区后，倾斜越厉害，加速度(油门)越大
        // 乘数 0.5f 是“马力系数”。假设你前倾 35 度，单帧提速就是 (35-5)*0.5 = 15.0f
        float throttle = (rel_roll - deadzone_roll) * 0.5f; 
        game.speed += throttle;
    } 
    else if (rel_roll < -deadzone_roll) {
        // 后仰踩刹车：刹车通常比油门灵敏，所以乘数设为 1.2f
        float brake = (-rel_roll - deadzone_roll) * 1.2f; 
        game.speed -= brake;
    } 
    else {
        // 处于死区内 (松开油门)，车轮受摩擦力自然怠速减速
        game.speed -= 3.0f; 
    }
    
    // 速度上下限钳制
    if (game.speed > game.max_speed) game.speed = game.max_speed;
    if (game.speed < 0) game.speed = 0;
    
    game.accel = game.speed - old_speed;
    // 2. 转向控制 (使用相对 Pitch)
    float steer_input = rel_pitch; 
    if (steer_input > 10.0f) {
        game.player_x += (steer_input - 10.0f) * game.steering_sensitivity * (game.speed / game.max_speed);
        // ★ 急转弯震动（高速+大力转向时）
        if (game.speed > game.max_speed * 0.5f && steer_input > 30.0f) {
            Output_MotorVibrate(128, 30);
        }
    } else if (steer_input < -10.0f) {
        game.player_x += (steer_input + 10.0f) * game.steering_sensitivity * (game.speed / game.max_speed);
        // ★ 急转弯震动（高速+大力转向时）
        if (game.speed > game.max_speed * 0.5f && steer_input < -30.0f) {
            Output_MotorVibrate(128, 30);
        }
    } else {
        game.player_x *= 0.7f;
    }
    
    float current_curve = game.segments[0].curve;
    game.player_x -= current_curve * (game.speed * 0.015f);
    
    if (game.player_x > RACE_ROAD_WIDTH * 1.5f) {
        game.player_x = RACE_ROAD_WIDTH * 1.5f;
        game.speed -= 10.0f; 
        // ★ 碰撞反馈
        Output_Feedback(FEEDBACK_HIT);
    }
    if (game.player_x < -RACE_ROAD_WIDTH * 1.5f) {
        game.player_x = -RACE_ROAD_WIDTH * 1.5f;
        game.speed -= 10.0f;
        // ★ 碰撞反馈
        Output_Feedback(FEEDBACK_HIT);
    }
}

// ==================== 生命周期 ====================
void GameRace_HandleSerial(void) {
    if (Serial.available() > 0) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim(); 
        if (cmd.startsWith("SET_FOV=")) { game.fov = cmd.substring(8).toFloat(); }
        else if (cmd.startsWith("SET_H=")) { game.cam_height = cmd.substring(6).toFloat(); }
        else if (cmd.startsWith("SET_SPD=")) { game.max_speed = cmd.substring(8).toFloat(); }
    }
}

void GameRace_Init(void) {
    memset(&game, 0, sizeof(game));
    game.max_speed = 450.0f; 
    game.cam_height = 600.0f; 
    game.fov = 100.0f;        
    game.steering_sensitivity = 1.5f;
    game.time_of_day = 17.0f;
    
    // 初始化时置为等待校准状态，并将车速清零
    game.waiting_to_start = true;
    game.base_roll = 0.0f;
    game.base_pitch = 0.0f;
    game.speed = 0.0f;
    
    for (int i = 0; i < RACE_SEGMENT_COUNT; i++) {
        generateNextSegment(&game.segments[i]);
    }
    
    game.initialized = true;
    game.last_update = millis();
    game.state_timer = millis(); // 记录进入初始化状态的时间
}

SystemState_t GameRace_Update(void) {
    if (!game.initialized) return SYS_GAME_PLAYING;
    
    GameRace_HandleSerial();
    extern IMU IMU_me;
    
    uint32_t now = millis();
    if (now - game.last_update < 33) return SYS_GAME_PLAYING; 
    game.last_update = now;
    
    // ==========================================
    // 状态 A：等待摇晃校准阶段
    // ==========================================
    if (game.waiting_to_start) {
        // 让环境和云朵继续流逝，提供酷炫的待机画面
        updateEnvironment(); 
        render();            
        drawDashboard();     

        // 绘制屏幕中央的半透明/黑色背景文字框
        Display_FillRoundRect(14, 60, 100, 40, 4, 0x0000); 
        
        Display_SetTextColor(0xFFE0, 0x0000); // 亮黄字黑底
        Display_SetTextSize(1);
        Display_DrawTextCenteredSimple(RACE_SCREEN_W / 2, 70, "SHAKE TO START");
        
        Display_SetTextColor(0xCE79, 0x0000); // 灰白字黑底
        Display_DrawTextCenteredSimple(RACE_SCREEN_W / 2, 85, "Hold flat & Shake");

        Display_EndFrame();

        // 【防抖逻辑】：进入界面 1000 毫秒后，才开始检测摇晃，防止切入游戏瞬间误触
        if (IMU_me.motion == MOTION_SHAKE && (millis() - game.state_timer > 1000)) {
            // 将摇晃发生这一瞬间的姿态，锁死为绝对零点基准！
            game.base_roll = IMU_me.roll;
            game.base_pitch = IMU_me.pitch;
            
            game.waiting_to_start = false;
            
            // ★ 游戏开始反馈
            Output_Feedback(FEEDBACK_SUCCESS);
            
            game.state_timer = millis();
        }
        return SYS_GAME_PLAYING;
    }
    
    // ==========================================
    // 状态 B：正式驾驶阶段
    // ==========================================
updateEnvironment();
    handleInput();
    
    game.cam_z += game.speed;
    game.total_distance += game.speed;
    
    while (game.cam_z >= RACE_SEGMENT_LENGTH) {
        game.cam_z -= RACE_SEGMENT_LENGTH;
        game.global_idx++; 
        memmove(&game.segments[0], &game.segments[1], sizeof(RaceSegment_t) * (RACE_SEGMENT_COUNT - 1));
        generateNextSegment(&game.segments[RACE_SEGMENT_COUNT - 1]);
    }
    
    render();          
    drawDashboard();   
    Display_EndFrame(); 
    
    // ==========================================
    // ★ 核心：马达引擎震动反馈 (Haptic Feedback)
    // ==========================================
    // 1. 计算当前速度的百分比 (0.0f 到 1.0f)
    float speed_ratio = game.speed / game.max_speed;
    if (speed_ratio > 1.0f) speed_ratio = 1.0f;
    if (speed_ratio < 0.0f) speed_ratio = 0.0f;
    
    // 2. 映射到 0~255 的 PWM 范围
    uint8_t motor_pwm = (uint8_t)(speed_ratio * 255.0f);
    
    // 3. 驱动马达 (你可以根据实际接线改为 MOTOR_A 或 MOTOR_B)
    // 建议：如果只是想要一点引擎震感，可以把最大值稍微压低，比如 255.0f 改成 180.0f
    Motor_Forward(MOTOR_BOTH, motor_pwm);
    
    // ★ 极速提示音（速度 > 90% 时每1秒响一次）
    static uint32_t last_high_speed_beep = 0;
    if (speed_ratio > 0.9f && millis() - last_high_speed_beep > 1000) {
        Output_Feedback(FEEDBACK_WARNING);
        last_high_speed_beep = millis();
    }
    // ★ 低速提示（速度 < 5% 时提醒）
    static uint32_t last_low_speed_beep = 0;
    if (speed_ratio < 0.05f && millis() - last_low_speed_beep > 2000) {
        Output_BuzzerTone(300, 100, 50);  // 简单单音
        last_low_speed_beep = millis();
    }
    

    // ==========================================
    // 退出防抖与马达安全急停
    // ==========================================
    // if (IMU_me.motion == MOTION_SHAKE && (millis() - game.state_timer > 2000)) {
    //     // ★ 退出前停止所有输出
    //     Output_Stop();
        
    //     return SYS_GAME_SELECT;
    // }
    
    return SYS_GAME_PLAYING;
}

int GameRace_GetScore(void) {
    return (int)(game.total_distance / 1000.0f);
}