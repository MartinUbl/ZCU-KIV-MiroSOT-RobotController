#include "motors.h"

extern TIM_HandleTypeDef htim1;

static void motors_enable() {
    HAL_GPIO_WritePin(Mot_STBY_GPIO_Port, Mot_STBY_Pin, GPIO_PIN_SET);
}

static void motors_disable() {
    HAL_GPIO_WritePin(Mot_STBY_GPIO_Port, Mot_STBY_Pin, GPIO_PIN_RESET);
}

void motors_init(void) {
    __HAL_TIM_MOE_ENABLE(&htim1);

    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);

    motors_enable();
}

static void motors_set_speed_common(MotorDirection dir, uint8_t speed, GPIO_TypeDef* port1, uint16_t pin1, GPIO_TypeDef* port2, uint16_t pin2, uint32_t channel) {
    if (speed > 100) {
        speed = 100;
    }

    switch (dir) {
        case MotorDirection::Stop:
            HAL_GPIO_WritePin(port1, pin1, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(port2, pin2, GPIO_PIN_RESET);
            __HAL_TIM_SET_COMPARE(&htim1, channel, 0);
            HAL_TIMEx_PWMN_Stop(&htim1, channel);
            break;
        case MotorDirection::Forward:
            HAL_GPIO_WritePin(port1, pin1, GPIO_PIN_SET);
            HAL_GPIO_WritePin(port2, pin2, GPIO_PIN_RESET);
            __HAL_TIM_SET_COMPARE(&htim1, channel, speed);
            HAL_TIMEx_PWMN_Start(&htim1, channel);
            break;
        case MotorDirection::Backward:
            HAL_GPIO_WritePin(port1, pin1, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(port2, pin2, GPIO_PIN_SET);
            __HAL_TIM_SET_COMPARE(&htim1, channel, speed);
            HAL_TIMEx_PWMN_Start(&htim1, channel);
            break;
    }
}

void motors_set_speed_left(MotorDirection dir, uint8_t speed) {
    motors_set_speed_common(dir, speed, Mot_AIN1_GPIO_Port, Mot_AIN1_Pin, Mot_AIN2_GPIO_Port, Mot_AIN2_Pin, TIM_CHANNEL_1);
}

void motors_set_speed_right(MotorDirection dir, uint8_t speed) {
    motors_set_speed_common(dir, speed, Mot_BIN1_GPIO_Port, Mot_BIN1_Pin, Mot_BIN2_GPIO_Port, Mot_BIN2_Pin, TIM_CHANNEL_2);
}
