#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
 
#include "gpio_expander.h"
 
#include "definitions.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "driver/spi_master.h"
 
#include <stdbool.h>
 
#define GPIO_MOSI 12
#define GPIO_MISO 13
#define GPIO_SCLK 14
#define GPIO_CS 15
 
#define GPIO_RED   1
#define GPIO_GREEN 4
#define GPIO_BLUE  3
 
static uint8_t state_low = 0;
static uint8_t state_high = 1;
 
#define WRITEADDR 0x40
#define IO_DIR_REG 0x00
#define GPIO_REG 0x09
 
#define SENDER_HOST SPI3_HOST
 
static const char *TAG = "GPIOEXP833";
 
esp_err_t ret;
spi_device_handle_t handle;
spi_transaction_t t;
 
uint8_t databuf[4];
 
uint8_t RGB_Color(int red_light, int green_light, int blue_light)
{
  gpio_set_level(GPIO_RED, red_light);
  gpio_set_level(GPIO_GREEN, green_light);
  gpio_set_level(GPIO_BLUE, blue_light);
  return 0;
}
 
int initgpio(task_t *_this)
{
  if(_this->state > TASKSTATE_UNINITIALIZED) {
    return -1;
  }
 
  gpio_config_t io_conf;
 
  io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = (1ULL<<GPIO_CS);
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 0;
  gpio_config(&io_conf);
 
  gpio_reset_pin(GPIO_CS);
  gpio_set_direction(GPIO_CS, GPIO_MODE_OUTPUT);
 
  gpio_reset_pin(GPIO_RED);
  gpio_reset_pin(GPIO_GREEN);
  gpio_reset_pin(GPIO_BLUE);
 
  gpio_set_direction(GPIO_RED, GPIO_MODE_OUTPUT);
  gpio_set_direction(GPIO_GREEN, GPIO_MODE_OUTPUT);
  gpio_set_direction(GPIO_BLUE, GPIO_MODE_OUTPUT);
 
  //Configuration for the SPI bus
  spi_bus_config_t buscfg={
    .mosi_io_num=GPIO_MOSI,
    .miso_io_num=GPIO_MISO,
    .sclk_io_num=GPIO_SCLK,
    .quadwp_io_num=-1,
    .quadhd_io_num=-1
  };
 
  //Configuration for the SPI device on the other side of the bus
  spi_device_interface_config_t devcfg={
    .command_bits=8,
    .address_bits=8,
    .dummy_bits=0,
    .clock_speed_hz=1000000,
    .duty_cycle_pos=128,        //50% duty cycle
    .mode=0,
    .spics_io_num=GPIO_CS, //(1ULL<<GPIO_CS),
    .cs_ena_pretrans=0,
    .cs_ena_posttrans=3,        //Keep the CS low 3 cycles after transaction, to stop slave from missing the last bit when CS has less propagation delay than CLK
    .queue_size=1,
    .flags=0,
    .pre_cb=NULL,
    .post_cb=NULL
  };
 
  //Initialize the SPI bus and add the device we want to send stuff to.
    ret=spi_bus_initialize(SENDER_HOST, &buscfg, SPI_DMA_CH_AUTO);
    assert(ret==ESP_OK);
 
    ret=spi_bus_add_device(SENDER_HOST, &devcfg, &handle);
    assert(ret==ESP_OK);
 
    t.flags = 0x0000;
    databuf[0]=0;
    t.tx_buffer=(void*)databuf;
    t.length=8;
 
    t.cmd=WRITEADDR;
    t.addr=IO_DIR_REG;
    ret=spi_device_transmit(handle, &t);
 
  _this->state = TASKSTATE_RUNNING;
    return -1;
}
 
void execgpio(task_t *_this) {
    gpio_set_level(GPIO_CS, state_low);
    t.cmd=WRITEADDR;
    t.addr=GPIO_REG;
    databuf[0]= RGB_Color(0,255,0);
    t.tx_buffer=databuf;
    ret=spi_device_transmit(handle, &t);
    //ESP_LOGI(TAG, "return value %s", t);
    gpio_set_level(GPIO_CS, state_high);
}