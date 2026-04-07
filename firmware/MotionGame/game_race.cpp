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

// ==================== 自定义调色板 (RGB565) ====================
#define MY_BROWN     0x7A85 // 棕色 (R15 G20 B5)
#define MY_DARKGREEN 0x03E0 // 暗绿 (R0 G31 B0)
#define MY_GREY      0x8410 // 灰色 (R16 G32 B16)
#define MY_ORANGE    0xFDA0 // 橙色

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
    int sky_x_shift = (int)game.sky_offset;
    
    if (game.time_of_day >= 6.0f && game.time_of_day <= 18.0f) {
        float sun_angle = ((game.time_of_day - 6.0f) / 12.0f) * M_PI; 
        int sun_y = RACE_SCREEN_H - 10 - (int)(sinf(sun_angle) * 140.0f); 
        int sun_x = (RACE_SCREEN_W / 2) + sky_x_shift; 
        Display_FillCircle(sun_x, sun_y, 14, 0xFFE0); 
    }
    if (game.time_of_day > 18.0f || game.time_of_day < 6.0f) {
        float moon_time = (game.time_of_day > 18.0f) ? (game.time_of_day - 18.0f) : (game.time_of_day + 6.0f);
        float moon_angle = (moon_time / 12.0f) * M_PI;
        int moon_y = RACE_SCREEN_H - 10 - (int)(sinf(moon_angle) * 140.0f);
        int moon_x = (RACE_SCREEN_W / 2) + sky_x_shift;
        Display_FillCircle(moon_x, moon_y, 12, TFT_WHITE);
    }
    if (game.time_of_day > 19.5f || game.time_of_day < 4.5f) {
        for (int i = 0; i < 20; i++) {
            int star_x = (i * 47 + sky_x_shift) % RACE_SCREEN_W;
            if (star_x < 0) star_x += RACE_SCREEN_W;
            int star_y = (i * 13) % (RACE_SCREEN_H / 2 - 20);
            if ((i + (int)(game.time_of_day * 4)) % 5 != 0) { 
                Display_DrawPixel(star_x, star_y, TFT_WHITE);
            }
        }
    }
}

// ★ 重构生成器：增加 procedural generation 物体逻辑
static void generateNextSegment(RaceSegment_t* seg) {
    // 1. 生成地形和道路参数
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

    // ★ 2. 生成景物 Sprite (Procedural Sprite Generation)
    seg->object_type = OBJ_NONE;
    // 假设 15% 的概率在该段生成景物
    if (randFloat(0.0f, 1.0f) < 0.15f) {
        seg->object_type = (int8_t)randFloat(1.0f, 4.0f); // 生成 OBJ_TREE_PIN 到 OBJ_ROCK
        seg->object_side = (randFloat(0.0f, 1.0f) > 0.5f) ? 1 : -1; // 随机左右
        seg->object_scale = randFloat(0.8f, 2.0f); // 随机大小
    }
}

// ==================== 渲染核心 (BojanSof 投影) ====================
static void project(float world_x, float world_y, float world_z, float current_fov,
                   float* screen_x, float* screen_y, float* screen_w) {
    float d = 1.0f / tanf((current_fov * 0.5f) * M_PI / 180.0f);
    float scale = d / world_z;
    
    // 摄像机灭点永远钉在屏幕中央，视角偏移完全依赖 world_x
    *screen_x = (RACE_SCREEN_W / 2.0f) + (RACE_SCREEN_W / 2.0f) * world_x * scale;
    *screen_y = (RACE_SCREEN_H / 2.0f) - (RACE_SCREEN_H / 2.0f) * world_y * scale;
    *screen_w = (RACE_SCREEN_W / 2.0f) * (RACE_ROAD_WIDTH * scale);
}

