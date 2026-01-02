#include "output.h"
#include "input.h"
#include "motion_service.h"
#include "motion_type.h"
#include "audio_service.h"
#include "audio_type.h"
#include "lcd.h"

void setup() {
    Serial.begin(115200);
    lcd_init();
    Input_Init();
    Output_Init();
    lcd_draw_dot(64, 80);
}

void loop() {
    Input_Update();



    // 映射到屏幕
    int x = 64 + IMU_me.ax  * 50;
    int y = 80 + IMU_me.ay * 50;   // 注意 Y 轴反向
    int last_x =64;
    int last_y =80;
    x = constrain(x, 10, 118);
    y = constrain(y, 10, 150);

    // 擦除旧点
    lcd_clear_dot(last_x, last_y);

    // 画新点
    lcd_draw_dot(x, y);
    last_x = x;
    last_y = y;


    MotionType_t motion = MotionService_Update();
    if (motion != MOTION_NONE)
    {
        Serial.println(motion);
        Audio_Play(BUZZER_ATTACK);
    }
    Output_Update();
    delay(20);
}
