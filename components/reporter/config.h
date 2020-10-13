#ifndef _CONFIG_H_
#define _CONFIG_H_

#define GATTS_TAG "MyKeyboard"

#define MAX_BT_DEVICENAME_LENGTH 40

typedef struct config_data {
    char bt_device_name[MAX_BT_DEVICENAME_LENGTH];
    uint8_t locale;
} config_data_t;


#endif
