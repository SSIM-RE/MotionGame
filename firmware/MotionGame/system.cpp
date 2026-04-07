// system.cpp
#include "system.h"
#include "menu_app.h"
#include "game_select_app.h"
#include "game_app.h"
#include "theme_app.h"
#include "wifi_controller.h"
#include "common_colors.h"
#include "cloud_api.h"
#include <Arduino.h>

// 颜色映射函数
static AppColors_t MapThemeToAppColors(const ThemeColors_t* theme_colors) {
    AppColors_t app_colors;
    
    if (theme_colors) {
        app_colors.MENU_BG = theme_colors->background;
        app_colors.TEXT_COLOR = theme_colors->text;
        app_colors.TITLE_COLOR = theme_colors->primary;
        app_colors.HINT_COLOR = theme_colors->secondary;
        app_colors.ACCENT_COLOR = theme_colors->accent;
        app_colors.PROGRESS_BG = theme_colors->foreground;
        app_colors.PROGRESS_FG = theme_colors->highlight;
        app_colors.ICON_COLOR = theme_colors->primary;
    } else {
        app_colors.MENU_BG = 0x0000;
        app_colors.TEXT_COLOR = 0xFFFF;
        app_colors.TITLE_COLOR = 0xF81F;
        app_colors.HINT_COLOR = 0x07FF;
        app_colors.ACCENT_COLOR = 0xF81F;
        app_colors.PROGRESS_BG = 0x3186;
        app_colors.PROGRESS_FG = 0xF81F;
        app_colors.ICON_COLOR = 0xF81F;
    }
    
    return app_colors;
}

static SystemState_t current_state = SYS_BOOT;
static SystemState_t previous_state = SYS_MENU;
static GameID_t selected_game = GAME_NONE;

void System_Init(void) {
    // 初始化云端模块（预留联网接口）
    Cloud_Init();
    
    const ThemeColors_t* theme_colors = Theme_GetCurrentColors();
    AppColors_t app_colors = MapThemeToAppColors(theme_colors);
    
    MenuApp_ApplyThemeColors(&app_colors);
    GameSelectApp_ApplyThemeColors(&app_colors);
    
    MenuApp_Init();
    current_state = SYS_MENU;
    previous_state = SYS_MENU;
}

void System_Update(void) {
    switch (current_state) {
        case SYS_MENU: {
            SystemState_t result = MenuApp_Update();
            if (result != SYS_NONE) {
                previous_state = current_state;
                
                switch (result) {
                    case SYS_GAME_SELECT:
                        current_state = SYS_GAME_SELECT;
                        GameSelectApp_Init();
                        break;
                    case SYS_THEME:
                        current_state = SYS_THEME;
                        ThemeApp_Init();
                        break;
                    case SYS_WIFI_CONTROLLER:
                        current_state = SYS_WIFI_CONTROLLER;
                        WifiController_Init();
                        break;
                    case SYS_SETTINGS:
                        current_state = SYS_SETTINGS;
                        // SettingsApp_Init();
                        break;
                    case SYS_ABOUT:
                        current_state = SYS_ABOUT;
                        // AboutApp_Init();
                        break;
                    default:
                        break;
                }
            }
            break;
        }
        
        case SYS_GAME_SELECT: {
            GameID_t temp_selected_game;
            SystemState_t result = GameSelectApp_Update(&temp_selected_game);
            
            if (result != SYS_NONE) {
                if (result == SYS_MENU) {
                    current_state = SYS_MENU;
                } else if (result == SYS_GAME_PLAYING) {
                    selected_game = temp_selected_game;
                    current_state = SYS_GAME_PLAYING;
                    GameApp_Init(selected_game);
                }
            }
            break;
        }
        
        case SYS_GAME_PLAYING: {
            SystemState_t result = GameApp_Update();
            
            if (result != SYS_NONE) {
                if (result == SYS_GAME_SELECT) {
                    current_state = SYS_GAME_SELECT;
                    GameSelectApp_ForceRedraw();
                    selected_game = GAME_NONE;
                }
            }
            break;
        }
        
        case SYS_THEME: {
            SystemState_t result = ThemeApp_Update();
            if (result != SYS_NONE) {
                if (result == SYS_MENU) {
                    const ThemeColors_t* theme_colors = Theme_GetCurrentColors();
                    AppColors_t app_colors = MapThemeToAppColors(theme_colors);
                    MenuApp_ApplyThemeColors(&app_colors);
                    GameSelectApp_ApplyThemeColors(&app_colors);
                    current_state = SYS_MENU;
                }
            }
            break;
        }
        
        case SYS_WIFI_CONTROLLER: {
            WifiController_Update();
            // 摇晃返回菜单（由 WifiController 内部处理）
            break;
        }
        
        case SYS_SETTINGS:
        case SYS_ABOUT:
            // 待实现
            current_state = SYS_MENU;
            break;
            
        default:
            current_state = SYS_MENU;
            break;
    }
}

void System_EnterMenu(int index) {
    current_state = SYS_MENU;
    MenuApp_SetCurrentIndex(index);
    MenuApp_ForceRedraw();
}

SystemState_t System_GetCurrentState(void) {
    return current_state;
}

SystemState_t System_GetPreviousState(void) {
    return previous_state;
}