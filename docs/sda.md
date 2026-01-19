#include "output.h"
#include "input.h"
#include "motion_service.h"
#include "motion_type.h"
#include "audio_service.h"
#include "audio_type.h"
#include "lcd.h"
#include "motion_debug_ui.h"
#include "motion_debug_serial.h"
#include "menu_service.h"
void setup() {
    Serial.begin(115200);
    lcd_init();
    Input_Init();
    Output_Init();
    lcd_draw_dot(64, 80);
    MotionDebugUI_Init();
    MotionService_Init();
    MotionDebugSerial_Init();
    MenuService_Init();
}

void loop() {

​    Input_Update();

​    MotionType_t motion = MotionService_Update();
​    if (motion != MOTION_NONE)
​    {
​        MotionDebugSerial_OnMotion(motion);
​        MotionDebugUI_Update();   // ← 调试可视化
​        Audio_Play(BUZZER_ATTACK);
​          // 串口连续数据

​    }
​    MotionDebugSerial_Update();  
​    MenuService_Update(motion);
​    MenuService_Render();

​    Output_Update();
​    delay(20);
}
​    // Input_Update();
​    // MotionType_t motion = MotionService_Update();

​    // if (motion != MOTION_NONE)
​    // {
​    //     MotionDebugSerial_OnMotion(motion);
​    //     //MotionDebugUI_Update();   // ← 调试可视化
​    //     Audio_Play(BUZZER_ATTACK);
​    //       // 串口连续数据

​    // }
​    // //MotionDebugSerial_Update();  
​    // Output_Update();
​    // delay(20);



 SPI.begin(

  2,  // SCLK

  -1,  // MISO

  3,  // MOSI

  7   // CS

 );