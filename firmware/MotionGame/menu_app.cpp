// menu_app.cpp
#include "menu_app.h"
#include "display_driver.h"
#include "motion_service.h"
#include "motion_type.h"
#include "menu_icons.h"
#include <math.h>
#include <stdio.h>

// 菜单项结构
typedef struct {
    const uint8_t* icon;
    const char* name;
    SystemState_t result;
} MenuItem_t;

static const MenuItem_t menu[] = {
    { ICON_GAME_64,     "GAME",      SYS_GAME_SELECT     },
    { ICON_THEME_64,    "THEME",     SYS_THEME           },
    { ICON_SETTINGS_64, "CONTROLLER", SYS_WIFI_CONTROLLER },  // WiFi 体感控制器
    { ICON_ABOUT_64,    "ABOUT",     SYS_ABOUT           },
};

#define MENU_COUNT (sizeof(menu)/sizeof(menu[0]))

// 布局常量
#define TITLE_AREA_HEIGHT 35
#define ICON_AREA_TOP 45
#define ICON_AREA_HEIGHT 64
#define NAME_AREA_TOP 115
#define HINT_AREA_TOP 150

// 状态变量
static int cur_index = 0;
static int dst_index = 0;
static float anim_time = 0.0f;
static float text_anim_time = 0.0f;
static float anim_duration = 0.5f;
static float text_anim_duration = 0.4f;
static float damping_strength = 1.7f;
static bool need_redraw = true;
static bool is_animating = false;

// 颜色变量
static AppColors_t menu_colors = {
    .MENU_BG = 0x0000,
    .TEXT_COLOR = 0xFFFF,
    .TITLE_COLOR = 0xF81F,
    .HINT_COLOR = 0x07FF,
    .ACCENT_COLOR = 0xF81F,
    .PROGRESS_BG = 0x3186,
    .PROGRESS_FG = 0xF81F,
    .ICON_COLOR = 0xF81F
};

// 缓动函数
static float easeOutBack(float t, float strength) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    float c1 = strength * 1.70158f;
    float c3 = c1 + 1.0f;
    return 1.0f + c3 * pow(t - 1.0f, 3.0f) + c1 * pow(t - 1.0f, 2.0f);
}

static float easeInOutCubic(float t) {
    if (t < 0.5f) return 4.0f * t * t * t;
    float f = (2.0f * t - 2.0f);
    return 0.5f * f * f * f + 1.0f;
}

// 位置计算函数
static void CalculateIconPositions(int* cur_x, int* dst_x, int* center_y) {
    const int CENTER_X = 32;
    const int CENTER_Y = ICON_AREA_TOP;
    
    *cur_x = CENTER_X;
    *dst_x = CENTER_X;
    *center_y = CENTER_Y;
    
    if (!is_animating) return;
    
    float raw_progress = anim_time / anim_duration;
    if (raw_progress > 1.0f) raw_progress = 1.0f;
    float progress = easeOutBack(raw_progress, damping_strength);
    float current_offset = progress * 128.0f;
    
    if (dst_index > cur_index) {
        *cur_x = CENTER_X - current_offset;
        *dst_x = CENTER_X + (128.0f - current_offset);
    } else {
        *cur_x = CENTER_X + current_offset;
        *dst_x = CENTER_X - (128.0f - current_offset);
    }
}

static void CalculateTextPositions(float* cur_alpha, float* dst_alpha, 
                                   int* cur_y_offset, int* dst_y_offset) {
    *cur_alpha = 1.0f;
    *dst_alpha = 0.0f;
    *cur_y_offset = 0;
    *dst_y_offset = 0;
    
    if (!is_animating) return;
    
    float text_raw_progress = text_anim_time / text_anim_duration;
    if (text_raw_progress > 1.0f) text_raw_progress = 1.0f;
    float text_progress = easeInOutCubic(text_raw_progress);
    
    float icon_raw_progress = anim_time / anim_duration;
    if (icon_raw_progress > 1.0f) icon_raw_progress = 1.0f;
    float icon_progress = easeOutBack(icon_raw_progress, damping_strength);
    
    if (text_progress < 0.5f) {
        *cur_alpha = 1.0f - (text_progress * 2.0f);
        *dst_alpha = text_progress * 2.0f;
    } else {
        *cur_alpha = 0.0f;
        *dst_alpha = 1.0f;
    }
    
    if (dst_index > cur_index) {
        *cur_y_offset = (int)(8.0f * icon_progress);
        *dst_y_offset = -(int)(8.0f * (1.0f - icon_progress));
    } else {
        *cur_y_offset = (int)(8.0f * icon_progress);
        *dst_y_offset = -(int)(8.0f * (1.0f - icon_progress));
    }
}

