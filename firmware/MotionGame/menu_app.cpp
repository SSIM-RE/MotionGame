// menu_app.cpp - 完整双缓冲版本（文本也有阻尼效果）
#include "menu_app.h"
#include "display_driver.h"
#include "motion_service.h"
#include "motion_type.h"
#include "menu_icons.h"
#include <math.h>

#define MENU_BG TFT_RED
#define TEXT_COLOR TFT_WHITE
#define TITLE_COLOR TFT_YELLOW
#define HINT_COLOR TFT_CYAN

/* ================= 菜单项 ================= */

typedef struct {
    const uint8_t* icon;   // 64x64 1bit
    const char* name;      // 菜单项名称
    MenuResult_t result;   // 菜单结果
} MenuItem_t;

static const MenuItem_t menu[] = {
    { ICON_GAME_64,     "GAME",     MENU_ENTER_GAME     },
    { ICON_THEME_64,    "THEME",    MENU_ENTER_THEME    },
    { ICON_SETTINGS_64, "SETTINGS", MENU_ENTER_SETTINGS },
    { ICON_ABOUT_64,    "ABOUT",    MENU_ENTER_ABOUT    },
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

// 文本动画相关
static float text_anim_time = 0.0f;
static float text_anim_duration = 0.4f;  // 文本动画稍快一些

/* ================= 缓动函数 ================= */

static float easeOutBack(float t, float strength)
{
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    
    float c1 = strength * 1.70158f;
    float c3 = c1 + 1.0f;
    return 1.0f + c3 * pow(t - 1.0f, 3.0f) + c1 * pow(t - 1.0f, 2.0f);
}

static float easeInOutCubic(float t)
{
    if (t < 0.5f) {
        return 4.0f * t * t * t;
    } else {
        float f = (2.0f * t - 2.0f);
        return 0.5f * f * f * f + 1.0f;
    }
}

/* ================= 计算位置（带阻尼） ================= */

/**
 * @brief 计算图标位置（带阻尼效果）
 */
static void CalculateIconPositions(int* cur_x, int* dst_x, int* center_y)
{
    // 固定中心位置
    const int CENTER_X = 32;  // (128-64)/2
    const int CENTER_Y = 48;  // (160-64)/2
    
    *cur_x = CENTER_X;
    *dst_x = CENTER_X;
    *center_y = CENTER_Y;
    
    if (!is_animating) return;
    
    // 计算动画进度
    float raw_progress = anim_time / anim_duration;
    if (raw_progress > 1.0f) raw_progress = 1.0f;
    
    float progress = easeOutBack(raw_progress, damping_strength);
    
    // 动画移动总距离 = 屏幕宽度（128像素）
    float current_offset = progress * 128.0f;
    
    if (dst_index > cur_index) {
        // 向右切换：当前图标向左移出，目标图标从右边进入
        *cur_x = CENTER_X - current_offset;
        *dst_x = CENTER_X + (128.0f - current_offset);
    } else {
        // 向左切换：当前图标向右移出，目标图标从左边进入
        *cur_x = CENTER_X + current_offset;
        *dst_x = CENTER_X - (128.0f - current_offset);
    }
}

/**
 * @brief 计算文本位置（带阻尼效果）
 * @param cur_y 当前文本y位置（输出）
 * @param dst_y 目标文本y位置（输出）
 * @param alpha 文本透明度（0.0-1.0，输出）
 */
static void CalculateTextPositions(float* cur_alpha, float* dst_alpha, 
                                   int* cur_y_offset, int* dst_y_offset)
{
    *cur_alpha = 1.0f;
    *dst_alpha = 0.0f;
    *cur_y_offset = 0;
    *dst_y_offset = 0;
    
    if (!is_animating) return;
    
    // 计算文本动画进度（比图标动画稍快，更有层次感）
    float text_raw_progress = text_anim_time / text_anim_duration;
    if (text_raw_progress > 1.0f) text_raw_progress = 1.0f;
    
    float text_progress = easeInOutCubic(text_raw_progress);
    
    // 图标动画进度
    float icon_raw_progress = anim_time / anim_duration;
    if (icon_raw_progress > 1.0f) icon_raw_progress = 1.0f;
    float icon_progress = easeOutBack(icon_raw_progress, damping_strength);
    
    // 文本淡入淡出效果
    if (text_progress < 0.5f) {
        // 前半段：当前文本淡出，目标文本淡入
        *cur_alpha = 1.0f - (text_progress * 2.0f);
        *dst_alpha = text_progress * 2.0f;
    } else {
        *cur_alpha = 0.0f;
        *dst_alpha = 1.0f;
    }
    
    // 文本上下移动效果
    if (dst_index > cur_index) {
        // 向右切换：当前文本向左下移动，目标文本从右上进入
        *cur_y_offset = (int)(20.0f * icon_progress);  // 向下移动
        *dst_y_offset = -(int)(20.0f * (1.0f - icon_progress));  // 从上方进入
    } else {
        // 向左切换：当前文本向右下移动，目标文本从左上进入
        *cur_y_offset = (int)(20.0f * icon_progress);  // 向下移动
        *dst_y_offset = -(int)(20.0f * (1.0f - icon_progress));  // 从上方进入
    }
}

/* ================= 绘制函数 ================= */

/**
 * @brief 绘制图标（带边界检查）
 */
static void DrawIcon(int x, int y, const uint8_t* icon, float alpha)
{
    // 只绘制在屏幕内或部分在屏幕内的图标
    if (x < 128 && x + 64 > 0 && y < 160 && y + 64 > 0) {
        // 根据alpha调整颜色强度（简化处理）
        uint16_t fg_color = TFT_YELLOW;
        if (alpha < 1.0f) {
            // 简单的透明度处理：降低颜色亮度
            uint8_t r = (fg_color >> 11) & 0x1F;
            uint8_t g = (fg_color >> 5) & 0x3F;
            uint8_t b = fg_color & 0x1F;
            
            r = (uint8_t)(r * alpha);
            g = (uint8_t)(g * alpha);
            b = (uint8_t)(b * alpha);
            
            fg_color = (r << 11) | (g << 5) | b;
        }
        
        Display_DrawBitmap1B(x, y, 64, 64, icon, fg_color, MENU_BG);
    }
}

/**
 * @brief 绘制文本（带透明度效果）
 */
static void DrawText(int x, int y, const char* text, 
                     const TextParams_t* base_params, float alpha)
{
    if (!text || alpha <= 0.01f) return;
    
    // 复制参数并调整透明度
    TextParams_t params = *base_params;
    
    // 根据alpha调整颜色
    if (alpha < 1.0f) {
        uint16_t original_color = params.color;
        uint8_t r = (original_color >> 11) & 0x1F;
        uint8_t g = (original_color >> 5) & 0x3F;
        uint8_t b = original_color & 0x1F;
        
        r = (uint8_t)(r * alpha);
        g = (uint8_t)(g * alpha);
        b = (uint8_t)(b * alpha);
        
        params.color = (r << 11) | (g << 5) | b;
    }
    
    Display_DrawString(x, y, text, &params);
}

/* ================= 主函数实现 ================= */

void MenuApp_Init(void)
{
    cur_index = 0;
    dst_index = 0;
    anim_time = 0.0f;
    text_anim_time = 0.0f;
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
            text_anim_time = 0.0f;
            is_animating = true;
            need_redraw = true;
        }
        else if (motion == MOTION_TILT_RIGHT && cur_index < MENU_COUNT - 1)
        {
            // 向右倾斜：切换到右边菜单
            dst_index = cur_index + 1;
            anim_time = 0.0f;
            text_anim_time = 0.0f;
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
        text_anim_time += 0.016f;
        
        if (anim_time >= anim_duration)
        {
            // 图标动画结束
            anim_time = anim_duration;
            cur_index = dst_index;
            anim_time = 0.0f;
            text_anim_time = 0.0f;
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
    
    // 清除整个屏幕
    Display_ClearBuffer(MENU_BG);
    
    // 计算图标位置
    int cur_icon_x, dst_icon_x, center_y;
    CalculateIconPositions(&cur_icon_x, &dst_icon_x, &center_y);
    
    // 计算文本动画参数
    float cur_text_alpha, dst_text_alpha;
    int cur_text_y_offset, dst_text_y_offset;
    CalculateTextPositions(&cur_text_alpha, &dst_text_alpha, 
                          &cur_text_y_offset, &dst_text_y_offset);
    
    /* ========== 绘制图标 ========== */
    
    // 绘制当前图标
    DrawIcon(cur_icon_x, center_y, menu[cur_index].icon, cur_text_alpha);
    
    // 绘制目标图标（如果正在动画）
    if (is_animating && dst_index != cur_index)
    {
        DrawIcon(dst_icon_x, center_y, menu[dst_index].icon, dst_text_alpha);
    }
    
    /* ========== 绘制标题（固定位置） ========== */
    
    TextParams_t title_params = {
        .font = DISPLAY_FONT_5x7,
        .color = TITLE_COLOR,
        .bg_color = MENU_BG,
        .size = 2,
        .h_align = TEXT_ALIGN_CENTER,
        .v_align = TEXT_ALIGN_TOP,
        .transparent = true
    };
    
    Display_DrawTextCentered(64, 5, "GAME MENU", &title_params);
    
    /* ========== 绘制菜单项名称（有动画） ========== */
    
    TextParams_t name_params = {
        .font = DISPLAY_FONT_5x7,
        .color = TEXT_COLOR,
        .bg_color = MENU_BG,
        .size = 2,
        .h_align = TEXT_ALIGN_CENTER,
        .v_align = TEXT_ALIGN_TOP,
        .transparent = true
    };
    
    // 当前菜单项名称位置（中心下方，有垂直偏移）
    int cur_text_y = 120 + cur_text_y_offset;
    DrawText(64, cur_text_y, menu[cur_index].name, &name_params, cur_text_alpha);
    
    // 目标菜单项名称（如果正在动画）
    if (is_animating && dst_index != cur_index)
    {
        int dst_text_y = 120 + dst_text_y_offset;
        DrawText(64, dst_text_y, menu[dst_index].name, &name_params, dst_text_alpha);
    }
    
    /* ========== 绘制指示器 ========== */
    
    char indicator[16];
    snprintf(indicator, sizeof(indicator), "%d/%d", cur_index + 1, MENU_COUNT);
    
    TextParams_t indicator_params = {
        .font = DISPLAY_FONT_5x7,
        .color = HINT_COLOR,
        .bg_color = MENU_BG,
        .size = 1,
        .h_align = TEXT_ALIGN_CENTER,
        .v_align = TEXT_ALIGN_TOP,
        .transparent = true
    };
    
    // 指示器也有淡入淡出效果
    float indicator_alpha = is_animating ? 0.5f : 1.0f;  // 动画时半透明
    DrawText(64, 140, indicator, &indicator_params, indicator_alpha);
    
    /* ========== 绘制操作提示 ========== */
    
    TextParams_t hint_params = {
        .font = DISPLAY_FONT_5x7,
        .color = TEXT_COLOR,
        .bg_color = MENU_BG,
        .size = 1,
        .h_align = TEXT_ALIGN_CENTER,
        .v_align = TEXT_ALIGN_BOTTOM,
        .transparent = true
    };
    
    // 动画时隐藏操作提示
    float hint_alpha = is_animating ? 0.3f : 1.0f;
    if (is_animating) {
        DrawText(64, 155, "Switching...", &hint_params, hint_alpha);
    } else {
        DrawText(64, 155, "Tilt:Select  Shake:Enter", &hint_params, hint_alpha);
    }
    
    /* ========== 绘制装饰元素 ========== */
    
    // 左右箭头提示（当前可用方向）
    if (!is_animating) {
        if (cur_index > 0) {
            // 左箭头
            Display_DrawTriangle(10, 80, 20, 70, 20, 90, TEXT_COLOR);
            Display_FillTriangle(10, 80, 20, 70, 20, 90, TEXT_COLOR);
        }
        
        if (cur_index < MENU_COUNT - 1) {
            // 右箭头
            Display_DrawTriangle(118, 80, 108, 70, 108, 90, TEXT_COLOR);
            Display_FillTriangle(118, 80, 108, 70, 108, 90, TEXT_COLOR);
        }
    }
    
    // 当前选中指示器（下划线）
    if (!is_animating) {
        int underline_y = 135; // 名称下方
        int text_width = Display_GetStringWidth(menu[cur_index].name, DISPLAY_FONT_5x7, 2);
        int underline_x = 64 - text_width / 2;
        
        Display_DrawLine(underline_x, underline_y, 
                        underline_x + text_width, underline_y, 
                        TITLE_COLOR);
        
        // 添加小圆点装饰
        Display_FillCircle(underline_x, underline_y, 1, TITLE_COLOR);
        Display_FillCircle(underline_x + text_width, underline_y, 1, TITLE_COLOR);
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
    text_anim_duration = duration * 0.8f; // 文本动画比图标快20%
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
        text_anim_time = 0.0f;
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