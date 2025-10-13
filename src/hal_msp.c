#include "main.h"

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

void HAL_MspInit(void) {
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    // no JTAG, just SWD
    __HAL_AFIO_REMAP_SWJ_NOJTAG();
}

void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (hspi->Instance == SPI1) {
        __HAL_RCC_SPI1_CLK_ENABLE();

        __HAL_RCC_GPIOA_CLK_ENABLE();

        GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_6;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef* hspi) {
    if (hspi->Instance == SPI1) {
        __HAL_RCC_SPI1_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);
    }
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim_base) {
    if (htim_base->Instance == TIM1) {
        __HAL_RCC_TIM1_CLK_ENABLE();
    }
}

void HAL_TIM_MspPostInit(TIM_HandleTypeDef* htim) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (htim->Instance == TIM1) {
        __HAL_RCC_GPIOB_CLK_ENABLE();

        GPIO_InitStruct.Pin = MotPWM_B_Pin|MotPWM_A_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* htim_base) {
    if (htim_base->Instance == TIM1) {
        __HAL_RCC_TIM1_CLK_DISABLE();
    }
}

void HAL_UART_MspInit(UART_HandleTypeDef* huart) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (huart->Instance == USART2) {
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        GPIO_InitStruct.Pin = GPIO_PIN_2;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* huart) {
    if (huart->Instance == USART2) {
        __HAL_RCC_USART2_CLK_DISABLE();

        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);
    }
}
