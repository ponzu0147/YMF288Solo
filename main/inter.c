#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "sdkconfig.h"


#define GPIO_INPUT_IO_0     33
#define GPIO_INPUT_IO_1     22
#define GPIO_INPUT_IO_2     23
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1) | (1ULL<<GPIO_INPUT_IO_2))
#define ESP_INTR_FLAG_DEFAULT 0

#define BLINK_GPIO 5


static volatile unsigned long counter = 0;
static volatile unsigned long lap_counter = 0;
static volatile unsigned long start_counter = 0;
static volatile int lap_flag = 0;
static volatile int start_flag = 0;


static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    counter++;
}


static void IRAM_ATTR gpio_isr_handler_button_start(void* arg)
{
    counter = 0;
    start_counter = 0;
    lap_counter = 0;
    start_flag = 1;
}


static void IRAM_ATTR gpio_isr_handler_button_lap(void* arg)
{
    start_counter = lap_counter;
    lap_counter = counter;
    lap_flag = 1;
}


static void blink(void* arg)
{
    uint32_t io_num = 0;
    for(;;) {
        gpio_set_level(BLINK_GPIO, io_num % 2);
        vTaskDelay(1000 / portTICK_RATE_MS);
        io_num++;
    }
}


void app_main(void)
{
    gpio_config_t io_conf;

    //interrupt of rising edge
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler_button_lap, (void*) GPIO_INPUT_IO_0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler_button_start, (void*) GPIO_INPUT_IO_1);

    gpio_isr_handler_add(GPIO_INPUT_IO_2, gpio_isr_handler, (void*) GPIO_INPUT_IO_2);

    gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    xTaskCreate(blink, "blink", 2048, NULL, 10, NULL);


    while(1) {
        if(start_flag == 1) {
            start_flag = 0;
            printf("start\n");
        }

        if (lap_flag > 0) {
            unsigned long diff = lap_counter - start_counter;

            lap_flag = 0;

            int total_sec = diff / 10000;
            int minute = total_sec / 60;
            int sec = total_sec % 60;
            int milli = (diff - (minute * 10000 * 60) - (sec * 10000) ) /10;

            char lap_str[100];
            sprintf(lap_str, "lap = %02d:%02d:%03d\n", minute, sec, milli);
            printf(lap_str);
        }

        vTaskDelay(10 / portTICK_RATE_MS);
    }
}