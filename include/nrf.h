#pragma once

#include "stdint.h"
#include "main.h"

#include <stdbool.h>

void nrf_prx_init(int address);
bool nrf_poll_rx(void);
