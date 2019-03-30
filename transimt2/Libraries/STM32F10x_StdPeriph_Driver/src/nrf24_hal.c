#include "nrf24_hal.h"


// Configure the GPIO lines of the nRF24L01 transceiver
// note: IRQ pin must be configured separately
void nRF24_GPIO_Init_2(void) {
    GPIO_InitTypeDef PORT;

    // Enable the nRF24L01 GPIO peripherals
	RCC->APB2ENR |= nRF24_GPIO_PERIPHERALS;

    // Configure CSN pin
	PORT.GPIO_Mode = GPIO_Mode_Out_PP;
	PORT.GPIO_Speed = GPIO_Speed_2MHz;
	PORT.GPIO_Pin = nRF24_CSN_PIN_2;
	GPIO_Init(nRF24_CSN_PORT_2, &PORT);
	nRF24_CSN_H_2();

	// Configure CE pin
	PORT.GPIO_Pin = nRF24_CE_PIN_2;
	GPIO_Init(nRF24_CE_PORT_2, &PORT);
	nRF24_CE_L_2();
}

void nRF24_GPIO_Init_1(void) {
    GPIO_InitTypeDef PORT;

    // Enable the nRF24L01 GPIO peripherals
	RCC->APB2ENR |= nRF24_GPIO_PERIPHERALS;

    // Configure CSN pin
	PORT.GPIO_Mode = GPIO_Mode_Out_PP;
	PORT.GPIO_Speed = GPIO_Speed_2MHz;
	PORT.GPIO_Pin = nRF24_CSN_PIN_1;
	GPIO_Init(nRF24_CSN_PORT_1, &PORT);
	nRF24_CSN_H_1();

	// Configure CE pin
	PORT.GPIO_Pin = nRF24_CE_PIN_1;
	GPIO_Init(nRF24_CE_PORT_1, &PORT);
	nRF24_CE_L_1();
}

// Low level SPI transmit/receive function (hardware depended)
// input:
//   data - value to transmit via SPI
// return: value received from SPI
uint8_t nRF24_LL_RW_2(uint8_t data) {
	 // Wait until TX buffer is empty
	while (SPI_I2S_GetFlagStatus(nRF24_SPI_PORT_2, SPI_I2S_FLAG_TXE) == RESET);
	// Send byte to SPI (TXE cleared)
	SPI_I2S_SendData(nRF24_SPI_PORT_2, data);
	// Wait while receive buffer is empty
	while (SPI_I2S_GetFlagStatus(nRF24_SPI_PORT_2, SPI_I2S_FLAG_RXNE) == RESET);

	// Return received byte
	return (uint8_t)SPI_I2S_ReceiveData(nRF24_SPI_PORT_2);
}

uint8_t nRF24_LL_RW_1(uint8_t data) {
	 // Wait until TX buffer is empty
	while (SPI_I2S_GetFlagStatus(nRF24_SPI_PORT_1, SPI_I2S_FLAG_TXE) == RESET);
	// Send byte to SPI (TXE cleared)
	SPI_I2S_SendData(nRF24_SPI_PORT_1, data);
	// Wait while receive buffer is empty
	while (SPI_I2S_GetFlagStatus(nRF24_SPI_PORT_1, SPI_I2S_FLAG_RXNE) == RESET);

	// Return received byte
	return (uint8_t)SPI_I2S_ReceiveData(nRF24_SPI_PORT_1);
}
