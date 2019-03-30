#ifndef __NRF24_HAL_H
#define __NRF24_HAL_H


// Hardware abstraction layer for NRF24L01+ transceiver (hardware depended functions)
// GPIO pins definition
// GPIO pins initialization and control functions
// SPI transmit functions


// Peripheral libraries
#include <stm32f10x_gpio.h>
#include <stm32f10x_spi.h>


// SPI port peripheral
#define nRF24_SPI_PORT_2             SPI2
#define nRF24_SPI_PORT_1             SPI1

// nRF24 GPIO peripherals
#define nRF24_GPIO_PERIPHERALS     (RCC_APB2ENR_IOPBEN)

// CE (chip enable) pin (PB11 and PB1)
#define nRF24_CE_PORT_2              GPIOB
#define nRF24_CE_PIN_2               GPIO_Pin_11
#define nRF24_CE_L_2()               GPIO_ResetBits(nRF24_CE_PORT_2, nRF24_CE_PIN_2)
#define nRF24_CE_H_2()               GPIO_SetBits(nRF24_CE_PORT_2, nRF24_CE_PIN_2)

#define nRF24_CE_PORT_1              GPIOB
#define nRF24_CE_PIN_1               GPIO_Pin_1
#define nRF24_CE_L_1()               GPIO_ResetBits(nRF24_CE_PORT_1, nRF24_CE_PIN_1)
#define nRF24_CE_H_1()               GPIO_SetBits(nRF24_CE_PORT_1, nRF24_CE_PIN_1)

// CSN (chip select negative) pin (PB12 and PA4)
#define nRF24_CSN_PORT_2             GPIOB
#define nRF24_CSN_PIN_2              GPIO_Pin_12
#define nRF24_CSN_L_2()              GPIO_ResetBits(nRF24_CSN_PORT_2, nRF24_CSN_PIN_2)
#define nRF24_CSN_H_2()              GPIO_SetBits(nRF24_CSN_PORT_2, nRF24_CSN_PIN_2)

#define nRF24_CSN_PORT_1             GPIOA
#define nRF24_CSN_PIN_1              GPIO_Pin_4
#define nRF24_CSN_L_1()              GPIO_ResetBits(nRF24_CSN_PORT_1, nRF24_CSN_PIN_1)
#define nRF24_CSN_H_1()              GPIO_SetBits(nRF24_CSN_PORT_1, nRF24_CSN_PIN_1)

// IRQ pin (PB10 and PB0)
#define nRF24_IRQ_PORT_2             GPIOB
#define nRF24_IRQ_PIN_2              GPIO_Pin_10

#define nRF24_IRQ_PORT_1             GPIOB
#define nRF24_IRQ_PIN_1              GPIO_Pin_0


// Function prototypes
void nRF24_GPIO_Init_2(void);
uint8_t nRF24_LL_RW_2(uint8_t data);

void nRF24_GPIO_Init_1(void);
uint8_t nRF24_LL_RW_1(uint8_t data);

#endif // __NRF24_HAL_H
