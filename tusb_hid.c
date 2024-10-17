/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021, Ha Thach (tinyusb.org)
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

#include "bsp/board_api.h"
#include "tusb.h"

#include "logger.h"

//--------------------------------------------------------------------+
// MACRO TYPEDEF CONSTANT ENUM DECLARATION
//--------------------------------------------------------------------+

#define MAX_REPORT  4

// Each HID instance can has multiple reports
static struct
{
  uint8_t report_count;
  tuh_hid_report_info_t report_info[MAX_REPORT];
} hid_info[CFG_TUH_HID];

extern void process_kbd_report(hid_keyboard_report_t const *report);
static void process_mouse_report(hid_mouse_report_t const * report);
static void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use. tuh_hid_parse_report_descriptor()
// can be used to parse common/simple enough descriptor.
// Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE, it will be skipped
// therefore report_desc = NULL, desc_len = 0
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
  LOG_INFO("USB HID device address = %d, instance = %d mounted", dev_addr, instance);

  // Interface protocol (hid_interface_protocol_enum_t)
  const char* protocol_str[] = { "None", "Keyboard", "Mouse" };
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

  LOG_INFO("USB HID Interface Protocol = %s", protocol_str[itf_protocol]);

  // By default host stack will use activate boot protocol on supported interface.
  // Therefore for this simple example, we only need to parse generic report descriptor (with built-in parser)
  if ( itf_protocol == HID_ITF_PROTOCOL_NONE )
  {
    hid_info[instance].report_count = tuh_hid_parse_report_descriptor(hid_info[instance].report_info, MAX_REPORT, desc_report, desc_len);
    LOG_INFO("USB HID has %u reports", hid_info[instance].report_count);
  }

  // request to receive report
  // tuh_hid_report_received_cb() will be invoked when report is available
  if ( !tuh_hid_receive_report(dev_addr, instance) )
  {
    LOG_ERROR("Error: cannot request to receive report");
  }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
  LOG_INFO("USB HID device address = %d, instance = %d unmounted", dev_addr, instance);
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

  switch (itf_protocol)
  {
    case HID_ITF_PROTOCOL_KEYBOARD:
      TU_LOG2("USB HID receive boot keyboard report\r\n");
      process_kbd_report( (hid_keyboard_report_t const*) report );
    break;

    case HID_ITF_PROTOCOL_MOUSE:
      TU_LOG2("USB HID receive boot mouse report\r\n");
      process_mouse_report( (hid_mouse_report_t const*) report );
    break;

    default:
      // Generic report requires matching ReportID and contents with previous parsed report info
      process_generic_report(dev_addr, instance, report, len);
    break;
  }

  // continue to request to receive report
  if ( !tuh_hid_receive_report(dev_addr, instance) )
  {
    LOG_ERROR("Error: cannot request to receive report");
  }
}

void tuh_mount_cb(uint8_t dev_addr) {
  // application set-up
  LOG_INFO("USB device (address: %d) mounted", dev_addr);
}

void tuh_umount_cb(uint8_t dev_addr) {
  // application tear-down
  LOG_INFO("USB device (address: %d) unmounted", dev_addr);
}


//--------------------------------------------------------------------+
// Keyboard
//--------------------------------------------------------------------+

// process_kbd_report moved to hog_keyboard.c

//--------------------------------------------------------------------+
// Mouse
//--------------------------------------------------------------------+

static void process_mouse_report(hid_mouse_report_t const * report)
{
  static hid_mouse_report_t prev_report = { 0 };

  //------------- button state  -------------//
  uint8_t button_changed_mask = report->buttons ^ prev_report.buttons;
  if ( button_changed_mask & report->buttons)
  {
    LOG_INFO("USB mouse buttons: %c%c%c ",
       report->buttons & MOUSE_BUTTON_LEFT   ? 'L' : '-',
       report->buttons & MOUSE_BUTTON_MIDDLE ? 'M' : '-',
       report->buttons & MOUSE_BUTTON_RIGHT  ? 'R' : '-');
  } else {
    LOG_INFO("USB mouse buttons: --- ");
  }
  LOG_INFO("USB mouse x: %d, y: %d, wheel: %d", report->x, report->y, report->wheel);
}

//--------------------------------------------------------------------+
// Generic Report
//--------------------------------------------------------------------+
static void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  (void) dev_addr;

  uint8_t const rpt_count = hid_info[instance].report_count;
  tuh_hid_report_info_t* rpt_info_arr = hid_info[instance].report_info;
  tuh_hid_report_info_t* rpt_info = NULL;

  if ( rpt_count == 1 && rpt_info_arr[0].report_id == 0)
  {
    // Simple report without report ID as 1st byte
    rpt_info = &rpt_info_arr[0];
  }else
  {
    // Composite report, 1st byte is report ID, data starts from 2nd byte
    uint8_t const rpt_id = report[0];

    // Find report id in the array
    for(uint8_t i=0; i<rpt_count; i++)
    {
      if (rpt_id == rpt_info_arr[i].report_id )
      {
        rpt_info = &rpt_info_arr[i];
        break;
      }
    }

    report++;
    len--;
  }

  if (!rpt_info)
  {
    LOG_INFO("Couldn't find report info");
    return;
  }

  // For complete list of Usage Page & Usage checkout src/class/hid/hid.h. For examples:
  // - Keyboard                     : Desktop, Keyboard
  // - Mouse                        : Desktop, Mouse
  // - Gamepad                      : Desktop, Gamepad
  // - Consumer Control (Media Key) : Consumer, Consumer Control
  // - System Control (Power key)   : Desktop, System Control
  // - Generic (vendor)             : 0xFFxx, xx
  if ( rpt_info->usage_page == HID_USAGE_PAGE_DESKTOP )
  {
    switch (rpt_info->usage)
    {
      case HID_USAGE_DESKTOP_KEYBOARD:
        LOG_INFO("HID receive keyboard report");
        // Assume keyboard follow boot report layout
        process_kbd_report( (hid_keyboard_report_t const*) report );
      break;

      case HID_USAGE_DESKTOP_MOUSE:
        LOG_INFO("HID receive mouse report");
        // Assume mouse follow boot report layout
        process_mouse_report( (hid_mouse_report_t const*) report );
      break;

      default: 
        break;
    }
  }
}
