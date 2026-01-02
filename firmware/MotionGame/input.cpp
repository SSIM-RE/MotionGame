#include "input.h"
#include "mpu6050.h"
//#include "motion_detect.h"

void Input_Init(void)
{
    MPU6050_Init();
    //MotionDetect_Init();
}

void Input_Update(void)
{
    MPU6050_Update();
   // MotionDetect_Update();
}
