#include "buzzer.h"
#include "motor.h"
#include "output_feedback.h"
#include "output.h"


void Output_Init(void) {
    Buzzer_Init();
    Motor_Init();
}

void Output_Update(void) {
    Buzzer_Update();
    Motor_Update();
    OutputFeedback_Update();  // 统一反馈系统更新
}