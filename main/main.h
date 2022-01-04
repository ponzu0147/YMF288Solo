#pragma once

#include "esp_err.h"
#include "driver/spi_master.h"

#define SPI_PIN_NUM_MISO GPIO_NUM_13
#define SPI_PIN_NUM_MOSI GPIO_NUM_12
#define SPI_PIN_NUM_CLK  GPIO_NUM_14
#define SPI_PIN_NUM_CS   GPIO_NUM_15



extern spi_bus_config_t buscfg;
extern spi_bus_config_t buscfg_mcp23S17;
extern spi_device_interface_config_t devcfg_mcp23S17;
extern spi_device_handle_t handle_spi_mcp23S17;
extern spi_transaction_t trans_mcp23S17;


#ifndef TRUE
#define	TRUE	(1)
#endif
#ifndef FALSE
#define	FALSE	(0)
#endif


#ifndef	HIGH
#define	HIGH	(1)
#endif
#ifndef LOW
#define	LOW		(0)
#endif

#ifndef ON
#define ON		(1)
#endif

#ifndef OFF
#define OFF		(0)
#endif

#ifndef INPUT
#define INPUT		(1)
#endif

#ifndef OUTPUT
#define OUTPUT		(0)
#endif
