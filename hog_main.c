/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "btstack_run_loop.h"
#include "pico/stdlib.h"

int picow_bt_example_init(void);
void picow_bt_example_main(void);

int main() {
    stdio_init_all();

    int res = picow_bt_example_init();
    if (res){
        return -1;
    }

    picow_bt_example_main();
    btstack_run_loop_execute();
}