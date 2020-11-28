#pragma once

#include "lazy.h"

typedef struct {
    TIM_HandleTypeDef *TIM_PWM;
    TIM_HandleTypeDef *TIM_FG;
    uint32_t TIM_PWM_Channel;
    uint32_t TIM_FG_Channel;
    GPIO_TypeDef *GPIO_T_Dir;
    uint16_t GPIO_N_Dir;
} MotorCommInit;

typedef struct {
    double IntegrationCo;
    double DifferentialCo;
    double ProportionalCo;
    double DecayCo;
    _Bool Reverse;
} MotorDriveInit;

typedef struct {
    int MaxMeasurePeriod;
    double WheelDiameter;
} MotorMeterInit;

typedef struct {
    MotorCommInit Com;
    MotorDriveInit Drive;
    MotorMeterInit Meter;
} MotorInit;

typedef struct  {
	UART_HandleTypeDef* Center;
	UART_HandleTypeDef* Left;
	UART_HandleTypeDef* Right;
	const char* CenterName;
	const char* LeftName;
	const char* RightName;
} BArrayInit;

typedef struct {
    MotorInit LeftMotor;
    MotorInit RightMotor;
    BArrayInit BArray;
	UART_HandleTypeDef* DebugPort;
} MachineInit;

C_BEGIN

void Machine_Configure(MachineInit* init);

void Machine_StartLoop(void);

C_END
