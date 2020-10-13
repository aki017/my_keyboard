/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 *
 * Copyright 2020, Benjamin Aigner <beni@asterics-foundation.org>,<aignerb@technikum-wien.at>
 *
 * This file is mostly based on the Espressif ESP32 BLE HID example.
 * Adaption were made for:
 * * UART interface
 * * console input for testing purposes
 * * Joystick support (replacing vendor report)
 * * command input via UART for controlling the BLE interface (get & delete pairings,...)
 *
 */

/* Original license text:
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this software is
   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_hidd_prf_api.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "hid_dev.h"
#include "config.h"

#include "input_matrix.h"
#include "reporter.h"
/**
 * Brief:
 * This example Implemented BLE HID device profile related functions, in which the HID device
 * has 4 Reports (1 is mouse, 2 is keyboard and LED, 3 is Consumer Devices, 4 is Vendor devices).
 * Users can choose different reports according to their own application scenarios.
 * BLE HID profile inheritance and USB HID class.
 */

/**
 * Note:
 * 1. Win10 does not support vendor report , So SUPPORT_REPORT_VENDOR is always set to FALSE, it defines in hidd_le_prf_int.h
 * 2. Update connection parameters are not allowed during iPhone HID encryption, slave turns
 * off the ability to automatically update connection parameters during encryption.
 * 3. After our HID device is connected, the iPhones write 1 to the Report Characteristic Configuration Descriptor,
 * even if the HID encryption is not completed. This should actually be written 1 after the HID encryption is completed.
 * we modify the permissions of the Report Characteristic Configuration Descriptor to `ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE_ENCRYPTED`.
 * if you got `GATT_INSUF_ENCRYPTION` error, please ignore.
 */

#define HID_DEMO_TAG "HID_DEMO"

/** @warning Currently (07.2020) whitelisting devices is still not possible for all devices,
 * because many BT devices use resolvable random addresses, this seems to unsupported:
 * https://github.com/espressif/esp-idf/issues/1368
 * https://github.com/espressif/esp-idf/issues/2262
 * Therefore, if we enable pairing only on request, it is not possible to connect
 * to the ESP32 anymore. Either the ESP32 is visible by all devices or none.
 *
 * To circumvent any problems with this repository, if the config for
 * disabled pairing by default is active, we throw an error here.
 * @todo Check regularily for updates on the above mentioned issues.
 **/
#if CONFIG_MODULE_BT_PAIRING
#error "Sorry, currently the BT controller of the ESP32 does NOT support whitelisting. Please deactivate the pairing on demand option in make menuconfig!"
#endif

static uint16_t hid_conn_id = 0;
static bool sec_conn = false;

static void hidd_event_callback(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param);

static config_data_t config;

static uint8_t hidd_service_uuid128[] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb,
    0x34,
    0x9b,
    0x5f,
    0x80,
    0x00,
    0x00,
    0x80,
    0x00,
    0x10,
    0x00,
    0x00,
    0x12,
    0x18,
    0x00,
    0x00,
};

/** @brief Event bit, set if pairing is enabled
 * @note If MODULE_BT_PAIRING ist set in menuconfig, this bit is disable by default
 * and can be enabled via $PM1 , disabled via $PM0.
 * If MODULE_BT_PAIRING is not set, this bit will be set on boot.*/
#define SYSTEM_PAIRING_ENABLED (1 << 0)

/** @brief Event bit, set if the ESP32 is currently advertising.
 *
 * Used for determining if we need to set advertising params again,
 * when the pairing mode is changed. */
#define SYSTEM_CURRENTLY_ADVERTISING (1 << 1)

/** @brief Event group for system status */
EventGroupHandle_t eventgroup_system;

static esp_ble_adv_data_t hidd_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x000A, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = 0x03c0,   //HID Generic,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(hidd_service_uuid128),
    .p_service_uuid = hidd_service_uuid128,
    .flag = 0x6,
};

// config scan response data
///@todo Scan response is currently not used. If used, add state handling (adv start) according to ble/gatt_security_server example of Espressif
static esp_ble_adv_data_t hidd_adv_resp = {
    .set_scan_rsp = true,
    .include_name = true,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
};

static esp_ble_adv_params_t hidd_adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x30,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

uint8_t uppercase(uint8_t c)
{
    if ((c >= 'a') && (c <= 'z'))
        return (c - 'a' + 'A');
    return (c);
}

