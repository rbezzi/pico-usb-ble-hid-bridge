/*
 * Copyright (C) 2014 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BLUEKITCHEN
 * GMBH OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at
 * contact@bluekitchen-gmbh.com
 *
 */

// hog = Hid Over Gatt

#define BTSTACK_FILE__ "hog_keyboard.c"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "hog_keyboard.h"
#include "logger.h"

#include "btstack.h"

#include "pico/cyw43_arch.h"

#include "ble/gatt-service/battery_service_server.h"
#include "ble/gatt-service/device_information_service_server.h"
#include "ble/gatt-service/hids_device.h"

#define REPORT_INPUT_STORAGE_SIZE 50

// from USB HID Specification 1.1, Appendix B.1
const uint8_t hid_descriptor_keyboard_boot_mode[] = {

    0x05, 0x01,                    // Usage Page (Generic Desktop)
    0x09, 0x06,                    // Usage (Keyboard)
    0xa1, 0x01,                    // Collection (Application)

    0x85,  0x01,                   // Report ID 1

    // Modifier byte

    0x75, 0x01,                    //   Report Size (1)
    0x95, 0x08,                    //   Report Count (8)
    0x05, 0x07,                    //   Usage Page (Key codes)
    0x19, 0xe0,                    //   Usage Minimum (Keyboard LeftControl)
    0x29, 0xe7,                    //   Usage Maxium (Keyboard Right GUI)
    0x15, 0x00,                    //   Logicalf Minimum (0)
    0x25, 0x01,                    //   Logical Maximum (1)
    0x81, 0x02,                    //   Input (Data, Variable, Absolute)

    // Reserved byte

    0x75, 0x01,                    //   Report Size (1)
    0x95, 0x08,                    //   Report Count (8)
    0x81, 0x03,                    //   Input (Constant, Variable, Absolute)

    // LED report + padding

    0x95, 0x05,                    //   Report Count (5)
    0x75, 0x01,                    //   Report Size (1)
    0x05, 0x08,                    //   Usage Page (LEDs)
    0x19, 0x01,                    //   Usage Minimum (Num Lock)
    0x29, 0x05,                    //   Usage Maxium (Kana)
    0x91, 0x02,                    //   Output (Data, Variable, Absolute)

    0x95, 0x01,                    //   Report Count (1)
    0x75, 0x03,                    //   Report Size (3)
    0x91, 0x03,                    //   Output (Constant, Variable, Absolute)

    // Keycodes

    0x95, 0x06,                    //   Report Count (6)
    0x75, 0x08,                    //   Report Size (8)
    0x15, 0x00,                    //   Logical Minimum (0)
    0x25, 0xff,                    //   Logical Maximum (1)
    0x05, 0x07,                    //   Usage Page (Key codes)
    0x19, 0x00,                    //   Usage Minimum (Reserved (no event indicated))
    0x29, 0xff,                    //   Usage Maxium (Reserved)
    0x81, 0x00,                    //   Input (Data, Array)

    0xc0,                          // End collection
};

typedef struct hid_keyboard_report
{
  uint8_t modifier;   /**< Keyboard modifier (KEYBOARD_MODIFIER_* masks). */
  uint8_t reserved;   /**< Reserved for OEM use, always set to 0. */
  uint8_t keycode[6]; /**< Key codes of the currently pressed keys. */
} hid_keyboard_report_t;

static btstack_packet_callback_registration_t hci_event_callback_registration;
static btstack_packet_callback_registration_t sm_event_callback_registration;
static uint8_t battery = 100;
static hci_con_handle_t con_handle = HCI_CON_HANDLE_INVALID;
static uint8_t protocol_mode = 1;

static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

const uint8_t adv_data[] = {
    // Flags general discoverable, BR/EDR not supported
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, 0x06,
    // Name (1st byte is the length of the 2nd up to the end of the line)
    0x0d, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'H', 'I', 'D', '_', 'P', 'i', 'c', 'o', '_', 'K', 'B', 'D',
    // 16-bit Service UUIDs
    0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE & 0xff, ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE >> 8,
    // Appearance HID - Keyboard (Category 15, Sub-Category 1)
    0x03, BLUETOOTH_DATA_TYPE_APPEARANCE, 0xC1, 0x03,
};
const uint8_t adv_data_len = sizeof(adv_data);

static void le_keyboard_setup(void){

    l2cap_init();

    // setup SM: Display only
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);

    // setup ATT server
    att_server_init(profile_data, NULL, NULL);

    // setup battery service
    battery_service_server_init(battery);

    // setup device information service
    device_information_service_server_init();

    // setup HID Device service
    hids_device_init(0, hid_descriptor_keyboard_boot_mode, sizeof(hid_descriptor_keyboard_boot_mode));

    // setup advertisements
    uint16_t adv_int_min = 0x0030;
    uint16_t adv_int_max = 0x0030;
    uint8_t adv_type = 0;
    bd_addr_t null_addr;
    memset(null_addr, 0, 6);
    gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(adv_data_len, (uint8_t*) adv_data);
    gap_advertisements_enable(1);

    // register for HCI events
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // register for SM events
    sm_event_callback_registration.callback = &packet_handler;
    sm_add_event_handler(&sm_event_callback_registration);

    // register for HIDS
    hids_device_register_packet_handler(packet_handler);
}

