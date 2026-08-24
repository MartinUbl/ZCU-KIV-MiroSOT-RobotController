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

static uint8_t Control_Mode = 0x00; // default mode

void controller_init(void) {
    command_queue_init();
    Last_Update_Time = HAL_GetTick();
}

void controller_run(void) {
    uint32_t now = HAL_GetTick();
    Last_Update_Time = now;

    if (Control_Mode == 0x00) {

        if (Command_In_Progress) {
            if (now >= Current_Command_End_Time || command_queue_peek_opcode() == 0x04 /* stop now! */) {
                // command finished
                u2_printf("Command finished\r\n");
                motors_ramp_to_speed_left(MotorDirection::Stop, 0, 500);
                motors_ramp_to_speed_right(MotorDirection::Stop, 0, 500);
                Command_In_Progress = false;
            } else {
                // command still in progress
                return;
            }
        }

        if (!Command_In_Progress) {
            uint8_t cmd[4];
            if (command_queue_peek(cmd)) {
                HAL_GPIO_WritePin(LED_Green_GPIO_Port, LED_Green_Pin, GPIO_PIN_SET);
                switch (cmd[0]) {
                    case 0x00: { // Initialize
                        uint8_t mode = cmd[1];
                        command_queue_pop_discard();
                        u2_printf("Initialize command received, mode=%u\r\n", mode);

                        // mode == 0x00 - stay here, no changes
                        // mode == 0x01 - clear the queue
                        if (mode == 0x01) {
                            command_queue_init();
                            u2_printf("Command queue cleared, switching to mode=%u\r\n", mode);
                        }
                        Control_Mode = mode;
                        break;
                    }
                    case 0x04: // Stop
                    default:
                        // do nothing
                        command_queue_pop_discard();
                        break;
                    case 0x01: { // Set speed
                        uint8_t speed = cmd[1];
                        Speed_Set = speed;
                        u2_printf("SPEED=%u\r\n", Speed_Set);
                        Command_In_Progress = false; // immediate effect
                        command_queue_pop_discard();
                        break;
                    }
                    case 0x02: { // Move
                        int16_t distance = static_cast<int16_t>(static_cast<uint16_t>(cmd[1]) | (static_cast<uint16_t>(cmd[2]) << 8));

                        MotorDirection dir = (distance >= 0) ? MotorDirection::Forward : MotorDirection::Backward;

                        uint32_t duration = (Speed_Set > 0) ? (static_cast<uint32_t>(abs(distance)) * 850 / Speed_Set) : 0;

                        uint32_t ramp_duration = 200;
                        if (duration < 2 * ramp_duration) {
                            ramp_duration = duration / 2;
                        }

                        MotorDirection common_dir = motors_get_common_direction();
                        if (common_dir != MotorDirection::Stop && common_dir != dir) {
                            // different direction - stop first
                            motors_ramp_to_speed_left(MotorDirection::Stop, 0, 200);
                            motors_ramp_to_speed_right(MotorDirection::Stop, 0, 200);
                            Command_In_Progress = true;
                            Current_Command_End_Time = now + 200;
                            return;
                        }

                        motors_ramp_to_speed_left(dir, Speed_Set, ramp_duration);
                        motors_ramp_to_speed_right(dir, Speed_Set, ramp_duration);

                        u2_printf("DIST=%d, DUR=%u ms\r\n", distance, duration);
                        Current_Command_End_Time = now + duration;
                        Command_In_Progress = true;
                        command_queue_pop_discard();
                        break;
                    }
                    case 0x03: { // Turn
                        int16_t angle = (static_cast<int16_t>(static_cast<uint16_t>(cmd[1]) | (static_cast<uint16_t>(cmd[2]) << 8)));

                        MotorDirection left_dir = (angle >= 0) ? MotorDirection::Forward : MotorDirection::Backward;
                        MotorDirection right_dir = (angle >= 0) ? MotorDirection::Backward : MotorDirection::Forward;
                        motors_ramp_to_speed_left(left_dir, Speed_Set, 200);
                        motors_ramp_to_speed_right(right_dir, Speed_Set, 200);

                        uint32_t duration = (Speed_Set > 0) ? (static_cast<uint32_t>(abs(angle)) * 28 / (Speed_Set * 20)) : 0; // 20 is an arbitrary multiplier chosen for protocol
                        u2_printf("ANGLE=%d, DUR=%u ms\r\n", angle, duration);
                        Current_Command_End_Time = now + duration;
                        Command_In_Progress = true;
                        command_queue_pop_discard();
                        break;
                    }
                }
            }
            else {
                // queue empty, no command in progress
                HAL_GPIO_WritePin(LED_Green_GPIO_Port, LED_Green_Pin, GPIO_PIN_RESET);
            }
        }
    }
    else if (Control_Mode == 0x01) {

        if (Command_In_Progress) {
            if (now >= Current_Command_End_Time || command_queue_peek_opcode() == 0x04 /* stop now! */) {
                // command finished
                //u2_printf("Command finished\r\n");
                motors_ramp_to_speed_left(MotorDirection::Stop, 0, 500);
                motors_ramp_to_speed_right(MotorDirection::Stop, 0, 500);
                Command_In_Progress = false;
            } else {
                // command still in progress
                //return;
            }
        }

        uint8_t cmd[4];
        if (command_queue_peek(cmd)) {
            HAL_GPIO_WritePin(LED_Green_GPIO_Port, LED_Green_Pin, GPIO_PIN_SET);
            switch (cmd[0]) {
                case 0x00: {
                    uint8_t mode = cmd[1];
                    command_queue_pop_discard();
                    u2_printf("Initialize command received, mode=%u\r\n", mode);

                    // mode == 0x01 - stay here, no changes
                    // mode == 0x00 - clear the queue
                    if (mode == 0x00) {
                        command_queue_init();
                        u2_printf("Command queue cleared, switching to mode=%u\r\n", mode);
                    }
                    Control_Mode = mode;
                    break;
                }
                case 0x05: { // Immediate motor control
                    int8_t speed_l = cmd[1];
                    int8_t speed_r = cmd[2];
                    uint16_t timeout = static_cast<uint16_t>(cmd[3]); // in 10 ms units

                    MotorDirection dir_l = (speed_l > 0) ? MotorDirection::Forward : MotorDirection::Stop;
                    MotorDirection dir_r = (speed_r > 0) ? MotorDirection::Forward : MotorDirection::Stop;

                    //motors_ramp_to_speed_left(dir_l, speed_l, 100);
                    //motors_ramp_to_speed_right(dir_r, speed_r, 100);
                    motors_set_speed_left(dir_l, speed_l, true);
                    motors_set_speed_right(dir_r, speed_r, true);

                    //u2_printf("Immediate motor control: Speed L=%u, Speed R=%u, Timeout=%u ms\r\n", speed_l, speed_r, static_cast<uint32_t>(timeout) * 10);

                    if (timeout > 0) {
                        Current_Command_End_Time = now + static_cast<uint32_t>(timeout) * 10;
                        Command_In_Progress = true;
                    } else {
                        Command_In_Progress = false;
                    }

                    command_queue_pop_discard();
                    break;
                }
                default:
                    // unknown command in this mode
                    command_queue_pop_discard();
                    break;
            }
        }
        else {
            // queue empty, no command in progress
            HAL_GPIO_WritePin(LED_Green_GPIO_Port, LED_Green_Pin, GPIO_PIN_RESET);
        }
    }
}
