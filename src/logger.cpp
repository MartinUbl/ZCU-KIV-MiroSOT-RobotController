#include "logger.h"
#include "stm32f1xx_hal.h"
#include <cstring>
#include <cstdarg>
#include <cstdio>

extern UART_HandleTypeDef huart2;

void u2_write(const uint8_t *p, uint16_t n) {
    HAL_UART_Transmit(&huart2, reinterpret_cast<const uint8_t*>(p), n, HAL_MAX_DELAY);
}

void u2_printf(const char *fmt, ...) {
    char buf[128];

    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (n > 0) {
        u2_write(reinterpret_cast<const uint8_t*>(buf), (static_cast<size_t>(n) < sizeof(buf)) ? n : sizeof(buf));
    }
}
