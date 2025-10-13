#include "main.h"
#include "stm32f1xx_hal.h"
#include "logger.h"
#include "nrf.h"
#include "motors.h"
#include "controller.h"
#include <cstring>
#include <cstdarg>
#include <cstdio>

SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart2;
TIM_HandleTypeDef htim1;

static void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_GPIO_Init(void) {

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(GPIOA, nRF_ClockEnable_Pin|nRF_ChipSelect_Pin|LED_Yellow_Pin|LED_Green_Pin|LED_Blue_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, Mot_STBY_Pin|Mot_BIN2_Pin|Mot_BIN1_Pin|Mot_AIN1_Pin|Mot_AIN2_Pin, GPIO_PIN_RESET);

    // enable interrupt detection on nRF IRQ pin (falling edge = the nRF is pulling the line low once it has data)
    GPIO_InitStruct.Pin = nRF_IRQ_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(nRF_IRQ_GPIO_Port, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);

    GPIO_InitStruct.Pin = nRF_ClockEnable_Pin|nRF_ChipSelect_Pin|LED_Yellow_Pin|LED_Green_Pin|LED_Blue_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = Mot_STBY_Pin|Mot_BIN2_Pin|Mot_BIN1_Pin|Mot_AIN1_Pin|Mot_AIN2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_GPIO_WritePin(nRF_ChipSelect_GPIO_Port, nRF_ChipSelect_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(nRF_ClockEnable_GPIO_Port, nRF_ClockEnable_Pin, GPIO_PIN_RESET);
}

static void MX_USART2_UART_Init(void) {
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 9600;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart2) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_SPI1_Init(void) {
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;

    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_TIM1_Init(void) {
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};
    TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 72-1;
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 100-1;
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&htim1) != HAL_OK) {
        Error_Handler();
    }

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

    if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;

    if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK) {
        Error_Handler();
    }

    sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
    sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
    sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
    sBreakDeadTimeConfig.DeadTime = 0;
    sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
    sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
    sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
    if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK) {
        Error_Handler();
    }

    HAL_TIM_MspPostInit(&htim1);
}

volatile bool data_available = false;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_1) {
        data_available = true;
    }
}

static void initialize_system() {

    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();

    u2_printf("ZCU-KIV MiroSOT Robot controller board\r\n");
    u2_printf("[BOOT] HAL, SystemClock, GPIO and USART2 initialized.\r\n");

    u2_printf("[BOOT] Initializing SPI1...\r\n");
    MX_SPI1_Init();

    u2_printf("[BOOT] Initializing TIM1...\r\n");
    MX_TIM1_Init();

    u2_printf("[BOOT] Initializing nRF24L01+...\r\n");
    nrf_prx_init();

    u2_printf("[BOOT] Initialization complete!\r\n\r\n");
}

int main(void) {
    initialize_system();

    motors_init();

    /*
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);

    __HAL_TIM_MOE_ENABLE(&htim1);

    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 30);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 30);

    HAL_GPIO_WritePin(Mot_STBY_GPIO_Port, Mot_STBY_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(Mot_AIN1_GPIO_Port, Mot_AIN1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(Mot_AIN2_GPIO_Port, Mot_AIN2_Pin, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(Mot_BIN1_GPIO_Port, Mot_BIN1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(Mot_BIN2_GPIO_Port, Mot_BIN2_Pin, GPIO_PIN_RESET);

    HAL_Delay(1000);

    HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_2);
    */

    motors_set_speed_left(MotorDirection::Forward, 30);
    motors_set_speed_right(MotorDirection::Forward, 30);
    HAL_Delay(300);
    motors_set_speed_left(MotorDirection::Stop, 0);
    motors_set_speed_right(MotorDirection::Stop, 0);
    HAL_Delay(300);
    motors_set_speed_left(MotorDirection::Backward, 30);
    motors_set_speed_right(MotorDirection::Backward, 30);
    HAL_Delay(300);
    motors_set_speed_left(MotorDirection::Stop, 0);
    motors_set_speed_right(MotorDirection::Stop, 0);
    HAL_Delay(300);

    while (1) {
        if (data_available) {
            data_available = !nrf_poll_rx();
        }

        controller_run();
    }
}

void Error_Handler(void) {
    __disable_irq();
    u2_printf("Error occurred!\r\n");
    while(1) {
        __WFI();
    }
}
