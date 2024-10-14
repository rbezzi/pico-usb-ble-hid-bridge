/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "btstack_event.h"
#include "hal_led.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "btstack.h"


// Start the btstack example
int btstack_main(int argc, const char * argv[]);



void picow_bt_example_main(void) {

    btstack_main(0, NULL);

}
