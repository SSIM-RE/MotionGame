#include "input.h"
#include "mpu6050.h"
#include "motion_service.h"

void Input_Init(void)
{
    MPU6050_Init();
    MotionService_Init();
}

void Input_Update(void)
{
    MPU6050_Update();
    MotionService_Update();
}
