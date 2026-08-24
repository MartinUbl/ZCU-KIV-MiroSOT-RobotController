#include "motors.h"
#include "logger.h"

extern TIM_HandleTypeDef htim1;

enum MotorChannel {
    MotorChannel_Left,
    MotorChannel_Right
};

struct MotorState {
    MotorDirection direction;
    uint8_t startSpeed;   // 0-100
    uint8_t currentSpeed; // 0-100
    uint8_t targetSpeed;  // 0-100
    uint32_t rampStartTime; // in ms
    uint32_t rampDuration;  // in ms
};

static const uint8_t MIN_START_SPEED = 20; // minimum speed to start the motor moving
static const uint32_t MAX_RAMP_DURATION_MS = 300;
static const uint32_t MAX_SPEED_CHANGE_PER_MS = 100 / MAX_RAMP_DURATION_MS; // speed units per ms

static uint8_t Clamp_Start_Speed(uint8_t speed) {
    if (speed > 0 && speed < MIN_START_SPEED) {
        return MIN_START_SPEED;
    }
    return speed;
}

static uint32_t motors_calculate_ramp_duration(uint8_t startSpeed, uint8_t targetSpeed) {
    uint8_t speedDiff = (startSpeed > targetSpeed) ? (startSpeed - targetSpeed) : (targetSpeed - startSpeed);
    return static_cast<uint32_t>(speedDiff) / MAX_SPEED_CHANGE_PER_MS;
}

static MotorState motorStates[2] = {
    { MotorDirection::Stop, 0, 0, 0, 0, 0 },
    { MotorDirection::Stop, 0, 0, 0, 0, 0 }
};

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

MotorDirection motors_get_common_direction() {
    if (motorStates[MotorChannel_Left].direction == motorStates[MotorChannel_Right].direction
        && motorStates[MotorChannel_Left].currentSpeed > 0
        && motorStates[MotorChannel_Right].currentSpeed > 0) {
        return motorStates[MotorChannel_Left].direction;
    }
    return MotorDirection::Stop;
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

void motors_set_speed_left(MotorDirection dir, uint8_t speed, bool force) {

    motorStates[MotorChannel_Left].direction = dir;

    // inverse direction for left motor (mounted mirrored)
    if (dir == MotorDirection::Forward) {
        dir = MotorDirection::Backward;
    } else if (dir == MotorDirection::Backward) {
        dir = MotorDirection::Forward;
    }

    motors_set_speed_common(dir, speed, Mot_AIN1_GPIO_Port, Mot_AIN1_Pin, Mot_AIN2_GPIO_Port, Mot_AIN2_Pin, TIM_CHANNEL_1);

    motorStates[MotorChannel_Left].currentSpeed = speed;

    if (dir == MotorDirection::Stop || force) {
        motorStates[MotorChannel_Left].targetSpeed = speed;
    }

    //u2_printf("L Speed=%u\r\n", speed);
}

void motors_set_speed_right(MotorDirection dir, uint8_t speed, bool force) {
    motors_set_speed_common(dir, speed, Mot_BIN1_GPIO_Port, Mot_BIN1_Pin, Mot_BIN2_GPIO_Port, Mot_BIN2_Pin, TIM_CHANNEL_2);

    motorStates[MotorChannel_Right].direction = dir;
    motorStates[MotorChannel_Right].currentSpeed = speed;

    if (dir == MotorDirection::Stop || force) {
        motorStates[MotorChannel_Right].targetSpeed = speed;
    }

    //u2_printf("R Speed=%u\r\n", speed);
}

static void motors_ramp_to_speed(MotorChannel channel, MotorDirection dir, uint8_t speed, uint32_t duration_ms) {
    if (duration_ms == 0) {
        duration_ms = motors_calculate_ramp_duration(motorStates[channel].currentSpeed, speed);
    }

    //u2_printf("Motor %s: Ramping from speed %u to speed %u in %u ms\r\n", channel == MotorChannel_Left ? "LEFT" : "RIGHT", motorStates[channel].currentSpeed, speed, duration_ms);

    if (dir != MotorDirection::Stop) {
        motorStates[channel].startSpeed = Clamp_Start_Speed(motorStates[channel].currentSpeed);
        motorStates[channel].direction = dir;
    }
    else {
        motorStates[channel].startSpeed = motorStates[channel].currentSpeed;
    }
    motorStates[channel].targetSpeed = speed;
    motorStates[channel].rampStartTime = HAL_GetTick();
    motorStates[channel].rampDuration = duration_ms;
}

void motors_ramp_to_speed_left(MotorDirection dir, uint8_t speed, uint32_t duration_ms) {
    motors_ramp_to_speed(MotorChannel_Left, dir, speed, duration_ms);
}

void motors_ramp_to_speed_right(MotorDirection dir, uint8_t speed, uint32_t duration_ms) {
    motors_ramp_to_speed(MotorChannel_Right, dir, speed, duration_ms);
}

void motors_update() {
    uint32_t now = HAL_GetTick();

    for (int i = 0; i < 2; i++) {
        MotorState& state = motorStates[i];

        if (state.currentSpeed != state.targetSpeed /*&& state.direction != MotorDirection::Stop*/) {
            if (now >= state.rampStartTime + state.rampDuration) {
                // ramp finished
                state.currentSpeed = state.targetSpeed;
            } else {
                // ramp in progress
                uint32_t elapsed = now - state.rampStartTime;
                if (state.targetSpeed > state.startSpeed)
                    state.currentSpeed = state.startSpeed + static_cast<uint8_t>((static_cast<uint32_t>(state.targetSpeed - state.startSpeed) * elapsed + state.rampDuration - 1) / state.rampDuration);
                else
                    state.currentSpeed = state.startSpeed - static_cast<uint8_t>((static_cast<uint32_t>(state.startSpeed - state.targetSpeed) * elapsed + state.rampDuration - 1) / state.rampDuration);
            }

            // apply new speed
            if (i == MotorChannel_Left) {
                motors_set_speed_left(state.direction, state.currentSpeed);
            } else {
                motors_set_speed_right(state.direction, state.currentSpeed);
            }
        }
    }
}
