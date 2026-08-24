#pragma once

#include "main.h"
#include <cstdint>
#include <cstdbool>

void command_queue_init(void);
void command_queue_push(uint8_t* data);
bool command_queue_pop(uint8_t* data);
bool command_queue_pop_discard();
bool command_queue_peek(uint8_t* data);
uint8_t command_queue_peek_opcode(void);
bool command_queue_has_opcode(uint8_t opcode);
void command_queue_clear(void);
bool command_queue_is_empty(void);