static void drawQuad(float x1, float x2, float y1, float y2, float w1, float w2, uint16_t color) {
    if (y1 < 0 && y2 < 0) return;
    if (y1 >= RACE_SCREEN_H && y2 >= RACE_SCREEN_H) return;
    Display_FillTriangle((int16_t)(x1-w1), (int16_t)y1, (int16_t)(x1+w1), (int16_t)y1, (int16_t)(x2-w2), (int16_t)y2, color);
    Display_FillTriangle((int16_t)(x1+w1), (int16_t)y1, (int16_t)(x2+w2), (int16_t)y2, (int16_t)(x2-w2), (int16_t)y2, color);
}

// ★ 自定义公告板物体绘制工具箱 (几何图形 Sprites)
static void drawPineTree(int x, int y, float scale) {
    int trunk_w = (int)(20.0f * scale);
    int trunk_h = (int)(40.0f * scale);
    int leaves_w = (int)(80.0f * scale);
    int leaves_h = (int)(100.0f * scale);
    
    // 树干 (矩形)
    Display_FillRect(x - trunk_w / 2, y - trunk_h, trunk_w, trunk_h, MY_BROWN);
    // 树冠 (三角形)
    Display_FillTriangle(x, y - trunk_h - leaves_h, x - leaves_w / 2, y - trunk_h, x + leaves_w / 2, y - trunk_h, MY_DARKGREEN);
}

static void drawPalmTree(int x, int y, float scale) {
    int trunk_w = (int)(15.0f * scale);
    int trunk_h = (int)(120.0f * scale);
    int leaves_r = (int)(50.0f * scale);
    
    // 树干 (矩形)
    Display_FillRect(x - trunk_w / 2, y - trunk_h, trunk_w, trunk_h, 0xCE79);
    // 树叶 (圆环几何图形替代)
    drawRing(x, y - trunk_h, (float)leaves_r, leaves_r * 0.2f, MY_DARKGREEN);
    drawRing(x, y - trunk_h, leaves_r * 0.8f, leaves_r * 0.3f, 0x07E0);
}


static void drawRoadSign(int x, int y, float scale, int side) {
    int post_w = (int)(10.0f * scale);
    int post_h = (int)(90.0f * scale);
    int board_w = (int)(70.0f * scale);
    int board_h = (int)(45.0f * scale);
    
    // 柱子 (矩形)
    Display_FillRect(x - post_w / 2, y - post_h, post_w, post_h, 0xFFFF);
    
    // 牌子 (根据侧边向外突出)
    int board_x = (side > 0) ? (x) : (x - board_w);
    Display_FillRect(board_x, y - post_h, board_w, board_h, MY_ORANGE);
    Display_DrawRect(board_x, y - post_h, board_w, board_h, 0x0000);
}

