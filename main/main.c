#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "sdkconfig.h"

#include "main.h"
#include "MCP23S17.h"

#define TEST_PIN1	GPB0
#define TEST_PIN2	GPB1

// mcp23S17 Setup
spi_bus_config_t buscfg;
spi_bus_config_t buscfg_mcp23S17;
spi_device_interface_config_t devcfg_mcp23S17;
spi_device_handle_t handle_spi_mcp23S17;
spi_transaction_t trans_mcp23S17;

/*******************************************************************************
 *
 *  Function:	   initHSPI
 *
 *  Inputs:		 None
 *  Outputs:		None
 *  Description:	Set up HSPI for MCP23S17 (IO Expander)
 *
 ******************************************************************************/
void initHSPI(void)
{
	
	// spi_bus_config_t
	buscfg.sclk_io_num = SPI_PIN_NUM_CLK;   		// GPIO pin for Spi CLocK signal, or -1 if not used.
	buscfg.mosi_io_num = SPI_PIN_NUM_MOSI;   		// GPIO pin for Master Out Slave In (=spi_d) signal, or -1 if not used.
	buscfg.miso_io_num = SPI_PIN_NUM_MISO;  	    // GPIO pin for Master In Slave Out (=spi_q) signal, or -1 if not used.O
	buscfg.quadwp_io_num = -1;  					// GPIO pin for WP (Write Protect) signal which is used as D2 in 4-bit communication modes, or -1 if not used.
	buscfg.quadhd_io_num = -1;  					// GPIO pin for HD (HolD) signal which is used as D3 in 4-bit communication modes, or -1 if not used.
	buscfg.max_transfer_sz = 0;  					// Maximum transfer size, in bytes. Defaults to 4094 if 0.
	ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &buscfg, 1)); 		// Use dma_chan 1
 
	 /****************************************************************************
     * 
     * 
     * Set Up SPI for MCP23S17 (IO Expander)
     * 
     * 
     ****************************************************************************/
	devcfg_mcp23S17.address_bits = 0;
	devcfg_mcp23S17.command_bits = 0;
	devcfg_mcp23S17.dummy_bits = 0;
	devcfg_mcp23S17.mode = 0;
	devcfg_mcp23S17.duty_cycle_pos = 0;
	devcfg_mcp23S17.cs_ena_posttrans = 0;
	devcfg_mcp23S17.cs_ena_pretrans = 0;
	devcfg_mcp23S17.clock_speed_hz = 10000;   
	devcfg_mcp23S17.spics_io_num = SPI_PIN_NUM_CS;
	devcfg_mcp23S17.flags = 0;
	devcfg_mcp23S17.queue_size = 7;
	devcfg_mcp23S17.pre_cb = NULL;
	devcfg_mcp23S17.post_cb = NULL;
	ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &devcfg_mcp23S17, &handle_spi_mcp23S17));

	// spi_transaction_t
	trans_mcp23S17.flags = 0;
	trans_mcp23S17.addr = 0;
	trans_mcp23S17.cmd = 0;
	trans_mcp23S17.length = 32;   		// 4 bytes
 	trans_mcp23S17.rxlength = 24;
	trans_mcp23S17.tx_buffer = NULL;
	trans_mcp23S17.rx_buffer = NULL;
	
}



void app_main()
{
	uint8_t state;
	
	// If using the IRQ pins from MCP23S17
	gpio_set_direction(MCP23S17_INTA, GPIO_MODE_INPUT);
	gpio_set_direction(MCP23S17_INTB, GPIO_MODE_INPUT);
	gpio_pullup_en(MCP23S17_INTA);
	gpio_pullup_en(MCP23S17_INTB); 
	
	// Setup SPI
	initHSPI();
	
	// Pass the Ax address pins vaule. I.E. if all tied low then the address passed is 0. 
	MCP23S17_Initalize(0x00);
	
	// Set port B as output and PortA as input
	mcp23S17_GpioMode(0x00, 0x00ff);  		// PORTB | PORTA	
	// Enable the pullups on PortA
	mcp23S17_PullupMode(0x00, 0x00ff);  	// PORTB | PORTA	
	
	
	mcp23S17_ClrPin(0x00, TEST_PIN1);
	mcp23S17_ClrPin(0x00, TEST_PIN2);
	
	
	while (1)
	{
		
		mcp23S17_ClrPin(0x00, TEST_PIN1);
		vTaskDelay(125 / portTICK_PERIOD_MS);
		
		mcp23S17_SetPin(0x00, TEST_PIN1);
		vTaskDelay(125 / portTICK_PERIOD_MS);
		
		// Read port A
		state = (uint8_t)(mcp23S17_ReadPorts(0x00) & 0x00ff);	
		
		printf("PortA: %x\r\n", state);

	}
	
	
	
    
}
