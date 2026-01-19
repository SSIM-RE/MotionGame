// menu_app.cpp - 修复版
#include "menu_app.h"
#include "display_driver.h"
#include "motion_service.h"
#include "motion_type.h"
#include "menu_icons.h"
#include <math.h>
#define MENU_BG TFT_RED
/* ================= 菜单项 ================= */

typedef struct {
    const uint8_t* icon;   // 64x64 1bit
    MenuResult_t   result;
} MenuItem_t;

static const MenuItem_t menu[] = {
    { ICON_GAME_64,     MENU_ENTER_GAME     },
    { ICON_THEME_64,    MENU_ENTER_THEME    },
    { ICON_SETTINGS_64, MENU_ENTER_SETTINGS },
    { ICON_ABOUT_64,    MENU_ENTER_ABOUT    },
};

#define MENU_COUNT (sizeof(menu)/sizeof(menu[0]))

/* ================= 状态变量 ================= */

static int cur_index = 0;        // 当前显示的菜单索引
static int dst_index = 0;        // 目标菜单索引

static float anim_time = 0.0f;   // 动画已进行的时间（秒）
static float anim_duration = 0.5f; // 动画总持续时间（秒）

static bool need_redraw = true;  // 需要重绘标志
static bool is_animating = false; // 是否正在动画中

// 阻尼参数
static float damping_strength = 1.7f;

/* ================= 缓动函数 ================= */

static float easeOutBack(float t, float strength)
{
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    
    float c1 = strength * 1.70158f;
    float c3 = c1 + 1.0f;
    return 1.0f + c3 * pow(t - 1.0f, 3.0f) + c1 * pow(t - 1.0f, 2.0f);
}

/* ================= 关键修复：正确的位置计算 ================= */

/**
 * @brief 计算图标位置（修复乱码的关键）
 * @param[out] cur_x 当前图标x位置
 * @param[out] dst_x 目标图标x位置
 * @param[out] cur_y 固定y位置
 */
static void CalculateCorrectPositions(int* cur_x, int* dst_x, int* cur_y)
{
    // 固定中心位置
    const int CENTER_X = 32;  // (128-64)/2
    const int CENTER_Y = 48;  // (160-64)/2
    
    *cur_x = CENTER_X;
    *dst_x = CENTER_X;
    *cur_y = CENTER_Y;
    
    if (!is_animating) return;
    
    // 计算动画进度
    float raw_progress = anim_time / anim_duration;
    if (raw_progress > 1.0f) raw_progress = 1.0f;
    
    float progress = easeOutBack(raw_progress, damping_strength);
    
    // 动画移动总距离 = 屏幕宽度（128像素）
    float current_offset = progress * 128.0f;
    
    if (dst_index > cur_index)  // 向右倾斜：切换到下一个菜单（右边）
    {
        // 当前图标向左移出屏幕（32 → -96）
        *cur_x = CENTER_X - current_offset;
        
        // 目标图标从右边进入屏幕（160 → 32）
        // 关键：目标图标开始时在屏幕右侧外（32+128=160）
        // 随着动画进行向左移动到中心位置（32）
        *dst_x = CENTER_X + (128.0f - current_offset);
    }
    else  // 向左倾斜：切换到上一个菜单（左边）
    {
        // 当前图标向右移出屏幕（32 → 160）
        *cur_x = CENTER_X + current_offset;
        
        // 目标图标从左边进入屏幕（-96 → 32）
        // 关键：目标图标开始时在屏幕左侧外（32-128=-96）
        // 随着动画进行向右移动到中心位置（32）
        *dst_x = CENTER_X - (128.0f - current_offset);
    }
    
    // 调试输出（如果需要）
    // Serial.printf("Anim: cur=%d->dst=%d, t=%.2f, p=%.2f, cur_x=%d, dst_x=%d\n",
    //               cur_index, dst_index, anim_time, progress, *cur_x, *dst_x);
}

/* ================= 安全绘制函数 ================= */

/**
 * @brief 安全绘制图标
 */
static void DrawIcon(int x, int y, const uint8_t* icon)
{
    // 只绘制在屏幕内或部分在屏幕内的图标
    if (x < 128 && x + 64 > 0 && y < 160 && y + 64 > 0)
    {
        Display_DrawBitmap1B(x, y, 64, 64, icon, TFT_YELLOW, MENU_BG);
    }
}

