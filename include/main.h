#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

void Error_Handler(void);

#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB

#define nRF_IRQ_Pin GPIO_PIN_1
#define nRF_IRQ_GPIO_Port GPIOB
#define MotPWM_B_Pin GPIO_PIN_13
#define MotPWM_B_GPIO_Port GPIOB
#define MotPWM_A_Pin GPIO_PIN_14
#define MotPWM_A_GPIO_Port GPIOB
#define nRF_ClockEnable_Pin GPIO_PIN_8
#define nRF_ClockEnable_GPIO_Port GPIOA
#define nRF_ChipSelect_Pin GPIO_PIN_9
#define nRF_ChipSelect_GPIO_Port GPIOA
#define LED_Yellow_Pin GPIO_PIN_10
#define LED_Yellow_GPIO_Port GPIOA
#define LED_Green_Pin GPIO_PIN_11
#define LED_Green_GPIO_Port GPIOA
#define LED_Blue_Pin GPIO_PIN_12
#define LED_Blue_GPIO_Port GPIOA
#define Mot_STBY_Pin GPIO_PIN_3
#define Mot_STBY_GPIO_Port GPIOB
#define Mot_BIN2_Pin GPIO_PIN_4
#define Mot_BIN2_GPIO_Port GPIOB
#define Mot_BIN1_Pin GPIO_PIN_5
#define Mot_BIN1_GPIO_Port GPIOB
#define Mot_AIN1_Pin GPIO_PIN_6
#define Mot_AIN1_GPIO_Port GPIOB
#define Mot_AIN2_Pin GPIO_PIN_7
#define Mot_AIN2_GPIO_Port GPIOB

void HAL_TIM_MspPostInit(TIM_HandleTypeDef* htim);

#ifdef __cplusplus
}
#endif