// HID Report sending
static void send_hid_report(hid_keyboard_report_t *report){
    switch (protocol_mode){
        case 0:
            hids_device_send_boot_keyboard_input_report(con_handle, (uint8_t *)report, sizeof(hid_keyboard_report_t));
            break;
        case 1:
           hids_device_send_input_report(con_handle, (uint8_t *)report, sizeof(hid_keyboard_report_t));
           break;
        default:
            break;
    }
}

static hid_keyboard_report_t* report_input_storage[REPORT_INPUT_STORAGE_SIZE];
static btstack_ring_buffer_t report_input_buffer;

static void can_send_now(void){
    while (1){
        hid_keyboard_report_t report;
        uint32_t num_bytes_read;

        btstack_ring_buffer_read(&report_input_buffer, (uint8_t *)&report, sizeof(hid_keyboard_report_t), &num_bytes_read);
        LOG_DEBUG("num_bytes_read: %d", num_bytes_read);
        if (num_bytes_read == 0){
            break;
        }

        LOG_DEBUG("send_hid_report: modifier:[keycodes]: %2d:[ %2d, %2d, %2d, %2d, %2d, %2d ]", 
            report.modifier, report.keycode[0], report.keycode[1], report.keycode[2], report.keycode[3], report.keycode[4], report.keycode[5]);

        send_hid_report(&report);
        hids_device_request_can_send_now_event(con_handle);
    }
}

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(channel);
    UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET) return;

    switch (hci_event_packet_get_type(packet)) {
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
            bd_addr_t local_addr;
            gap_local_bd_addr(local_addr);
            LOG_INFO("BTstack up and running on %s", bd_addr_to_str(local_addr));
            break;
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            con_handle = HCI_CON_HANDLE_INVALID;
            LOG_INFO("Disconnected");
            break;
        case SM_EVENT_JUST_WORKS_REQUEST:
            LOG_INFO("Just Works requested");
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            break;
        case SM_EVENT_NUMERIC_COMPARISON_REQUEST:
            LOG_INFO("Confirming numeric comparison: %"PRIu32"", sm_event_numeric_comparison_request_get_passkey(packet));
            sm_numeric_comparison_confirm(sm_event_passkey_display_number_get_handle(packet));
            break;
        case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
            LOG_INFO("Display Passkey: %"PRIu32"", sm_event_passkey_display_number_get_passkey(packet));
            break;
        case HCI_EVENT_HIDS_META:
            switch (hci_event_hids_meta_get_subevent_code(packet)){
                case HIDS_SUBEVENT_INPUT_REPORT_ENABLE:
                    con_handle = hids_subevent_input_report_enable_get_con_handle(packet);
                    LOG_INFO("Report Characteristic Subscribed %u", hids_subevent_input_report_enable_get_enable(packet));
                    break;
                case HIDS_SUBEVENT_BOOT_KEYBOARD_INPUT_REPORT_ENABLE:
                    con_handle = hids_subevent_boot_keyboard_input_report_enable_get_con_handle(packet);
                    LOG_INFO("Boot Keyboard Characteristic Subscribed %u", hids_subevent_boot_keyboard_input_report_enable_get_enable(packet));
                    break;
                case HIDS_SUBEVENT_PROTOCOL_MODE:
                    protocol_mode = hids_subevent_protocol_mode_get_protocol_mode(packet);
                    LOG_INFO("Protocol Mode: %s mode", hids_subevent_protocol_mode_get_protocol_mode(packet) ? "Report" : "Boot");
                    break;
                case HIDS_SUBEVENT_CAN_SEND_NOW:
                    can_send_now();
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

void process_kbd_report(hid_keyboard_report_t const *report) {
    LOG_DEBUG("process_kbd_report: modifier:[keycodes]: %2d:[ %2d, %2d, %2d, %2d, %2d, %2d ]", 
        report->modifier, report->keycode[0], report->keycode[1], report->keycode[2], report->keycode[3], report->keycode[4], report->keycode[5]);
    btstack_ring_buffer_write(&report_input_buffer, (uint8_t *)report, sizeof(hid_keyboard_report_t));
    btstack_run_loop_poll_data_sources_from_irq();
    // start sending
    if (con_handle != HCI_CON_HANDLE_INVALID){
        hids_device_request_can_send_now_event(con_handle);
    }
}

int picow_bt_init(void) {
    // initialize CYW43 driver architecture (will enable BT if/because CYW43_ENABLE_BLUETOOTH == 1)
    if (cyw43_arch_init()) {
        LOG_ERROR("failed to initialise cyw43_arch");
        return -1;
    }
    return 0;
}

int btstack_main(void)
{
    le_keyboard_setup();

    btstack_ring_buffer_init(&report_input_buffer, (uint8_t *)report_input_storage, sizeof(report_input_storage));

    // turn on!
    hci_power_control(HCI_POWER_ON);

    return 0;
}

static int led_state = 0;

void hal_led_toggle(void){
    led_state = 1 - led_state;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_state);
}

