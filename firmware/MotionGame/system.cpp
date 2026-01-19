#include "system.h"
#include "menu_app.h"
#include "game_app.h"
#include <Arduino.h>
static SystemState_t current_state = SYS_BOOT;

void System_Init(void)
{
    MenuApp_Init();
    current_state = SYS_MENU;
}

void System_Update(void)
{
    switch (current_state)
    {
        case SYS_MENU:
        {
            MenuResult_t r = MenuApp_Update();
            if (r != MENU_NONE)
            {
                current_state = (SystemState_t)r;
            }
        } break;

        case SYS_GAME:
            GameApp_Update();
            break;

        case SYS_THEME:
            break;

        case SYS_SETTINGS:
            break;

        case SYS_ABOUT:
            break;

        default:
            break;
    }
}