// 绘制函数
static void DrawProgressBar(int current, int total) {
    int bar_width = 100;
    int bar_height = 4;
    int bar_x = (128 - bar_width) / 2;
    int bar_y = 25;
    
    Display_FillRect(bar_x, bar_y, bar_width, bar_height, menu_colors.PROGRESS_BG);
    
    int progress_width = (current * bar_width) / total;
    if (progress_width > 0) {
        Display_FillRect(bar_x, bar_y, progress_width, bar_height, menu_colors.PROGRESS_FG);
        if (progress_width > 2) {
            Display_DrawLine(bar_x + progress_width - 2, bar_y + 1, 
                           bar_x + progress_width - 1, bar_y + 1, 
                           menu_colors.ACCENT_COLOR);
        }
    }
    
    for (int i = 1; i < total; i++) {
        int marker_x = bar_x + (i * bar_width / total);
        Display_DrawLine(marker_x, bar_y, marker_x, bar_y + bar_height - 1, menu_colors.MENU_BG);
    }
}

static void DrawTitle(void) {
    TextParams_t main_title = {
        .font = DISPLAY_FONT_5x7,
        .color = menu_colors.TITLE_COLOR,
        .bg_color = menu_colors.MENU_BG,
        .size = 2,
        .h_align = TEXT_ALIGN_CENTER,
        .v_align = TEXT_ALIGN_TOP,
        .transparent = true
    };
    
    TextParams_t shadow_title = main_title;
    shadow_title.color = 0x3186;
    Display_DrawString(65, 7, "GAME MENU", &shadow_title);
    Display_DrawString(64, 6, "GAME MENU", &main_title);
}

static void DrawIcon(int x, int y, const uint8_t* icon, float alpha) {
    if (alpha <= 0.01f) return;
    
    uint16_t icon_color = menu_colors.ICON_COLOR;
    if (alpha < 1.0f) {
        uint8_t r = (icon_color >> 11) & 0x1F;
        uint8_t g = (icon_color >> 5) & 0x3F;
        uint8_t b = icon_color & 0x1F;
        r = (uint8_t)(r * alpha);
        g = (uint8_t)(g * alpha);
        b = (uint8_t)(b * alpha);
        icon_color = (r << 11) | (g << 5) | b;
    }
    
    Display_DrawBitmap1B(x, y, 64, 64, icon, icon_color, menu_colors.MENU_BG);
}

static void DrawMenuName(int x, int y, const char* name, float alpha) {
    if (!name || alpha <= 0.01f) return;
    
    TextParams_t name_params = {
        .font = DISPLAY_FONT_5x7,
        .color = menu_colors.TEXT_COLOR,
        .bg_color = menu_colors.MENU_BG,
        .size = 2,
        .h_align = TEXT_ALIGN_CENTER,
        .v_align = TEXT_ALIGN_TOP,
        .transparent = true
    };
    
    if (alpha < 1.0f) {
        uint16_t original_color = name_params.color;
        uint8_t r = (original_color >> 11) & 0x1F;
        uint8_t g = (original_color >> 5) & 0x3F;
        uint8_t b = original_color & 0x1F;
        r = (uint8_t)(r * alpha);
        g = (uint8_t)(g * alpha);
        b = (uint8_t)(b * alpha);
        name_params.color = (r << 11) | (g << 5) | b;
    }
    
    Display_DrawString(x, y, name, &name_params);
}

static void DrawControlHints(void) {
    TextParams_t hint_params = {
        .font = DISPLAY_FONT_5x7,
        .color = menu_colors.HINT_COLOR,
        .bg_color = menu_colors.MENU_BG,
        .size = 1,
        .h_align = TEXT_ALIGN_CENTER,
        .v_align = TEXT_ALIGN_TOP,
        .transparent = true
    };
    
    if (is_animating) {
        Display_DrawString(64, HINT_AREA_TOP, "Switching...", &hint_params);
    } else {
        Display_DrawString(64, HINT_AREA_TOP, "Tilt:Select  Shake:Enter", &hint_params);
    }
}

static void DrawDirectionArrows(void) {
    if (is_animating) return;
    
    if (cur_index > 0) {
        Display_DrawTriangle(10, 82, 18, 75, 18, 89, menu_colors.TEXT_COLOR);
        Display_FillTriangle(10, 82, 18, 75, 18, 89, menu_colors.TEXT_COLOR);
    }
    
    if (cur_index < MENU_COUNT - 1) {
        Display_DrawTriangle(118, 82, 110, 75, 110, 89, menu_colors.TEXT_COLOR);
        Display_FillTriangle(118, 82, 110, 75, 110, 89, menu_colors.TEXT_COLOR);
    }
}

// 接口函数
void MenuApp_Init(void) {
    cur_index = 0;
    dst_index = 0;
    anim_time = 0.0f;
    text_anim_time = 0.0f;
    need_redraw = true;
    is_animating = false;
    damping_strength = 1.7f;
}