/* ================= 主函数实现 ================= */

void MenuApp_Init(void)
{
    cur_index = 0;
    dst_index = 0;
    anim_time = 0.0f;
    need_redraw = true;
    is_animating = false;
    damping_strength = 1.7f;
    
    // 清除屏幕
    Display_ClearScreen(MENU_BG);
}

MenuResult_t MenuApp_Update(void)
{
    // 获取运动输入
    MotionType_t motion = MotionService_Update();
    
    /* ========== 输入处理 ========== */
    if (!is_animating)
    {
        if (motion == MOTION_TILT_LEFT && cur_index > 0)
        {
            // 向左倾斜：切换到左边菜单
            dst_index = cur_index - 1;
            anim_time = 0.0f;
            is_animating = true;
            need_redraw = true;
        }
        else if (motion == MOTION_TILT_RIGHT && cur_index < MENU_COUNT - 1)
        {
            // 向右倾斜：切换到右边菜单
            dst_index = cur_index + 1;
            anim_time = 0.0f;
            is_animating = true;
            need_redraw = true;
        }
        else if (motion == MOTION_SHAKE)
        {
            // 摇晃：进入当前菜单项
            return menu[cur_index].result;
        }
    }
    
    /* ========== 动画更新 ========== */
    if (is_animating)
    {
        anim_time += 0.016f;  // 60FPS
        
        if (anim_time >= anim_duration)
        {
            // 动画结束
            anim_time = anim_duration;
            cur_index = dst_index;
            anim_time = 0.0f;
            is_animating = false;
            need_redraw = true;
        }
        else
        {
            need_redraw = true;
        }
    }
    
    if (!need_redraw)
    {
        return MENU_NONE;
    }
    
    /* ========== 绘制处理 ========== */
    
    // 开始双缓冲帧
    Display_StartFrame();
    
    // 清除整个图标区域（防止残影）
    Display_FillRect(0, 48, 128, 64, MENU_BG);
    
    // 计算正确的位置
    int cur_x, dst_x, cur_y;
    CalculateCorrectPositions(&cur_x, &dst_x, &cur_y);
    
    // 绘制当前图标
    DrawIcon(cur_x, cur_y, menu[cur_index].icon);
    
    // 绘制目标图标（如果正在动画）
    if (is_animating && dst_index != cur_index)
    {
        DrawIcon(dst_x, cur_y, menu[dst_index].icon);
    }
    
    // 结束帧
    Display_EndFrame();
    
    need_redraw = false;
    return MENU_NONE;
}

/* ================= 其他函数 ================= */

void MenuApp_SetDampingStrength(float strength)
{
    if (strength < 0.5f) strength = 0.5f;
    if (strength > 3.0f) strength = 3.0f;
    
    damping_strength = strength;
    need_redraw = true;
}

void MenuApp_SetAnimationDuration(float duration)
{
    if (duration < 0.1f) duration = 0.1f;
    if (duration > 2.0f) duration = 2.0f;
    
    anim_duration = duration;
    need_redraw = true;
}

float MenuApp_GetDampingStrength(void) { return damping_strength; }
float MenuApp_GetAnimationDuration(void) { return anim_duration; }
int MenuApp_GetCurrentIndex(void) { return cur_index; }

void MenuApp_SetCurrentIndex(int index)
{
    if (index >= 0 && index < MENU_COUNT)
    {
        cur_index = index;
        dst_index = index;
        anim_time = 0.0f;
        is_animating = false;
        need_redraw = true;
    }
}

bool MenuApp_IsAnimating(void) { return is_animating; }
void MenuApp_ForceRedraw(void) { need_redraw = true; }

void MenuApp_ApplyAnimationPreset(int preset)
{
    switch (preset)
    {
        case 0: // 快速响应
            MenuApp_SetDampingStrength(0.8f);
            MenuApp_SetAnimationDuration(0.3f);
            break;
            
        case 1: // 自然回弹（默认）
            MenuApp_SetDampingStrength(1.7f);
            MenuApp_SetAnimationDuration(0.5f);
            break;
            
        case 2: // 强阻尼效果
            MenuApp_SetDampingStrength(2.5f);
            MenuApp_SetAnimationDuration(0.6f);
            break;
            
        default:
            MenuApp_SetDampingStrength(1.7f);
            MenuApp_SetAnimationDuration(0.5f);
            break;
    }
}