// ★ 核心重构：双通道渲染引擎 (通道1：道路从近到远，通道2：景物从远到近)
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

    // ===================================================================
    // 通道 1：渲染地面和道路 (从近到远，画家算法处理天空遮挡)
    // ===================================================================
    for (uint16_t n = 1; n < RACE_DRAW_DISTANCE; n++) {
        RaceSegment_t* prev = &game.segments[n - 1];
        RaceSegment_t* curr = &game.segments[n];
        
        float pz = (n - 1) * RACE_SEGMENT_LENGTH - game.cam_z;
        float cz = n * RACE_SEGMENT_LENGTH - game.cam_z;
        if (cz < clip_z) continue; 

        // 强行弯曲道路的物理 X 坐标
        float py = prev->y, cy = curr->y;
        current_dx += prev->curve;             
        current_x += current_dx;               

        if (pz < clip_z) { 
            float t = (clip_z - pz) / (cz - pz);
            pz = clip_z;
            py = py + (cy - py) * t;
        }
        
        // 渲染世界坐标系 X = -(道路累加弯曲度 + 玩家横向偏移)
        float px = -current_x - game.player_x; 
        float cx = -current_x - game.player_x; 
        
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

    // ===================================================================
    // 通道 2：渲染路边景物 Sprites (从远到近，画家算法保证正确遮挡)
    // ===================================================================
    // 重新计算从灭点到最近点的道路中心线
    current_dx = 0; current_x = 0;
    // 1. 计算灭点处的 dx 总和
    for (uint16_t i = 1; i < RACE_DRAW_DISTANCE; i++) current_dx += game.segments[i-1].curve;
    // 2. 计算灭点处相对玩家的中心点 current_x
    for (uint16_t i = 1; i < RACE_DRAW_DISTANCE; i++) {
        current_x += current_dx;
        current_dx -= game.segments[RACE_DRAW_DISTANCE-1-i].curve;
    }
    // 3. 远到近循环
    for (uint16_t n = RACE_DRAW_DISTANCE - 1; n > 0; n--) {
        RaceSegment_t* seg = &game.segments[n];
        
        float world_z = n * RACE_SEGMENT_LENGTH - game.cam_z;
        if (world_z < clip_z) continue; // 裁切近处物体

        // 计算该段道路中心线 world_x
        float world_center_x = -current_x - game.player_x; 
        current_dx -= game.segments[n-1].curve;
        current_x -= current_dx;

        // ★ 如果该段有关联物体，进行投影绘制
        if (seg->object_type != OBJ_NONE) {
            float world_y = seg->y - cam_y;
            // 物体 X 坐标 = 道路中心 + (物体侧边 * 路宽 * 偏移比例)
            float obj_world_x = world_center_x + (seg->object_side * RACE_ROAD_WIDTH * 1.6f);
            
            float sx, sy, sw;
            project(obj_world_x, world_y, world_z, dynamic_fov, &sx, &sy, &sw);
            
            sy += final_pitch_offset;
            // 透视缩放比例 = (屏幕上的路宽 / 基础路宽定义)
            float sprite_scale = (sw / RACE_ROAD_WIDTH) * 60.0f * seg->object_scale;

            // 绘制景物 (在屏幕内裁切)
            if (sy > -50 && sy < max_y + 100 && sx > -100 && sx < RACE_SCREEN_W + 100) {
                switch(seg->object_type) {
                    case OBJ_TREE_PIN: drawPineTree((int)sx, (int)sy, sprite_scale); break;
                    case OBJ_PALM:     drawPalmTree((int)sx, (int)sy, sprite_scale); break;
                    case OBJ_SIGN:     drawRoadSign((int)sx, (int)sy, sprite_scale, seg->object_side); break;
                }
            }
        }
    }
}

