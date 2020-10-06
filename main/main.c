/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "input_matrix.h"
#include "debug.h"
#include "reporter.h"

void app_main(void)
{
    printf("Start App Main\n");

    output_chip_info();
    
    init_reporter();
    setup_input();

    while(true){
        scan_input();
        
        int down_count = 0;
        for(int i = 0; i < NBUTTON; i++){
            if(input_buttons[i] == 1){
                printf("%d ", i);
                down_count++;
            }
        }
        
        if(down_count > 0){
            printf("\n");
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}