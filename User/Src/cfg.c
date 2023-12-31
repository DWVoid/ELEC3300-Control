#include <main.h>
#include "machine.h"

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim8;

extern UART_HandleTypeDef huart4;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;

static MachineInit init = {
        .LeftMotor = {
                .Com = {
                        .TIM_PWM = &htim4, .TIM_FG = &htim1,
                        .TIM_PWM_Channel = TIM_CHANNEL_1, .TIM_FG_Channel = TIM_CHANNEL_1,
                        .GPIO_T_Dir = GPIOC, .GPIO_N_Dir = GPIO_PIN_8
                },
                .Drive = {
                        .IntegrationCo = 1.0, .DifferentialCo = 1.0, .ProportionalCo = 1.0, .DecayCo = 1.0,
                        .Reverse = true
                },
                .Meter = {.MaxMeasurePeriod = 1000, .WheelDiameter = 0.065}
        },
        .RightMotor = {
                .Com = {
                        .TIM_PWM = &htim3, .TIM_FG = &htim8,
                        .TIM_PWM_Channel = TIM_CHANNEL_1, .TIM_FG_Channel = TIM_CHANNEL_1,
                        .GPIO_T_Dir = GPIOC, .GPIO_N_Dir = GPIO_PIN_9
                },
                .Drive = {
                        .IntegrationCo = 1.0, .DifferentialCo = 1.0, .ProportionalCo = 1.0, .DecayCo = 1.0,
                        .Reverse = false
                },
                .Meter = {.MaxMeasurePeriod = 1000, .WheelDiameter = 0.065}
        },
        .BArray = {
        	.Center = &huart3,
            .Left = &huart4,
		    .Right = &huart2,
			.CenterName = "BARR-C",
			.LeftName = "BARR-L",
			.RightName = "BARR-R"
        },
		.DebugPort = &huart1
};

MachineInit* GetInit() { return &init; }
