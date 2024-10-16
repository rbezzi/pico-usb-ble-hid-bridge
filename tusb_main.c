/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board_api.h"
#include "tusb.h"

#include "btstack_run_loop.h"
#include "pico/stdlib.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+
extern void hid_app_task(void);
int btstack_main(void);
int picow_bt_init(void);

int main(void) {

  // tinyusb init
  board_init();

  printf("TinyUSB Host HID init\r\n");
  // init host stack on configured roothub port
  tuh_init(BOARD_TUH_RHPORT);

  // useless for pico
  // if (board_init_after_tusb) {
  //  board_init_after_tusb();
  // }

  // btstack init
  printf("BTStack init\r\n");

  stdio_init_all();

  int res = picow_bt_init();
  if (res){
    return -1;
  }

  printf("__one\r\n");
  btstack_main();
  printf("__two\r\n");
  // REMOVE THIS
  // btstack_run_loop_execute();
  printf("__three\r\n");

  printf("TinyUSB Host HID loop starting\r\n");

  while (1) {
    // tinyusb host task
    tuh_task();

    hid_app_task(); // this is an empty method!!
  }
}

// REMOVE ME: working example
/*
int _____btstack_main(void)
{

    board_init();

    // init host stack on configured roothub port
    tuh_init(BOARD_TUH_RHPORT);

    // HERE: these 3 below are in btstack_main()

    le_keyboard_setup();

    btstack_ring_buffer_init(&ascii_input_buffer, (uint8_t *)ascii_input_storage, sizeof(ascii_input_storage));

    hci_power_control(HCI_POWER_ON);

    while(true) {
        tuh_task();
        hid_app_task();
    }

    return 0;
}
*/
//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

void tuh_mount_cb(uint8_t dev_addr) {
  // application set-up
  printf("A device with address %d is mounted\r\n", dev_addr);
}

void tuh_umount_cb(uint8_t dev_addr) {
  // application tear-down
  printf("A device with address %d is unmounted \r\n", dev_addr);
}