int get_int(const char *input, int index, int *value)
{
    int sign = 1, result = 0, valid = 0;

    while (input[index] == ' ')
        index++; // skip leading spaces
    if (input[index] == '-')
    {
        sign = -1;
        index++;
    }
    while ((input[index] >= '0') && (input[index] <= '9'))
    {
        result = result * 10 + input[index] - '0';
        valid = 1;
        index++;
    }
    while (input[index] == ' ')
        index++; // skip trailing spaces
    if (input[index] == ',')
        index++; // or a comma

    if (valid)
    {
        *value = result * sign;
        return (index);
    }
    return (0);
}

void update_config()
{
    nvs_handle my_handle;
    esp_err_t err = nvs_open("config_c", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
        ESP_LOGE("MAIN", "error opening NVS");
    err = nvs_set_str(my_handle, "btname", config.bt_device_name);
    if (err != ESP_OK)
        ESP_LOGE("MAIN", "error saving NVS - bt name");
    err = nvs_set_u8(my_handle, "locale", config.locale);
    if (err != ESP_OK)
        ESP_LOGE("MAIN", "error saving NVS - locale");
    printf("Committing updates in NVS ... ");
    err = nvs_commit(my_handle);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
    nvs_close(my_handle);
}

static void hidd_event_callback(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param)
{
    switch (event)
    {
    case ESP_HIDD_EVENT_REG_FINISH:
    {
        if (param->init_finish.state == ESP_HIDD_INIT_OK)
        {
            //esp_bd_addr_t rand_addr = {0x04,0x11,0x11,0x11,0x11,0x05};
            esp_ble_gap_set_device_name(config.bt_device_name);
            esp_ble_gap_config_adv_data(&hidd_adv_data);
        }
        break;
    }
    case ESP_BAT_EVENT_REG:
    {
        break;
    }
    case ESP_HIDD_EVENT_DEINIT_FINISH:
        break;
    case ESP_HIDD_EVENT_BLE_CONNECT:
    {
        ESP_LOGI(HID_DEMO_TAG, "ESP_HIDD_EVENT_BLE_CONNECT");
        hid_conn_id = param->connect.conn_id;
        xEventGroupClearBits(eventgroup_system, SYSTEM_CURRENTLY_ADVERTISING);
        break;
    }
    case ESP_HIDD_EVENT_BLE_DISCONNECT:
    {
        sec_conn = false;
        ESP_LOGI(HID_DEMO_TAG, "ESP_HIDD_EVENT_BLE_DISCONNECT");
        esp_ble_gap_start_advertising(&hidd_adv_params);
        xEventGroupSetBits(eventgroup_system, SYSTEM_CURRENTLY_ADVERTISING);
        break;
    }
    case ESP_HIDD_EVENT_BLE_VENDOR_REPORT_WRITE_EVT:
    {
        ESP_LOGI(HID_DEMO_TAG, "%s, ESP_HIDD_EVENT_BLE_VENDOR_REPORT_WRITE_EVT", __func__);
        ESP_LOG_BUFFER_HEX(HID_DEMO_TAG, param->vendor_write.data, param->vendor_write.length);
    }
    case ESP_HIDD_EVENT_BLE_LED_OUT_WRITE_EVT:
    {
        ESP_LOGI(HID_DEMO_TAG, "%s, ESP_HIDD_EVENT_BLE_LED_OUT_WRITE_EVT, keyboard LED value: %d", __func__, param->vendor_write.data[0]);
    }
    default:
        break;
    }
    return;
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&hidd_adv_params);
        xEventGroupSetBits(eventgroup_system, SYSTEM_CURRENTLY_ADVERTISING);
        break;
    case ESP_GAP_BLE_SEC_REQ_EVT:
        for (int i = 0; i < ESP_BD_ADDR_LEN; i++)
        {
            ESP_LOGD(HID_DEMO_TAG, "%x:", param->ble_security.ble_req.bd_addr[i]);
        }
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT:
        sec_conn = true;
        esp_bd_addr_t bd_addr;
        memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
        ESP_LOGI(HID_DEMO_TAG, "remote BD_ADDR: %08x%04x",
                 (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
                 (bd_addr[4] << 8) + bd_addr[5]);
        ESP_LOGI(HID_DEMO_TAG, "address type = %d", param->ble_security.auth_cmpl.addr_type);
        ESP_LOGI(HID_DEMO_TAG, "pair status = %s", param->ble_security.auth_cmpl.success ? "success" : "fail");
        if (!param->ble_security.auth_cmpl.success)
        {
            ESP_LOGE(HID_DEMO_TAG, "fail reason = 0x%x", param->ble_security.auth_cmpl.fail_reason);
        }
        else
        {
            xEventGroupClearBits(eventgroup_system, SYSTEM_CURRENTLY_ADVERTISING);
        }
#if CONFIG_MODULE_BT_PAIRING
        //add connected device to whitelist (necessary if whitelist connections only).
        if (esp_ble_gap_update_whitelist(true, bd_addr, BLE_WL_ADDR_TYPE_PUBLIC) != ESP_OK)
        {
            ESP_LOGW(HID_DEMO_TAG, "cannot add device to whitelist, with public address");
        }
        else
        {
            ESP_LOGI(HID_DEMO_TAG, "added device to whitelist");
        }
        if (esp_ble_gap_update_whitelist(true, bd_addr, BLE_WL_ADDR_TYPE_RANDOM) != ESP_OK)
        {
            ESP_LOGW(HID_DEMO_TAG, "cannot add device to whitelist, with random address");
        }
#endif
        break;
    default:
        break;
    }
}

