#pragma once

#include "main.h"

enum class MotorDirection {
    Stop,
    Forward,
    Backward
};

void motors_init(void);
void motors_set_speed_left(MotorDirection dir, uint8_t speed);
void motors_set_speed_right(MotorDirection dir, uint8_t speed);
