#include "output.h"
#include "input.h"
#include "motion_service.h"
#include "motion_type.h"
#include "audio_service.h"
#include "audio_type.h"
#include "motion_debug_serial.h"
#include "system.h"
#include "motion_service.h"
#include "display_driver.h"


void setup() {
    Serial.begin(115200);
    Input_Init();
    Output_Init();
    MotionService_Init();
    Display_Init();
    System_Init();
    //MotionDebugSerial_Init();
    
}
void loop() {

    Input_Update();
    System_Update();
   // MotionDebugSerial_Update();  
    Output_Update();
    delay(20);
}
 