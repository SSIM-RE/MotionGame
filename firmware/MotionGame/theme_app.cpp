// theme_app.cpp
#include "theme_app.h"
#include "display_driver.h"
#include "motion_service.h"
#include <EEPROM.h>

// 全局当前主题
static Theme_t current_theme = THEME_GAME;

// 主题颜色数组
static const ThemeColors_t themes[THEME_COUNT] = {
    // THEME_CLASSIC
    {0x0000, 0xFFFF, 0xF800, 0x07E0, 0x001F, 0xFFFF, 0xFD20, 0x8410},
    // THEME_DARK
    {0x1082, 0x3186, 0x39C7, 0x7BEF, 0x051F, 0xDEFB, 0xFD20, 0x2104},
    // THEME_LIGHT
    {0xFFFF, 0xEF7D, 0x001F, 0xF800, 0x07E0, 0x0000, 0xFD20, 0xC618},
    // THEME_RETRO
    {0x0000, 0x07E0, 0xF800, 0xFD20, 0x001F, 0x07E0, 0xFD20, 0x07E0},
    // THEME_GAME
    {0x0000, 0x3186, 0xF81F, 0x07FF, 0xF81F, 0xFFFF, 0xF81F, 0x07FF},
    // THEME_TECH
    {0x0000, 0x3186, 0x07FF, 0xF81F, 0x07FF, 0xFFFF, 0x07FF, 0xF81F}
};

// 主题名称
static const char* theme_names[THEME_COUNT] = {
    "Classic", "Dark", "Light", "Retro", "Game", "Tech"
};

// 主题应用状态
static int theme_display_index = 0;
static bool need_redraw = true;

// EEPROM地址
#define THEME_EEPROM_ADDR 0

// 私有函数
static void load_theme_from_eeprom(void) {
    uint8_t saved;
    EEPROM.get(THEME_EEPROM_ADDR, saved);
    if (saved < THEME_COUNT) {
        current_theme = (Theme_t)saved;
    }
}

static void save_theme_to_eeprom(void) {
    EEPROM.put(THEME_EEPROM_ADDR, (uint8_t)current_theme);
}

// 全局主题接口
const ThemeColors_t* Theme_GetCurrentColors(void) {
    return &themes[current_theme];
}

Theme_t Theme_GetCurrent(void) {
    return current_theme;
}

const ThemeColors_t* Theme_GetAllThemes(void) {
    return themes;
}

int Theme_GetThemeCount(void) {
    return THEME_COUNT;
}

// 主题应用界面
void ThemeApp_Init(void) {
    load_theme_from_eeprom();
    theme_display_index = (int)current_theme;
    need_redraw = true;
    Display_ClearScreen(themes[current_theme].background);
}

SystemState_t ThemeApp_Update(void) {
    MotionType_t motion = MotionService_Update();
    
    switch (motion) {
        case MOTION_TILT_LEFT:
            theme_display_index--;
            if (theme_display_index < 0) {
                theme_display_index = THEME_COUNT - 1;
            }
            need_redraw = true;
            break;
            
        case MOTION_TILT_RIGHT:
            theme_display_index = (theme_display_index + 1) % THEME_COUNT;
            need_redraw = true;
            break;
            
        case MOTION_SHAKE:
            current_theme = (Theme_t)theme_display_index;
            save_theme_to_eeprom();
            return SYS_MENU;
            
        case MOTION_ROLL_BACK:
            return SYS_MENU;
            
        default:
            break;
    }
    
    if (need_redraw) {
        const ThemeColors_t* colors = &themes[theme_display_index];
        
        Display_StartFrame();
        Display_ClearBuffer(colors->background);
        
        TextParams_t title_params = {
            .font = DISPLAY_FONT_5x7,
            .color = colors->primary,
            .bg_color = colors->background,
            .size = 2,
            .h_align = TEXT_ALIGN_CENTER,
            .v_align = TEXT_ALIGN_TOP,
            .transparent = true
        };
        Display_DrawString(64, 10, "SELECT THEME", &title_params);
        
        TextParams_t text_params = {
            .font = DISPLAY_FONT_5x7,
            .color = colors->text,
            .bg_color = colors->background,
            .size = 2,
            .h_align = TEXT_ALIGN_CENTER,
            .v_align = TEXT_ALIGN_MIDDLE,
            .transparent = true
        };
        Display_DrawString(64, 64, theme_names[theme_display_index], &text_params);
        
        int box_size = 30;
        int box_x = (128 - box_size) / 2;
        int box_y = 90;
        Display_FillRect(box_x, box_y, box_size, box_size, colors->background);
        Display_DrawRect(box_x, box_y, box_size, box_size, colors->border);
        
        int inner_size = 20;
        int inner_x = box_x + (box_size - inner_size) / 2;
        int inner_y = box_y + (box_size - inner_size) / 2;
        Display_FillRect(inner_x, inner_y, inner_size, inner_size, colors->foreground);
        
        int dot_size = 10;
        int dot_x = box_x + (box_size - dot_size) / 2;
        int dot_y = box_y + (box_size - dot_size) / 2;
        Display_FillRect(dot_x, dot_y, dot_size, dot_size, colors->primary);
        
        int dot_spacing = 12;
        int dots_start_x = 64 - (THEME_COUNT * dot_spacing / 2);
        for (int i = 0; i < THEME_COUNT; i++) {
            int x = dots_start_x + i * dot_spacing;
            if (i == theme_display_index) {
                Display_FillCircle(x, 140, 4, colors->highlight);
            } else {
                Display_FillCircle(x, 140, 2, colors->secondary);
            }
        }
        
        TextParams_t hint_params = {
            .font = DISPLAY_FONT_5x7,
            .color = colors->text,
            .bg_color = colors->background,
            .size = 1,
            .h_align = TEXT_ALIGN_CENTER,
            .v_align = TEXT_ALIGN_TOP,
            .transparent = true
        };
        Display_DrawString(64, 150, "Tilt:Select", &hint_params);
        Display_DrawString(64, 160, "Shake:Confirm  DoubleTap:Back", &hint_params);
        
        Display_EndFrame();
        need_redraw = false;
    }
    
    return SYS_NONE;
}