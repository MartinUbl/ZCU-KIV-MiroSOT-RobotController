#include "controller.h"
#include "motors.h"
#include "command_queue.h"
#include "logger.h"
#include <cstdint>
#include <cstdbool>
#include <cstdlib>


static uint32_t Last_Update_Time = 0;
static uint32_t Current_Command_End_Time = 0;
static bool Command_In_Progress = false;

static uint32_t Speed_Set = 0;

void controller_init(void) {
    command_queue_init();
    Last_Update_Time = HAL_GetTick();
}

void controller_run(void) {
    uint32_t now = HAL_GetTick();
    Last_Update_Time = now;

    if (Command_In_Progress) {
        if (now >= Current_Command_End_Time || command_queue_peek_opcode() == 0x04 /* stop now! */) {
            // command finished
            u2_printf("Command finished\r\n");
            motors_set_speed_left(MotorDirection::Stop, 0);
            motors_set_speed_right(MotorDirection::Stop, 0);
            Command_In_Progress = false;
        } else {
            // command still in progress
            return;
        }
    }

    if (!Command_In_Progress) {
        uint8_t cmd[4];
        if (command_queue_pop(cmd)) {
            switch (cmd[0]) {
                case 0x00: // Ping
                case 0x04: // Stop
                    // do nothing
                    break;
                case 0x01: { // Set speed
                    uint8_t speed = cmd[1];
                    Speed_Set = speed;
                    Command_In_Progress = false; // immediate effect
                    break;
                }
                case 0x02: { // Move
                    int16_t distance = static_cast<int16_t>(static_cast<uint16_t>(cmd[1]) | (static_cast<uint16_t>(cmd[2]) << 8));
                    u2_printf("Move distance=%d\r\n", distance);

                    MotorDirection dir = (distance >= 0) ? MotorDirection::Forward : MotorDirection::Backward;
                    motors_set_speed_left(dir, Speed_Set);
                    motors_set_speed_right(dir, Speed_Set);

                    uint32_t duration = (Speed_Set > 0) ? (static_cast<uint32_t>(abs(distance)) * 1000 / Speed_Set) : 0;
                    u2_printf("Move duration=%u ms\r\n", duration);
                    Current_Command_End_Time = now + duration;
                    Command_In_Progress = true;
                    break;
                }
                case 0x03: { // Turn
                    int16_t angle = (static_cast<int16_t>(static_cast<uint16_t>(cmd[1]) | (static_cast<uint16_t>(cmd[2]) << 8)));
                    u2_printf("Turn angle=%d\r\n", angle);

                    MotorDirection left_dir = (angle >= 0) ? MotorDirection::Forward : MotorDirection::Backward;
                    MotorDirection right_dir = (angle >= 0) ? MotorDirection::Backward : MotorDirection::Forward;
                    motors_set_speed_left(left_dir, Speed_Set);
                    motors_set_speed_right(right_dir, Speed_Set);

                    uint32_t duration = (Speed_Set > 0) ? (static_cast<uint32_t>(abs(angle)) * 1000 / (Speed_Set * 20)) : 0; // 20 is an arbitrary multiplier chosen for protocol
                    u2_printf("Turn duration=%u ms\r\n", duration);
                    Current_Command_End_Time = now + duration;
                    Command_In_Progress = true;
                    break;
                }
            }
        }
    }
}
