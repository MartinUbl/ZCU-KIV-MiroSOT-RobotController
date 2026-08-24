#include "command_queue.h"
#include <cstring>

constexpr size_t Command_Size = 4;
constexpr size_t Command_Queue_Size = 128;

static uint8_t command_queue[Command_Queue_Size][Command_Size];
static size_t command_queue_head = 0;
static size_t command_queue_tail = 0;
static size_t command_queue_count = 0;

void command_queue_init(void) {
    command_queue_head = 0;
    command_queue_tail = 0;
    command_queue_count = 0;
}

void command_queue_push(uint8_t* data) {
    if (command_queue_count < Command_Queue_Size) {
        memcpy(command_queue[command_queue_head], data, Command_Size);
        command_queue_head = (command_queue_head + 1) % Command_Queue_Size;
        command_queue_count++;
    }
}

bool command_queue_pop(uint8_t* data) {
    if (command_queue_count > 0) {
        memcpy(data, command_queue[command_queue_tail], Command_Size);
        command_queue_tail = (command_queue_tail + 1) % Command_Queue_Size;
        command_queue_count--;
        return true;
    }
    return false;
}

bool command_queue_pop_discard() {
    if (command_queue_count > 0) {
        command_queue_tail = (command_queue_tail + 1) % Command_Queue_Size;
        command_queue_count--;
        return true;
    }
    return false;
}

bool command_queue_peek(uint8_t* data) {
    if (command_queue_count > 0) {
        memcpy(data, command_queue[command_queue_tail], Command_Size);
        return true;
    }
    return false;
}

uint8_t command_queue_peek_opcode(void) {
    if (command_queue_count > 0) {
        return command_queue[command_queue_tail][0];
    }
    return 0xFF; // indicate empty queue
}

bool command_queue_has_opcode(uint8_t opcode) {
    size_t index = command_queue_tail;
    for (size_t i = 0; i < command_queue_count; i++) {
        if (command_queue[index][0] == opcode) {
            return true;
        }
        index = (index + 1) % Command_Queue_Size;
    }
    return false;
}

void command_queue_clear(void) {
    command_queue_head = 0;
    command_queue_tail = 0;
    command_queue_count = 0;
}

bool command_queue_is_empty(void) {
    return command_queue_count == 0;
}