SystemState_t MenuApp_Update(void) {
    if (!is_animating) {
        if (IMU_me.motion == MOTION_TILT_LEFT && cur_index > 0) {
            dst_index = cur_index - 1;
            anim_time = 0.0f;
            text_anim_time = 0.0f;
            is_animating = true;
            need_redraw = true;
        }
        else if (IMU_me.motion == MOTION_TILT_RIGHT && cur_index < MENU_COUNT - 1) {
            dst_index = cur_index + 1;
            anim_time = 0.0f;
            text_anim_time = 0.0f;
            is_animating = true;
            need_redraw = true;
        }
        else if (IMU_me.motion == MOTION_SHAKE) {
            return menu[cur_index].result;
        }
    }
    
    if (is_animating) {
        anim_time += 0.016f;
        text_anim_time += 0.016f;
        
        if (anim_time >= anim_duration) {
            anim_time = anim_duration;
            cur_index = dst_index;
            anim_time = 0.0f;
            text_anim_time = 0.0f;
            is_animating = false;
            need_redraw = true;
        } else {
            need_redraw = true;
        }
    }
    
    if (!need_redraw) {
        return SYS_NONE;
    }
    
    Display_StartFrame();
    Display_ClearBuffer(menu_colors.MENU_BG);
    
    DrawTitle();
    DrawProgressBar(cur_index + 1, MENU_COUNT);
    
    int cur_icon_x, dst_icon_x, center_y;
    CalculateIconPositions(&cur_icon_x, &dst_icon_x, &center_y);
    
    float cur_text_alpha, dst_text_alpha;
    int cur_text_y_offset, dst_text_y_offset;
    CalculateTextPositions(&cur_text_alpha, &dst_text_alpha, 
                          &cur_text_y_offset, &dst_text_y_offset);
    
    DrawIcon(cur_icon_x, center_y, menu[cur_index].icon, cur_text_alpha);
    if (is_animating && dst_index != cur_index) {
        DrawIcon(dst_icon_x, center_y, menu[dst_index].icon, dst_text_alpha);
    }
    
    DrawDirectionArrows();
    
    int cur_name_y = NAME_AREA_TOP + cur_text_y_offset;
    int dst_name_y = NAME_AREA_TOP + dst_text_y_offset;
    DrawMenuName(64, cur_name_y, menu[cur_index].name, cur_text_alpha);
    if (is_animating && dst_index != cur_index) {
        DrawMenuName(64, dst_name_y, menu[dst_index].name, dst_text_alpha);
    }
    
    DrawControlHints();
    Display_EndFrame();
    need_redraw = false;
    
    return SYS_NONE;
}

void MenuApp_ApplyThemeColors(const AppColors_t* theme_colors) {
    if (!theme_colors) return;
    
    bool colors_changed = false;
    if (menu_colors.MENU_BG != theme_colors->MENU_BG) {
        menu_colors.MENU_BG = theme_colors->MENU_BG;
        colors_changed = true;
    }
    if (menu_colors.TEXT_COLOR != theme_colors->TEXT_COLOR) {
        menu_colors.TEXT_COLOR = theme_colors->TEXT_COLOR;
        colors_changed = true;
    }
    if (menu_colors.TITLE_COLOR != theme_colors->TITLE_COLOR) {
        menu_colors.TITLE_COLOR = theme_colors->TITLE_COLOR;
        colors_changed = true;
    }
    if (menu_colors.HINT_COLOR != theme_colors->HINT_COLOR) {
        menu_colors.HINT_COLOR = theme_colors->HINT_COLOR;
        colors_changed = true;
    }
    if (menu_colors.ACCENT_COLOR != theme_colors->ACCENT_COLOR) {
        menu_colors.ACCENT_COLOR = theme_colors->ACCENT_COLOR;
        colors_changed = true;
    }
    if (menu_colors.PROGRESS_BG != theme_colors->PROGRESS_BG) {
        menu_colors.PROGRESS_BG = theme_colors->PROGRESS_BG;
        colors_changed = true;
    }
    if (menu_colors.PROGRESS_FG != theme_colors->PROGRESS_FG) {
        menu_colors.PROGRESS_FG = theme_colors->PROGRESS_FG;
        colors_changed = true;
    }
    if (menu_colors.ICON_COLOR != theme_colors->ICON_COLOR) {
        menu_colors.ICON_COLOR = theme_colors->ICON_COLOR;
        colors_changed = true;
    }
    
    if (colors_changed) need_redraw = true;
}

// 控制函数
void MenuApp_SetDampingStrength(float strength) {
    if (strength < 0.5f) strength = 0.5f;
    if (strength > 3.0f) strength = 3.0f;
    damping_strength = strength;
    need_redraw = true;
}

void MenuApp_SetAnimationDuration(float duration) {
    if (duration < 0.1f) duration = 0.1f;
    if (duration > 2.0f) duration = 2.0f;
    anim_duration = duration;
    text_anim_duration = duration * 0.8f;
    need_redraw = true;
}

float MenuApp_GetDampingStrength(void) { return damping_strength; }
float MenuApp_GetAnimationDuration(void) { return anim_duration; }
int MenuApp_GetCurrentIndex(void) { return cur_index; }

void MenuApp_SetCurrentIndex(int index) {
    if (index >= 0 && index < MENU_COUNT) {
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

void MenuApp_ApplyAnimationPreset(int preset) {
    switch (preset) {
        case 0: // 快速响应
            MenuApp_SetDampingStrength(0.8f);
            MenuApp_SetAnimationDuration(0.3f);
            break;
        case 1: // 自然回弹
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