#define CHAR(x) (x - 'a' + 4)
#define NUMB(x) (x - '1' + 0x1E)

#if LEFT
uint8_t input_map[] = {
    KC_ESCAPE, KC_1, KC_2, KC_3, KC_4, KC_5,
    KC_GRAVE, KC_Q, KC_W, KC_E, KC_R, KC_T,
    KC_TAB, KC_A, KC_S, KC_D, KC_F, KC_G,
    KC_LSHIFT, KC_Z, KC_X, KC_C, KC_V, KC_B,
    KC_NO, KC_NO, KC_TAB, KC_BSLASH, KC_DELETE, KC_LSHIFT,
    KC_SPACE, KC_LCTRL, KC_ENTER, KC_LALT, KC_NO, KC_NO};
#else
uint8_t input_map[] = {
    KC_6, KC_7, KC_8, KC_9, KC_0, KC_MINUS,
    KC_Y, KC_U, KC_I, KC_O, KC_P, KC_EQUAL,
    KC_H, KC_J, KC_K, KC_L, KC_SCOLON, KC_QUOTE,
    KC_N, KC_M, KC_COMMA, KC_DOT, KC_SLASH, KC_RSHIFT,
    KC_SPACE, KC_BSPACE, KC_LBRACKET, KC_RBRACKET, KC_NO, KC_NO,
    KC_RCTRL, KC_RALT, KC_RGUI, KC_PGDOWN, KC_NO, KC_NO};
#endif
void input_test(void *pvParameters)
{
    uint8_t ndown = 0;
    uint8_t kbdcmd[] = {0, 0, 0, 0, 0, 0};
    KeyboardModifier modifier = {0};
    while (true)
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);

        if (sec_conn == false)
        {
            continue;
        }
        ndown = 0;
        modifier.Value = 0;
        for (int i = 0; i < NBUTTON; i++)
        {
            if (input_buttons[i] == 1)
            {
                switch (input_map[i])
                {
                case KC_LCTRL:
                    modifier.LCTRL = true;
                    break;
                case KC_LSHIFT:
                    modifier.LSHIFT = true;
                    break;
                case KC_LALT:
                    modifier.LALT = true;
                    break;
                case KC_LGUI:
                    modifier.LGUI = true;
                    break;
                case KC_RCTRL:
                    modifier.RCTRL = true;
                    break;
                case KC_RSHIFT:
                    modifier.RSHIFT = true;
                    break;
                case KC_RALT:
                    modifier.RALT = true;
                    break;
                case KC_RGUI:
                    modifier.RGUI = true;
                    break;
                default:
                    kbdcmd[ndown] = input_map[i];
                    ndown++;
                }
            }
        }

        esp_hidd_send_keyboard_value(hid_conn_id, modifier.Value, kbdcmd, ndown);
    }
}

