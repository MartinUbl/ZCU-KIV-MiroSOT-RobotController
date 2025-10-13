#pragma once

#include "stdint.h"
#include "main.h"

#include <stdbool.h>

void nrf_prx_init(void);
bool nrf_poll_rx(void);
