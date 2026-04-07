// system_state.h
#pragma once

// 系统状态枚举
typedef enum {
    SYS_NONE = 0,          // 无状态变化
    SYS_BOOT,              // 启动状态
    SYS_MENU,              // 主菜单
    SYS_GAME_SELECT,       // 游戏选择菜单
    SYS_GAME_PLAYING,      // 游戏运行状态
    SYS_THEME,             // 主题设置
    SYS_SETTINGS,         // 系统设置
    SYS_ABOUT,             // 关于页面
    SYS_WIFI_CONTROLLER    // WiFi 体感控制器模式
} SystemState_t;

// 游戏 ID 枚举
typedef enum {
    GAME_NONE = 0,
    GAME_SNAKE = 1,     // 贪吃蛇
    GAME_BALANCE,       // 平衡球
    GAME_RACE,          // 赛车
    GAME_COUNT         // 游戏总数
} GameID_t;

// 主题枚举
typedef enum {
    THEME_CLASSIC = 0,
    THEME_DARK,
    THEME_LIGHT,
    THEME_RETRO,
    THEME_GAME,
    THEME_TECH,
    THEME_COUNT
} Theme_t;
