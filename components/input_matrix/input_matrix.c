#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "input_matrix.h"

static const gpio_num_t col_pins[NCOL] = MATRIX_COLS;
static const gpio_num_t row_pins[NROW] = MATRIX_ROWS;

bool input_buttons[NBUTTON] = {false};
void setup_input() {
    for(int i = 0; i < NCOL; i++){
        ESP_ERROR_CHECK(gpio_reset_pin(col_pins[i]));
        ESP_ERROR_CHECK(gpio_set_direction(col_pins[i], GPIO_MODE_OUTPUT));
    }
    for(int i = 0; i < NROW; i++){
        ESP_ERROR_CHECK(gpio_reset_pin(row_pins[i]));
        ESP_ERROR_CHECK(gpio_set_direction(row_pins[i], GPIO_MODE_INPUT));
        ESP_ERROR_CHECK(gpio_pullup_en(row_pins[i]));
    }
}
void scan_input(){
    for(int i = 0; i < NCOL; i++){
        for(int j = 0; j < NCOL; j++){
            ESP_ERROR_CHECK(gpio_set_level(col_pins[j], i == j ? 0 : 1));
        }
        
        vTaskDelay(1 / portTICK_PERIOD_MS);

        for(int j = 0; j < NROW; j++){
            int level = gpio_get_level(row_pins[j]);
            input_buttons[j*NROW+i] = level > 0 ? 0 : 1;
        }
    }
}