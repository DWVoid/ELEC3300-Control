/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BLE_R_TX_Pin GPIO_PIN_2
#define BLE_R_TX_GPIO_Port GPIOA
#define BLE_R_RX_Pin GPIO_PIN_3
#define BLE_R_RX_GPIO_Port GPIOA
#define PWM_RIGHT_Pin GPIO_PIN_6
#define PWM_RIGHT_GPIO_Port GPIOA
#define BLE_C_TX_Pin GPIO_PIN_10
#define BLE_C_TX_GPIO_Port GPIOB
#define BLE_C_RX_Pin GPIO_PIN_11
#define BLE_C_RX_GPIO_Port GPIOB
#define LCD_BK_Pin GPIO_PIN_12
#define LCD_BK_GPIO_Port GPIOD
#define FG_RIGHT_Pin GPIO_PIN_6
#define FG_RIGHT_GPIO_Port GPIOC
#define V24_ENABLE_Pin GPIO_PIN_7
#define V24_ENABLE_GPIO_Port GPIOC
#define RV_LEFT_Pin GPIO_PIN_8
#define RV_LEFT_GPIO_Port GPIOC
#define RV_RIGHT_Pin GPIO_PIN_9
#define RV_RIGHT_GPIO_Port GPIOC
#define FG_LEFT_Pin GPIO_PIN_8
#define FG_LEFT_GPIO_Port GPIOA
#define BLE_L_TX_Pin GPIO_PIN_10
#define BLE_L_TX_GPIO_Port GPIOC
#define BLE_L_RX_Pin GPIO_PIN_11
#define BLE_L_RX_GPIO_Port GPIOC
#define PWM_LEFT_Pin GPIO_PIN_6
#define PWM_LEFT_GPIO_Port GPIOB
#define GIRO_I2C_SCL_Pin GPIO_PIN_8
#define GIRO_I2C_SCL_GPIO_Port GPIOB
#define GIRO_I2C_SDA_Pin GPIO_PIN_9
#define GIRO_I2C_SDA_GPIO_Port GPIOB
#define LCD_RST_Pin GPIO_PIN_1
#define LCD_RST_GPIO_Port GPIOE
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
