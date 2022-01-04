#pragma once

#include "esp_err.h"

#define MCP23S17_MANUF_CHIP_ADDRESS	0x40	// Default Address
#define MCP23S17_INTA		GPIO_NUM_22		// Make these input pins if using
#define MCP23S17_INTB		GPIO_NUM_23		// Make these input pins if using



#define IODIRA    (0x00)      // MCP23x17 I/O Direction Register
#define IODIRB    (0x01)      // 1 = Input (default), 0 = Output

#define IPOLA     (0x02)      // MCP23x17 Input Polarity Register
#define IPOLB     (0x03)      // 0 = Normal (default)(low reads as 0), 1 = Inverted (low reads as 1)

#define GPINTENA  (0x04)      // MCP23x17 Interrupt on Change Pin Assignements
#define GPINTENB  (0x05)      // 0 = No Interrupt on Change (default), 1 = Interrupt on Change

#define DEFVALA   (0x06)      // MCP23x17 Default Compare Register for Interrupt on Change
#define DEFVALB   (0x07)      // Opposite of what is here will trigger an interrupt (default = 0)

#define INTCONA   (0x08)      // MCP23x17 Interrupt on Change Control Register
#define INTCONB   (0x09)      // 1 = pin is compared to DEFVAL, 0 = pin is compared to previous state (default)

#define IOCON     (0x0A)      // MCP23x17 Configuration Register

#define GPPUA     (0x0C)      // MCP23x17 Weak Pull-Up Resistor Register
#define GPPUB     (0x0D)      // INPUT ONLY: 0 = No Internal 100k Pull-Up (default) 1 = Internal 100k Pull-Up 

#define INTFA     (0x0E)      // MCP23x17 Interrupt Flag Register
#define INTFB     (0x0F)      // READ ONLY: 1 = This Pin Triggered the Interrupt

#define	INTCAPA	(0x10)      // MCP23x17 Interrupt Captured Value for Port Register
#define	INTCAPB	(0x11)      // READ ONLY: State of the Pin at the Time the Interrupt Occurred

#define GPIOA	(0x12)      // MCP23x17 GPIO Port Register
#define GPIOB	(0x13)      // Value on the Port - Writing Sets Bits in the Output Latch

#define	OLATA	(0x14)      // MCP23x17 Output Latch Register
#define	OLATB	(0x15)		// 1 = Latch High, 0 = Latch Low (default) Reading Returns Latch State, Not Port Value!

// PortA Pin Referneces
#define GPA0	0
#define GPA1	1
#define GPA2	2
#define GPA3	3
#define GPA4	4
#define GPA5	5
#define GPA6	6
#define GPA7	7

// PortB Pin Referneces
#define GPB0	8
#define GPB1	9
#define GPB2	10
#define GPB3	11
#define GPB4	12
#define GPB5	13
#define GPB6	14
#define GPB7	15





void MCP23S17_Initalize(uint8_t address);
void mcp23S17_WriteByte(uint8_t address, uint8_t reg, uint8_t value);
void mcp23S17_WriteWord(uint8_t address, uint8_t reg, uint16_t data);
uint16_t mcp23S17_ReadWord(uint8_t address, uint8_t reg);
void mcp23S17_GpioPinMode(uint8_t address, uint8_t pin, uint8_t mode);
void mcp23S17_GpioMode(uint8_t address, uint16_t mode);
void mcp23S17_PullupPinMode(uint8_t address, uint8_t pin, uint8_t mode);
void mcp23S17_PullupMode(uint8_t address, uint16_t mode);

void mcp23S17_GpioPullupMode(uint8_t address, uint16_t mode);
void mcp23S17_GpioInvertPinMode(uint8_t address, uint8_t pin, uint8_t mode);
void mcp23S17_GpioInvertMode(uint8_t address, uint16_t mode);
void mcp23S17_SetPin(uint8_t address, uint8_t pin);
void mcp23S17_ClrPin(uint8_t address, uint8_t pin);
void mcp23S17_WritePorts(uint8_t address, uint16_t value);
uint16_t mcp23S17_ReadPorts(uint8_t address);
bool mcp23S17_ReadPin(uint8_t address, uint8_t gpio);







