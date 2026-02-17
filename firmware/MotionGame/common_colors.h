// common_colors.h
#ifndef COMMON_COLORS_H
#define COMMON_COLORS_H

#include <stdint.h>

// 统一颜色结构体（所有应用共用）
typedef struct {
    uint16_t MENU_BG;        // 黑色背景
    uint16_t TEXT_COLOR;     // 白色文字
    uint16_t TITLE_COLOR;    // 品红标题
    uint16_t HINT_COLOR;     // 青色提示
    uint16_t ACCENT_COLOR;   // 品红强调
    uint16_t PROGRESS_BG;    // 紫灰进度背景
    uint16_t PROGRESS_FG;    // 品红进度条
    uint16_t ICON_COLOR;     // 品红图标
} AppColors_t;

#endif // COMMON_COLORS_H