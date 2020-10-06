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
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"

#include "esp_hidd.h"
#include "esp_hid_gap.h"

#include "./reporter.h"

static const char *TAG = "HID_DEV_DEMO";

const unsigned char keyboardReportMap[] = {

    0x05, 0x01,  // Usage Pg (Generic Desktop)
    0x09, 0x06,  // Usage (Keyboard)
    0xA1, 0x01,  // Collection: (Application)
    0x85, 0x01,  // Report Id (1)

    0x05, 0x07,  //   Usage Pg (Key Codes)
    0x19, 0xE0,  //   Usage Min (224)
    0x29, 0xE7,  //   Usage Max (231)
    0x15, 0x00,  //   Log Min (0)
    0x25, 0x01,  //   Log Max (1)

    //   Modifier byte
    0x75, 0x01,  //   Report Size (1)
    0x95, 0x08,  //   Report Count (8)
    0x81, 0x02,  //   Input: (Data, Variable, Absolute)

    //   Reserved byte
    0x95, 0x01,  //   Report Count (1)
    0x75, 0x08,  //   Report Size (8)
    0x81, 0x01,  //   Input: (Constant)

    //   LED report
    0x95, 0x05,  //   Report Count (5)
    0x75, 0x01,  //   Report Size (1)
    0x05, 0x08,  //   Usage Pg (LEDs)
    0x19, 0x01,  //   Usage Min (1)
    0x29, 0x05,  //   Usage Max (5)
    0x91, 0x02,  //   Output: (Data, Variable, Absolute)

    //   LED report padding
    0x95, 0x01,  //   Report Count (1)
    0x75, 0x03,  //   Report Size (3)
    0x91, 0x01,  //   Output: (Constant)

    //   Key arrays (6 bytes)
    0x95, 0x06,  //   Report Count (6)
    0x75, 0x08,  //   Report Size (8)
    0x15, 0x00,  //   Log Min (0)
    0x25, 0x65,  //   Log Max (101)
    0x05, 0x07,  //   Usage Pg (Key Codes)
    0x19, 0x00,  //   Usage Min (0)
    0x29, 0x65,  //   Usage Max (101)
    0x81, 0x00,  //   Input: (Data, Array)

    0xC0,        // End Collection
};

static esp_hid_raw_report_map_t report_maps[] = {
    {.data = keyboardReportMap,
     .len = sizeof(keyboardReportMap)}};

static esp_hid_device_config_t hid_config = {
    .vendor_id = 0xfeed,
    .product_id = 0xf00d,
    .version = 0xbeaf,
    .device_name = "My Keyboard",
    .manufacturer_name = "aki017",
    .serial_number = "mykeyboard",
    .report_maps = report_maps,
    .report_maps_len = 1 // 3
};

static esp_hidd_dev_t *hid_dev = NULL;
static bool dev_connected = false;

static void hidd_event_callback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    esp_hidd_event_t event = (esp_hidd_event_t)id;
    esp_hidd_event_data_t *param = (esp_hidd_event_data_t *)event_data;

    switch (event)
    {
    case ESP_HIDD_START_EVENT:
    {
        ESP_LOGI(TAG, "START");
        esp_hid_ble_gap_adv_start();
        break;
    }
    case ESP_HIDD_CONNECT_EVENT:
    {
        ESP_LOGI(TAG, "CONNECT");
        dev_connected = true; //todo: this should be on auth_complete (in GAP)
        break;
    }
    case ESP_HIDD_PROTOCOL_MODE_EVENT:
    {
        ESP_LOGI(TAG, "PROTOCOL MODE[%u]: %s", param->protocol_mode.map_index, param->protocol_mode.protocol_mode ? "REPORT" : "BOOT");
        break;
    }
    case ESP_HIDD_CONTROL_EVENT:
    {
        ESP_LOGI(TAG, "CONTROL[%u]: %sSUSPEND", param->control.map_index, param->control.control ? "EXIT_" : "");
        break;
    }
    case ESP_HIDD_OUTPUT_EVENT:
    {
        ESP_LOGI(TAG, "OUTPUT[%u]: %8s ID: %2u, Len: %d, Data:", param->output.map_index, esp_hid_usage_str(param->output.usage), param->output.report_id, param->output.length);
        ESP_LOG_BUFFER_HEX(TAG, param->output.data, param->output.length);
        break;
    }
    case ESP_HIDD_FEATURE_EVENT:
    {
        ESP_LOGI(TAG, "FEATURE[%u]: %8s ID: %2u, Len: %d, Data:", param->feature.map_index, esp_hid_usage_str(param->feature.usage), param->feature.report_id, param->feature.length);
        ESP_LOG_BUFFER_HEX(TAG, param->feature.data, param->feature.length);
        break;
    }
    case ESP_HIDD_DISCONNECT_EVENT:
    {
        ESP_LOGI(TAG, "DISCONNECT: %s", esp_hid_disconnect_reason_str(esp_hidd_dev_transport_get(param->disconnect.dev), param->disconnect.reason));
        dev_connected = false;
        esp_hid_ble_gap_adv_start();
        break;
    }
    case ESP_HIDD_STOP_EVENT:
    {
        ESP_LOGI(TAG, "STOP");
        break;
    }
    default:
        break;
    }
    return;
}

#define HID_RPT_ID_KEYBOARD_IN        1   // Keyboard input report ID
#define HID_KEYBOARD_IN_RPT_LEN       8   // Keyboard input report Len
static void esp_hidd_send_keyboard_value(uint8_t special_key_mask,
					 uint8_t *keyboard_cmd,
					 uint8_t num_key)
{
    if (num_key > HID_KEYBOARD_IN_RPT_LEN - 2) {
        ESP_LOGE(TAG,
		 "%s(), the number of key should not be more than %d",
		 __func__, HID_KEYBOARD_IN_RPT_LEN - 2);
        return;
    }

    uint8_t buffer[HID_KEYBOARD_IN_RPT_LEN] = {0};
    buffer[0] = special_key_mask;
    for (int i = 0; i < num_key; i++) {
        buffer[i+2] = keyboard_cmd[i];
    }

    esp_hidd_dev_input_set(hid_dev, 1, HID_RPT_ID_KEYBOARD_IN,
			   buffer, sizeof(buffer));

}


void hid_demo_task(void *pvParameters)
{
    static bool send_volum_up = false;
    while (1)
    {
        if (dev_connected)
        {
            ESP_LOGI(TAG, "Send the volume");
            if (send_volum_up)
            {
                uint8_t data[] = {6};
                esp_hidd_send_keyboard_value(0, data, 1);
                //esp_hidd_send_consumer_value(HID_CONSUMER_VOLUME_UP, true);
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }
        }
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void init_reporter()
{
    esp_err_t ret;
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = esp_hid_gap_init(ESP_BT_MODE_BTDM);
    ESP_ERROR_CHECK(ret);

    ret = esp_hid_ble_gap_adv_init(ESP_HID_APPEARANCE_KEYBOARD, hid_config.device_name);
    ESP_ERROR_CHECK(ret);

    if ((ret = esp_ble_gatts_register_callback(esp_hidd_gatts_event_handler)) != ESP_OK)
    {
        ESP_LOGE(TAG, "GATTS register callback failed: %d", ret);
        return;
    }

    ESP_ERROR_CHECK(esp_hidd_dev_init(&hid_config, ESP_HID_TRANSPORT_BLE, hidd_event_callback, &hid_dev));
    xTaskCreate(&hid_demo_task, "hid_task", 2048, NULL, 2, NULL);
}