void init_reporter()
{
    esp_err_t ret;

    // Initialize FreeRTOS elements
    eventgroup_system = xEventGroupCreate();
    if (eventgroup_system == NULL)
        ESP_LOGE(HID_DEMO_TAG, "Cannot initialize event group");
        //if set in KConfig, pairing is disable by default.
        //User has to enable pairing with $PM1
#if CONFIG_MODULE_BT_PAIRING
    ESP_LOGI(HID_DEMO_TAG, "pairing disabled by default");
    xEventGroupClearBits(eventgroup_system, SYSTEM_PAIRING_ENABLED);
    hidd_adv_params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_WLST;
#else
    ESP_LOGI(HID_DEMO_TAG, "pairing enabled by default");
    xEventGroupSetBits(eventgroup_system, SYSTEM_PAIRING_ENABLED);
    hidd_adv_params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
#endif

    // Initialize NVS.
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret)
    {
        ESP_LOGE(HID_DEMO_TAG, "%s initialize controller failed\n", __func__);
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BTDM);
    if (ret)
    {
        ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
        ESP_LOGE(HID_DEMO_TAG, "%s enable controller failed\n", __func__);
        return;
    }

    ret = esp_bluedroid_init();
    if (ret)
    {
        ESP_LOGE(HID_DEMO_TAG, "%s init bluedroid failed\n", __func__);
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret)
    {
        ESP_LOGE(HID_DEMO_TAG, "%s init bluedroid failed\n", __func__);
        return;
    }

    if ((ret = esp_hidd_profile_init()) != ESP_OK)
    {
        ESP_LOGE(HID_DEMO_TAG, "%s init bluedroid failed\n", __func__);
    }

    // Read config
    nvs_handle my_handle;
    ESP_LOGI("MAIN", "loading configuration from NVS");
    ret = nvs_open("config_c", NVS_READWRITE, &my_handle);
    if (ret != ESP_OK)
        ESP_LOGE("MAIN", "error opening NVS");
    size_t available_size = MAX_BT_DEVICENAME_LENGTH;
    strcpy(config.bt_device_name, GATTS_TAG);
    strcpy(config.bt_device_name + strlen(GATTS_TAG), LEFT ? "(L)" : "(R)");
    nvs_get_str(my_handle, "btname", config.bt_device_name, &available_size);
    if (ret != ESP_OK)
    {
        ESP_LOGI("MAIN", "error reading NVS - bt name, setting to default");
        strcpy(config.bt_device_name, GATTS_TAG);
        strcpy(config.bt_device_name + strlen(GATTS_TAG), LEFT ? "(L)" : "(R)");
    }
    else
        ESP_LOGI("MAIN", "bt device name is: %s", config.bt_device_name);

    ret = nvs_get_u8(my_handle, "locale", &config.locale);
    //if(ret != ESP_OK || config.locale >= LAYOUT_MAX)
    ///@todo implement keyboard layouts.
    if (ret != ESP_OK)
    {
        ESP_LOGI("MAIN", "error reading NVS - locale, setting to US_INTERNATIONAL");
        //config.locale = LAYOUT_US_INTERNATIONAL;
    }
    else
        ESP_LOGI("MAIN", "locale code is : %d", config.locale);
    nvs_close(my_handle);
    ///@todo How to handle the locale here? We have the memory for full lookups on the ESP32, but how to communicate this with the Teensy?

    ///register the callback function to the gap module
    esp_ble_gap_register_callback(gap_event_handler);
    esp_hidd_register_callbacks(hidd_event_callback);

    /* set the security iocap & auth_req & key size & init key response key parameters to the stack*/
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND; //bonding with peer device after authentication
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;       //set the IO capability to No output No input
    uint8_t key_size = 16;                          //the key size should be 7~16 bytes
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    /* If your BLE device act as a Slave, the init_key means you hope which types of key of the master should distribute to you,
    and the response key means which key you can distribute to the Master;
    If your BLE device act as a master, the response key means you hope which types of key of the slave should distribute to you,
    and the init key means which key you can distribute to the slave. */
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

    //xTaskCreate(&uart_console_task,  "console", 4096, NULL, configMAX_PRIORITIES, NULL);
    //xTaskCreate(&uart_external_task, "external", 4096, NULL, configMAX_PRIORITIES, NULL);
    ///@todo maybe reduce stack size for blink task? 4k words for blinky :-)?
    //xTaskCreate(&blink_task, "blink", 4096, NULL, configMAX_PRIORITIES, NULL);
    xTaskCreate(&input_test, "input_test", 4096, NULL, configMAX_PRIORITIES, NULL);
}
