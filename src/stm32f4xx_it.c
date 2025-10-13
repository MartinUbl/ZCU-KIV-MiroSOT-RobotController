#include "main.h"
#include "stm32f4xx_it.h"

extern SPI_HandleTypeDef hspi1;

void NMI_Handler(void) {
    while (1)
        ;
}

void HardFault_Handler(void) {
    while (1)
        ;
}

void MemManage_Handler(void) {
    while (1)
        ;
}

void BusFault_Handler(void) {
    while (1)
        ;
}

void UsageFault_Handler(void) {
    while (1)
        ;
}

void SVC_Handler(void) {
}

void DebugMon_Handler(void) {
}

void PendSV_Handler(void) {
}

void SysTick_Handler(void) {
    HAL_IncTick();
}

void SPI1_IRQHandler(void) {
    HAL_SPI_IRQHandler(&hspi1);
}

void EXTI1_IRQHandler(void)
{
    HAL_GPIO_TogglePin(LED_Blue_GPIO_Port, LED_Blue_Pin);
  HAL_GPIO_EXTI_IRQHandler(nRF_IRQ_Pin);
}
