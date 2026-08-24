#pragma once

#include "main.h"

enum class MotorDirection {
    Stop,
    Forward,
    Backward
};

void motors_init(void);
void motors_set_speed_left(MotorDirection dir, uint8_t speed, bool force = false);
void motors_set_speed_right(MotorDirection dir, uint8_t speed, bool force = false);
void motors_ramp_to_speed_left(MotorDirection dir, uint8_t speed, uint32_t duration_ms);
void motors_ramp_to_speed_right(MotorDirection dir, uint8_t speed, uint32_t duration_ms);
void motors_update();

MotorDirection motors_get_common_direction();