// ==================== 街机风 UI 系统 ====================
static void drawDashboard(void) {
    extern IMU IMU_me;
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
    
    int display_spd = (int)game.speed; 
    Display_DrawTextF(spd_cx - 10, meter_cy - 12, "%3d", display_spd);

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

// ==================== 核心：目标速度映射与平滑过渡 ====================
static void handleInput(void) {
    extern IMU IMU_me;
    float old_speed = game.speed;
    
    float rel_roll = IMU_me.roll - game.base_roll;    // 前后倾斜
    float rel_pitch = IMU_me.pitch - game.base_pitch; // 左右倾斜
    
    float deadzone = 5.0f;       // 5度死区
    float max_tilt = 35.0f;      // 35度为“油门踩到底”的极限倾角

    // =======================================================
    // 1. 目标速度映射 (倾斜度决定的物理目标)
    // =======================================================
    float target_speed = 0.0f;

    if (rel_roll > deadzone) { 
        // 前倾：计算踩油门的深度比例 (0.0 到 1.0)
        float tilt_ratio = (rel_roll - deadzone) / (max_tilt - deadzone);
        if (tilt_ratio > 1.0f) tilt_ratio = 1.0f;
        target_speed = tilt_ratio * game.max_speed;
    } 
    else {
        target_speed = 0.0f;
    }

    // =======================================================
    // 2. 丝滑速度过渡 (指数平滑阻尼器)
    // =======================================================
    if (game.speed < target_speed) {
        // 正在加速：每次补充当前速度与目标速度之间差距的 5%
        game.speed += (target_speed - game.speed) * 0.05f; 
    } 
    else {
        // 正在减速
        if (rel_roll < -deadzone) {
            // 主动后仰(踩刹车)：降速极其剧烈 (补充 15% 的差距)
            game.speed += (target_speed - game.speed) * 0.15f; 
        } else {
            // 松开油门(处于死区)：自然滑行
            game.speed += (target_speed - game.speed) * 0.02f; 
        }
    }
    
    if (game.speed > game.max_speed) game.speed = game.max_speed;
    if (game.speed < 0) game.speed = 0;
    
    game.accel = game.speed - old_speed;

    // =======================================================
    // 3. 转向控制 
    // =======================================================
    float move_speed = 300.0f * (game.speed / game.max_speed); 
    
    float input_strength = fabs(rel_pitch) / 20.0f; 
    if (input_strength > 1.0f) input_strength = 1.0f;

    if (rel_pitch < -deadzone) { 
        game.player_x -= move_speed * input_strength; 
        if (game.speed > (game.max_speed * 0.5f) && rel_pitch < -25.0f) Output_MotorVibrate(128, 30);
    } 
    else if (rel_pitch > deadzone) { 
        game.player_x += move_speed * input_strength; 
        if (game.speed > (game.max_speed * 0.5f) && rel_pitch > 25.0f) Output_MotorVibrate(128, 30);
    } 

    // =======================================================
    // 4. 离心力与天空联动 (重置系数，适配180极限速度)
    // =======================================================
    float current_curve = game.segments[0].curve;
    game.player_x += current_curve * (game.speed * 0.1f); 
    game.sky_offset -= current_curve * game.speed * 0.035f; 

    // =======================================================
    // 5. 草地减速判定 (自由驰骋版)
    // =======================================================
    static uint32_t last_hit_time = 0;
    
    if (game.player_x > RACE_ROAD_WIDTH * 1.5f || game.player_x < -RACE_ROAD_WIDTH * 1.5f) {
        // 草地上最高速度会被压制
        game.speed *= 0.95f; 
        
        if (millis() - last_hit_time > 500) {
            Output_Feedback(FEEDBACK_HIT);
            last_hit_time = millis();
        }
    }
}

// ==================== 串口键盘 Debug 系统 ====================
void GameRace_HandleSerial(void) {
    while (Serial.available() > 0) {
        char c = Serial.read(); 
        
        switch (c) {
            case 'w': case 'W': game.cam_height += 50.0f; break;
            case 's': case 'S': game.cam_height -= 50.0f; break;
            case 'r': case 'R': game.fov += 2.0f; break;
            case 'f': case 'F': game.fov -= 2.0f; break;
            case ']': game.max_speed += 20.0f; break;
            case '[': game.max_speed -= 20.0f; break;
        }
        
        if (game.cam_height < 100.0f) game.cam_height = 100.0f;
        if (game.cam_height > 3000.0f) game.cam_height = 3000.0f;
        if (game.fov < 20.0f) game.fov = 20.0f;
        if (game.fov > 160.0f) game.fov = 160.0f;
    }
}

// ==================== 生命周期 ====================
void GameRace_Init(void) {
    memset(&game, 0, sizeof(game));
    game.max_speed = 180.0f; 
    
    game.cam_height = 600.0f; 
    game.fov = 100.0f;        
    game.steering_sensitivity = 1.5f;
    game.time_of_day = 17.0f;
    
    game.waiting_to_start = true;
    game.base_roll = 0.0f;
    game.base_pitch = 0.0f;
    game.speed = 0.0f;
    game.sky_offset = 0.0f; 
    
    // 初始化时生成景物
    for (int i = 0; i < RACE_SEGMENT_COUNT; i++) {
        generateNextSegment(&game.segments[i]);
    }
    
    game.initialized = true;
    game.last_update = millis();
    game.state_timer = millis(); 
}

SystemState_t GameRace_Update(void) {
    if (!game.initialized) return SYS_GAME_PLAYING;
    
    GameRace_HandleSerial(); 
    extern IMU IMU_me;
    
    uint32_t now = millis();
    if (now - game.last_update < 33) return SYS_GAME_PLAYING; 
    game.last_update = now;
    
    OutputFeedback_Update();
    
    if (game.waiting_to_start) {
        updateEnvironment(); 
        render();            
        drawDashboard();     

        Display_FillRoundRect(14, 60, 100, 40, 4, 0x0000); 
        Display_SetTextColor(0xFFE0, 0x0000); 
        Display_SetTextSize(1);
        Display_DrawTextCenteredSimple(RACE_SCREEN_W / 2, 70, "SHAKE TO START");
        Display_SetTextColor(0xCE79, 0x0000); 
        Display_DrawTextCenteredSimple(RACE_SCREEN_W / 2, 85, "Hold flat & Shake");

        Display_EndFrame();

        if (IMU_me.motion == MOTION_SHAKE && (millis() - game.state_timer > 1000)) {
            game.base_roll = IMU_me.roll;
            game.base_pitch = IMU_me.pitch;
            game.waiting_to_start = false;
            
            Output_Feedback(FEEDBACK_SUCCESS);
            
            game.state_timer = millis(); 
        }
        return SYS_GAME_PLAYING;
    }
    
    updateEnvironment();
    handleInput(); 
    
    game.cam_z += game.speed;
    game.total_distance += game.speed;
    
    while (game.cam_z >= RACE_SEGMENT_LENGTH) {
        game.cam_z -= RACE_SEGMENT_LENGTH;
        game.global_idx++; 
        memmove(&game.segments[0], &game.segments[1], sizeof(RaceSegment_t) * (RACE_SEGMENT_COUNT - 1));
        // ★ 生成新赛道段时，会自动附带新的随机景物信息
        generateNextSegment(&game.segments[RACE_SEGMENT_COUNT - 1]);
    }
    
    render();          
    drawDashboard();   
    Display_EndFrame(); 
    
    // 马达基础引擎震动 (受让权机制保护)
    if (!Output_IsMotorPlaying()) {
        float speed_ratio = game.speed / game.max_speed;
        if (speed_ratio > 1.0f) speed_ratio = 1.0f;
        if (speed_ratio < 0.0f) speed_ratio = 0.0f;
        
        uint8_t motor_pwm = (uint8_t)(speed_ratio * 150.0f); 
        
        if (fabs(game.player_x) > (RACE_ROAD_WIDTH * 0.5f)) {
            if (game.speed > 10.0f) {
                int grass_pwm = motor_pwm + 100;
                motor_pwm = (grass_pwm > 255) ? 255 : (uint8_t)grass_pwm;
            }
        }
        
        Motor_Forward(MOTOR_BOTH, motor_pwm);
    }
    
    static uint32_t last_high_speed_beep = 0;
    if ((game.speed / game.max_speed) > 0.9f && millis() - last_high_speed_beep > 1000) {
        Output_Feedback(FEEDBACK_WARNING);
        last_high_speed_beep = millis();
    }
    
    static uint32_t last_low_speed_beep = 0;
    if ((game.speed / game.max_speed) < 0.05f && millis() - last_low_speed_beep > 2000) {
        Output_BuzzerTone(300, 100, 50);  
        last_low_speed_beep = millis();
    }
    
    // if (IMU_me.motion == MOTION_SHAKE && (millis() - game.state_timer > 2000)) {
    //     Output_Stop();
    //     Motor_Stop(MOTOR_BOTH);
    //     return SYS_GAME_SELECT;
    // }
    
    return SYS_GAME_PLAYING;
}

int GameRace_GetScore(void) {
    return (int)(game.total_distance / 1000.0f);
}