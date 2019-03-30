#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_spi.h>

#include "uart.h"
#include "delay.h"

#include "nrf24.h"


// Define what part of demo will be compiled:
//   0 : disable
//   1 : enable
#define DEMO_RX_SINGLE      0 // Single address receiver (1 pipe)
#define DEMO_RX_MULTI       0 // Multiple address receiver (3 pipes)
#define DEMO_RX_SOLAR       0 // Solar temperature sensor receiver
#define DEMO_TX_SINGLE      0 // Single address transmitter (1 pipe)
#define DEMO_TX_MULTI       0 // Multiple address transmitter (3 pipes)
#define DEMO_RX_SINGLE_ESB  0 // Single address receiver with Enhanced ShockBurst (1 pipe)
#define DEMO_TX_SINGLE_ESB  0 // Single address transmitter with Enhanced ShockBurst (1 pipe)
#define DEMO_TX_RX_SINGLE		1 // Single address receiver transmitter(1 pipe)

// Kinda foolproof :)
#if ((DEMO_RX_SINGLE + DEMO_RX_MULTI + DEMO_RX_SOLAR + DEMO_TX_SINGLE + DEMO_TX_MULTI + DEMO_RX_SINGLE_ESB + DEMO_TX_SINGLE_ESB + DEMO_TX_RX_SINGLE) != 1)
#error "Define only one DEMO_xx, use the '1' value"
#endif


uint32_t i,j,k;
	extern uint8_t status, status_1;

// Buffer to store a payload of maximum width
uint8_t nRF24_payload_tx[32];
uint8_t nRF24_payload_rx[32];

// Pipe number
nRF24_RXResult pipe;

// Length of received payload
uint8_t payload_length;




#if ((DEMO_TX_SINGLE) || (DEMO_TX_MULTI) || (DEMO_TX_SINGLE_ESB) || (DEMO_TX_RX_SINGLE))

// Helpers for transmit mode demo

// Timeout counter (depends on the CPU speed)
// Used for not stuck waiting for IRQ
#define nRF24_WAIT_TIMEOUT         (uint32_t)0x000000FF

// Result of packet transmission
typedef enum {
	nRF24_TX_ERROR  = (uint8_t)0x00, // Unknown error
	nRF24_TX_SUCCESS,                // Packet has been transmitted successfully
	nRF24_TX_TIMEOUT,                // It was timeout during packet transmit
	nRF24_TX_MAXRT                   // Transmit failed with maximum auto retransmit count
} nRF24_TXResult;

nRF24_TXResult tx_res;

// Function to transmit data packet
// input:
//   pBuf - pointer to the buffer with data to transmit
//   length - length of the data buffer in bytes
// return: one of nRF24_TX_xx values
nRF24_TXResult nRF24_TransmitPacket(uint8_t *pBuf, uint8_t length) {
	volatile uint32_t wait = nRF24_WAIT_TIMEOUT;

	// Deassert the CE pin (in case if it still high)
	nRF24_CE_L_2();
	nRF24_CE_L_1();

	// Transfer a data from the specified buffer to the TX FIFO
	nRF24_WritePayload(pBuf, length);
	nRF24_WritePayload_1(pBuf, length);
	
	// Start a transmission by asserting CE pin (must be held at least 10us)
	nRF24_CE_H_2();
	nRF24_CE_H_1();

	// Poll the transceiver status register until one of the following flags will be set:
	//   TX_DS  - means the packet has been transmitted
	//   MAX_RT - means the maximum number of TX retransmits happened
	// note: this solution is far from perfect, better to use IRQ instead of polling the status
	
	// Deassert the CE pin (Standby-II --> Standby-I)
	nRF24_CE_L_2();
	nRF24_CE_L_1();

	if (!wait) {
		// Timeout
		return nRF24_TX_TIMEOUT;
	}

	// Check the flags in STATUS register
	UART_SendStr("[");
	UART_SendHex8(status);
	UART_SendStr("] ");
	
	UART_SendStr("[");
	UART_SendHex8(status_1);
	UART_SendStr("] ");

	// Clear pending IRQ flags
    nRF24_ClearIRQFlags();
		nRF24_ClearIRQFlags_1();

	if (status & nRF24_FLAG_MAX_RT || status_1 & nRF24_FLAG_MAX_RT ) {
		// Auto retransmit counter exceeds the programmed maximum limit (FIFO is not removed)
		return nRF24_TX_MAXRT;
	}

	if (status & nRF24_FLAG_TX_DS || status_1 & nRF24_FLAG_TX_DS) {
		// Successful transmission
		return nRF24_TX_SUCCESS;
	}

	// Some banana happens, a payload remains in the TX FIFO, flush it
	nRF24_FlushTX();
	nRF24_FlushTX_1();

	return nRF24_TX_ERROR;
}

#endif // DEMO_TX_




int main(void) {
    UART_Init(115200);
    UART_SendStr("\r\nSTM32F103RET6 is online.\r\n");


    GPIO_InitTypeDef PORT;
    SPI_InitTypeDef SPI;
		EXTI_InitTypeDef EXTI_InitStructure;
		NVIC_InitTypeDef NVIC_InitStructure;



    // Enable SPI2 peripheral
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2,ENABLE);

    // Enable SPI2 GPIO peripheral (PORTB)
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	
		// Enable SPI1 peripheral
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1,ENABLE);

    // Enable SPI1 GPIO peripheral (PORTB)
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);


    // Configure nRF24_2 IRQ pin
	PORT.GPIO_Mode  = GPIO_Mode_Out_PP;
	PORT.GPIO_Speed = GPIO_Speed_2MHz;
	PORT.GPIO_Pin   = nRF24_IRQ_PIN_2;
	GPIO_Init(nRF24_IRQ_PORT_2, &PORT);
	
		// Configure nRF24_1 IRQ pin
	PORT.GPIO_Mode  = GPIO_Mode_Out_PP;
	PORT.GPIO_Speed = GPIO_Speed_2MHz;
	PORT.GPIO_Pin   = nRF24_IRQ_PIN_1;
	GPIO_Init(nRF24_IRQ_PORT_1, &PORT);

	// Configure SPI pins (SPI2)
    PORT.GPIO_Mode  = GPIO_Mode_AF_PP;
    PORT.GPIO_Speed = GPIO_Speed_50MHz;
    PORT.GPIO_Pin   = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_Init(GPIOB, &PORT);
		
	// Configure SPI pins (SPI1)
    PORT.GPIO_Mode  = GPIO_Mode_AF_PP;
    PORT.GPIO_Speed = GPIO_Speed_50MHz;
    PORT.GPIO_Pin   = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_Init(GPIOA, &PORT);


    // Initialize SPI2
    SPI.SPI_Mode = SPI_Mode_Master;
    SPI.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
    SPI.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI.SPI_CPOL = SPI_CPOL_Low;
    SPI.SPI_CPHA = SPI_CPHA_1Edge;
    SPI.SPI_CRCPolynomial = 7;
    SPI.SPI_DataSize = SPI_DataSize_8b;
    SPI.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI.SPI_NSS = SPI_NSS_Soft;
    SPI_Init(nRF24_SPI_PORT_2, &SPI);
    SPI_NSSInternalSoftwareConfig(nRF24_SPI_PORT_2, SPI_NSSInternalSoft_Set);
    SPI_Cmd(nRF24_SPI_PORT_2, ENABLE);

		// Initialize SPI1
    SPI.SPI_Mode = SPI_Mode_Master;
    SPI.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
    SPI.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI.SPI_CPOL = SPI_CPOL_Low;
    SPI.SPI_CPHA = SPI_CPHA_1Edge;
    SPI.SPI_CRCPolynomial = 7;
    SPI.SPI_DataSize = SPI_DataSize_8b;
    SPI.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI.SPI_NSS = SPI_NSS_Soft;
    SPI_Init(nRF24_SPI_PORT_1, &SPI);
    SPI_NSSInternalSoftwareConfig(nRF24_SPI_PORT_1, SPI_NSSInternalSoft_Set);
    SPI_Cmd(nRF24_SPI_PORT_1, ENABLE);
		
				// Initialize IRQ1
		/*Init Interrupt 0*/
	
	/* Enable AFIO clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	
  /* Connect EXTI0 Line to PB0 pin */
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, nRF24_IRQ_PIN_1);
	
	 /* Configure EXTI0 line */
  EXTI_InitStructure.EXTI_Line = EXTI_Line0;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);
	
  /* Enable and set EXTI0 Interrupt to the lowest priority */
  NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	
	// Initialize IRQ2
	/*Init Interrupt 1*/
	
	/* Enable AFIO clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	
  /* Connect EXTI1 Line to PB10 pin */
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource10);
	
	 /* Configure EXTI1 line */
  EXTI_InitStructure.EXTI_Line = EXTI_Line1;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);
	
  /* Enable and set EXTI1 Interrupt to the higher priority */
  NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);	

    // Initialize delay
    Delay_Init();


    // Initialize the nRF24L01 GPIO pins
    nRF24_GPIO_Init_2();
		nRF24_GPIO_Init_1();

    // RX/TX disabled
    nRF24_CE_L_2();
		nRF24_CE_L_1();

    // Configure the nRF24L01+
    UART_SendStr("nRF24L01+ check: ");
    if (!nRF24_Check() && !nRF24_Check_1()) {
    	UART_SendStr("FAIL\r\n");
    	while (1);
    }
	UART_SendStr("OK\r\n");

    // Initialize the nRF24L01 to its default state
    nRF24_Init();
		nRF24_Init_1();

/***************************************************************************/

#if (DEMO_RX_SINGLE)

	// This is simple receiver with one RX pipe:
	//   - pipe#1 address: '0xE7 0x1C 0xE3'
	//   - payload: 5 bytes
	//   - RF channel: 115 (2515MHz)
	//   - data rate: 250kbps (minimum possible, to increase reception reliability)
	//   - CRC scheme: 2 byte

    // The transmitter sends a 5-byte packets to the address '0xE7 0x1C 0xE3' without Auto-ACK (ShockBurst disabled)

    // Disable ShockBurst for all RX pipes
    nRF24_DisableAA_1(0xFF);

    // Set RF channel
    nRF24_SetRFChannel_1(115);

    // Set data rate
    nRF24_SetDataRate_1(nRF24_DR_250kbps);

    // Set CRC scheme
    nRF24_SetCRCScheme_1(nRF24_CRC_2byte);

    // Set address width, its common for all pipes (RX and TX)
    nRF24_SetAddrWidth_1(3);

    // Configure RX PIPE#1
    static const uint8_t nRF24_ADDR[] = { 0xE7, 0x1C, 0xE3 };
    nRF24_SetAddr_1(nRF24_PIPE1, nRF24_ADDR); // program address for RX pipe #1
    nRF24_SetRXPipe_1(nRF24_PIPE1, nRF24_AA_OFF, 5); // Auto-ACK: disabled, payload length: 5 bytes

    // Set operational mode (PRX == receiver)
    nRF24_SetOperationalMode_1(nRF24_MODE_RX);

    // Wake the transceiver
    nRF24_SetPowerMode_1(nRF24_PWR_UP);

    // Put the transceiver to the RX mode
    nRF24_CE_H_1();


    // The main loop
    while (1) {
    	//
    	// Constantly poll the status of the RX FIFO and get a payload if FIFO is not empty
    	//
    	// This is far from best solution, but it's ok for testing purposes
    	// More smart way is to use the IRQ pin :)
    	//
    	if (nRF24_GetStatus_RXFIFO_1() != nRF24_STATUS_RXFIFO_EMPTY) {
    		// Get a payload from the transceiver
    		pipe = nRF24_ReadPayload_1(nRF24_payload_rx, &payload_length);

    		// Clear all pending IRQ flags
			nRF24_ClearIRQFlags_1();

			// Print a payload contents to UART
			UART_SendStr("RCV PIPE#");
			UART_SendInt(pipe);
			UART_SendStr(" PAYLOAD:>");
			UART_SendBufHex((char *)nRF24_payload_rx, payload_length);
			UART_SendStr("<\r\n");
    	}
    }

#endif // DEMO_RX_SINGLE

/***************************************************************************/

#if (DEMO_RX_MULTI)

	// This is simple receiver with multiple RX pipes:
    //   - pipe#0 address: "WBC"
    //   - pipe#0 payload: 11 bytes
	//   - pipe#1 address: '0xE7 0x1C 0xE3'
    //   - pipe#1 payload: 5 bytes
    //   - pipe#4 address: '0xE7 0x1C 0xE6' (this is pipe#1 address with different last byte)
    //   - pipe#4 payload: 32 bytes (the maximum payload length)
	//   - RF channel: 115 (2515MHz)
	//   - data rate: 250kbps (minimum possible, to increase reception reliability)
	//   - CRC scheme: 2 byte

    // The transmitter sends packets of different length to the three different logical addresses,
    // cycling them one after another, that packets comes to different pipes (0, 1 and 4)

    // Disable ShockBurst for all RX pipes
    nRF24_DisableAA(0xFF);

    // Set RF channel
    nRF24_SetRFChannel(115);

    // Set data rate
    nRF24_SetDataRate(nRF24_DR_250kbps);

    // Set CRC scheme
    nRF24_SetCRCScheme(nRF24_CRC_2byte);

    // Set address width, its common for all pipes (RX and TX)
    nRF24_SetAddrWidth(3);

    // Configure RX PIPE#0
    static const uint8_t nRF24_ADDR0[] = { 'W', 'B', 'C' };
    nRF24_SetAddr(nRF24_PIPE0, nRF24_ADDR0); // program address for RX pipe #0
    nRF24_SetRXPipe(nRF24_PIPE0, nRF24_AA_OFF, 11); // Auto-ACK: disabled, payload length: 11 bytes

    // Configure RX PIPE#1
    static const uint8_t nRF24_ADDR1[] = { 0xE7, 0x1C, 0xE3 };
    nRF24_SetAddr(nRF24_PIPE1, nRF24_ADDR1); // program address for RX pipe #1
    nRF24_SetRXPipe(nRF24_PIPE1, nRF24_AA_OFF, 5); // Auto-ACK: disabled, payload length: 5 bytes

    // Configure RX PIPE#4
    static const uint8_t nRF24_ADDR4[] = { 0xE6 };
    nRF24_SetAddr(nRF24_PIPE4, nRF24_ADDR4); // program address for RX pipe #4
    nRF24_SetRXPipe(nRF24_PIPE4, nRF24_AA_OFF, 32); // Auto-ACK: disabled, payload length: 32 bytes

    // Set operational mode (PRX == receiver)
    nRF24_SetOperationalMode(nRF24_MODE_RX);

    // Wake the transceiver
    nRF24_SetPowerMode(nRF24_PWR_UP);

    // Put the transceiver to the RX mode
    nRF24_CE_H();


    // The main loop
    while (1) {
    	//
    	// Constantly poll the status of the RX FIFO and get a payload if FIFO is not empty
    	//
    	// This is far from best solution, but it's ok for testing purposes
    	// More smart way is to use the IRQ pin :)
    	//
    	if (nRF24_GetStatus_RXFIFO() != nRF24_STATUS_RXFIFO_EMPTY) {
    		// Get a payload from the transceiver
    		pipe = nRF24_ReadPayload(nRF24_payload, &payload_length);

    		// Clear all pending IRQ flags
			nRF24_ClearIRQFlags();

			// Print a payload contents to UART
			UART_SendStr("RCV PIPE#");
			UART_SendInt(pipe);
			UART_SendStr(" PAYLOAD:>");
			UART_SendBufHex((char *)nRF24_payload, payload_length);
			UART_SendStr("<\r\n");
    	}
    }

#endif // DEMO_RX_MULTI

/***************************************************************************/

#if (DEMO_RX_SOLAR)

    // This part is to receive data packets from the old solar-powered temperature sensor

    // Set RF channel
    nRF24_SetRFChannel(110);

    // Set data rate
    nRF24_SetDataRate(nRF24_DR_1Mbps);

    // Set CRC scheme
    nRF24_SetCRCScheme(nRF24_CRC_2byte);

    // Set address width, its common for all pipes (RX and TX)
    nRF24_SetAddrWidth(5);

    // Configure RX PIPE#0
	#define RX_PAYLOAD 18
    // That address written here in reverse order because the old device that
    // transmits data uses old nRF24 "library" with errors :)
    static const uint8_t nRF24_ADDR[] = { 'T', 'k', 'l', 'o', 'W' };
    nRF24_SetAddr(nRF24_PIPE0, nRF24_ADDR); // program pipe address
    nRF24_SetRXPipe(nRF24_PIPE0, nRF24_AA_ON, RX_PAYLOAD); // Auto-ACK: enabled, payload length: 18 bytes

    // Configure TX PIPE address, this must be done for Auto-ACK (a.k.a. ShockBurst)
    // The address of TX PIPE must be same as it configured on the transmitter side
    nRF24_SetAddr(nRF24_PIPETX, nRF24_ADDR);

    // Set TX power to maximum (for more reliable Auto-ACK)
    nRF24_SetTXPower(nRF24_TXPWR_0dBm);

    // Set operational mode (PRX == receiver)
    nRF24_SetOperationalMode(nRF24_MODE_RX);

    // Wake the transceiver
    nRF24_SetPowerMode(nRF24_PWR_UP);

    // Put the transceiver to the RX mode
    nRF24_CE_H();


    int16_t temp;
    uint16_t vrefint;
    uint16_t LSI_freq;
    uint8_t TR1, TR2, TR3;
    uint8_t DR1, DR2, DR3;
    uint8_t hours, minutes, seconds;
    uint8_t day, month, year;

    // The main loop
    while (1) {
    	if (nRF24_GetStatus_RXFIFO() != nRF24_STATUS_RXFIFO_EMPTY) {
    		// Get a payload from the transceiver
    		pipe = nRF24_ReadPayload(nRF24_payload, &payload_length);

    		// Clear all pending IRQ flags
			nRF24_ClearIRQFlags();

			// Print a payload contents to UART
			UART_SendStr("RX=["); UART_SendBufHex((char *)nRF24_payload, payload_length);
			UART_SendStr("] CRC=(");
			UART_SendHex8(nRF24_payload[payload_length - 1]);
			UART_SendStr(")\r\nTemperature: ");
			temp = (nRF24_payload[0] << 8) | nRF24_payload[1];
			if (temp < 0) {
				temp *= -1;
				UART_SendChar('-');
			} else {
				UART_SendChar('+');
			}
			UART_SendInt(temp / 10); UART_SendChar('.');
			temp %= 10;
			UART_SendInt(temp % 10); UART_SendStr("C\r\n");

			UART_SendStr("Packet: #");
			UART_SendInt((uint32_t)((nRF24_payload[2] << 24) | (nRF24_payload[3] << 16) | (nRF24_payload[4] << 8) | (nRF24_payload[5])));
			UART_SendStr("\r\n");

			vrefint = (nRF24_payload[6] << 8) + nRF24_payload[7];
			UART_SendStr("Vcc: ");
			UART_SendInt(vrefint / 100);
			UART_SendChar('.');
			UART_SendInt0(vrefint % 100);
			UART_SendStr("V\r\n");

			LSI_freq = (nRF24_payload[14] << 8) + nRF24_payload[15];
			UART_SendStr("LSI: ");
	        UART_SendInt(LSI_freq);
	        UART_SendStr("Hz\r\nOBSERVE_TX:\r\n\t");
	        UART_SendHex8(nRF24_payload[16] >> 4);
	        UART_SendStr(" pckts lost\r\n\t");
	        UART_SendHex8(nRF24_payload[16] & 0x0F);
	        UART_SendStr(" retries\r\n");

	        TR1 = nRF24_payload[8];
	        TR2 = nRF24_payload[9];
	        TR3 = nRF24_payload[10];
	        DR1 = nRF24_payload[11];
	        DR2 = nRF24_payload[12];
	        DR3 = nRF24_payload[13];
	        seconds = ((TR1 >> 4) * 10) + (TR1 & 0x0F);
	        minutes = ((TR2 >> 4) * 10) + (TR2 & 0x0F);
	        hours   = (((TR3 & 0x30) >> 4) * 10) + (TR3 & 0x0F);
	        day   = ((DR1 >> 4) * 10) + (DR1 & 0x0F);
	        //dow   = DR2 >> 5;
	        month = (((DR2 & 0x1F) >> 4) * 10) + (DR2 & 0x0F);
	        year  = ((DR3 >> 4) * 10) + (DR3 & 0x0F);

	        UART_SendStr("Uptime: ");
	        UART_SendInt0(hours);
	        UART_SendChar(':');
	        UART_SendInt0(minutes);
	        UART_SendChar(':');
	        UART_SendInt0(seconds);
	        UART_SendChar(' ');
	        UART_SendInt0(day);
	        UART_SendChar('.');
	        UART_SendInt0(month);
	        UART_SendStr(".20");
	        UART_SendInt0(year);
	        UART_SendStr("\r\n");
    	}
    }
#endif // DEMO_RX_SOLAR

/***************************************************************************/

#if (DEMO_TX_SINGLE)

	// This is simple transmitter (to one logic address):
	//   - TX address: '0xE7 0x1C 0xE3'
	//   - payload: 5 bytes
	//   - RF channel: 115 (2515MHz)
	//   - data rate: 250kbps (minimum possible, to increase reception reliability)
	//   - CRC scheme: 2 byte

    // The transmitter sends a 5-byte packets to the address '0xE7 0x1C 0xE3' without Auto-ACK (ShockBurst disabled)

    // Disable ShockBurst for all RX pipes
    nRF24_DisableAA(0xFF);

    // Set RF channel
    nRF24_SetRFChannel(115);

    // Set data rate
    nRF24_SetDataRate(nRF24_DR_250kbps);

    // Set CRC scheme
    nRF24_SetCRCScheme(nRF24_CRC_2byte);

    // Set address width, its common for all pipes (RX and TX)
    nRF24_SetAddrWidth(3);

    // Configure TX PIPE
    static const uint8_t nRF24_ADDR[] = { 0xE7, 0x1C, 0xE3 };
    nRF24_SetAddr(nRF24_PIPETX, nRF24_ADDR); // program TX address

    // Set TX power (maximum)
    nRF24_SetTXPower(nRF24_TXPWR_0dBm);

    // Set operational mode (PTX == transmitter)
    nRF24_SetOperationalMode(nRF24_MODE_TX);

    // Clear any pending IRQ flags
    nRF24_ClearIRQFlags();

    // Wake the transceiver
    nRF24_SetPowerMode(nRF24_PWR_UP);


    // The main loop
    j = 0;
    payload_length = 5;
    while (1) {
    	// Prepare data packet
    	for (i = 0; i < payload_length; i++) {
    		nRF24_payload[i] = j++;
    		if (j > 0x000000FF) j = 0;
    	}

    	// Print a payload
    	UART_SendStr("PAYLOAD:>");
    	UART_SendBufHex((char *)nRF24_payload, payload_length);
    	UART_SendStr("< ... TX: ");

    	// Transmit a packet
    	tx_res = nRF24_TransmitPacket(nRF24_payload, payload_length);
    	switch (tx_res) {
			case nRF24_TX_SUCCESS:
				UART_SendStr("OK");
				break;
			case nRF24_TX_TIMEOUT:
				UART_SendStr("TIMEOUT");
				break;
			case nRF24_TX_MAXRT:
				UART_SendStr("MAX RETRANSMIT");
				break;
			default:
				UART_SendStr("ERROR");
				break;
		}
    	UART_SendStr("\r\n");

    	// Wait ~0.5s
    	Delay_ms(500);
    }

#endif // DEMO_TX_SINGLE

/***************************************************************************/

#if (DEMO_TX_MULTI)

    // This is simple transmitter (to multiple logic addresses):
	//   - TX addresses and payload lengths:
    //       'WBC', 11 bytes
    //       '0xE7 0x1C 0xE3', 5 bytes
    //       '0xE7 0x1C 0xE6', 32 bytes
	//   - RF channel: 115 (2515MHz)
	//   - data rate: 250kbps (minimum possible, to increase reception reliability)
	//   - CRC scheme: 2 byte

    // The transmitter sends a data packets to the three logic addresses without Auto-ACK (ShockBurst disabled)
    // The payload length depends on the logic address

    // Disable ShockBurst for all RX pipes
    nRF24_DisableAA(0xFF);

    // Set RF channel
    nRF24_SetRFChannel(115);

    // Set data rate
    nRF24_SetDataRate(nRF24_DR_250kbps);

    // Set CRC scheme
    nRF24_SetCRCScheme(nRF24_CRC_2byte);

    // Set address width, its common for all pipes (RX and TX)
    nRF24_SetAddrWidth(3);

    // Set TX power (maximum)
    nRF24_SetTXPower(nRF24_TXPWR_0dBm);

    // Set operational mode (PTX == transmitter)
    nRF24_SetOperationalMode(nRF24_MODE_TX);

    // Clear any pending IRQ flags
    nRF24_ClearIRQFlags();

    // Wake the transceiver
    nRF24_SetPowerMode(nRF24_PWR_UP);

    static const uint8_t nRF24_ADDR0[] = { 'W', 'B', 'C' };
    static const uint8_t nRF24_ADDR1[] = { 0xE7, 0x1C, 0xE3 };
    static const uint8_t nRF24_ADDR2[] = { 0xE7, 0x1C, 0xE6 };

    // The main loop
    j = 0; pipe = 0;
    while (1) {
    	// Logic address
    	UART_SendStr("ADDR#");
    	UART_SendInt(pipe);

    	// Configure the TX address and payload length
    	switch (pipe) {
			case 0:
				// addr #1
				nRF24_SetAddr(nRF24_PIPETX, nRF24_ADDR0);
				payload_length = 11;
				break;
			case 1:
				// addr #2
				nRF24_SetAddr(nRF24_PIPETX, nRF24_ADDR1);
				payload_length = 5;
				break;
			case 2:
				// addr #3
				nRF24_SetAddr(nRF24_PIPETX, nRF24_ADDR2);
				payload_length = 32;
				break;
			default:
				break;
		}

    	// Prepare data packet
    	for (i = 0; i < payload_length; i++) {
    		nRF24_payload[i] = j++;
    		if (j > 0x000000FF) j = 0;
    	}

    	// Print a payload
    	UART_SendStr(" PAYLOAD:>");
    	UART_SendBufHex((char *)nRF24_payload, payload_length);
    	UART_SendStr("< ... TX: ");

    	// Transmit a packet
    	tx_res = nRF24_TransmitPacket(nRF24_payload, payload_length);
    	switch (tx_res) {
			case nRF24_TX_SUCCESS:
				UART_SendStr("OK");
				break;
			case nRF24_TX_TIMEOUT:
				UART_SendStr("TIMEOUT");
				break;
			case nRF24_TX_MAXRT:
				UART_SendStr("MAX RETRANSMIT");
				break;
			default:
				UART_SendStr("ERROR");
				break;
		}
    	UART_SendStr("\r\n");

    	// Proceed to next address
    	pipe++;
    	if (pipe > 2) {
    		pipe = 0;
    	}

    	// Wait ~0.5s
    	Delay_ms(500);
    }

#endif // DEMO_TX_MULTI

/***************************************************************************/

#if (DEMO_RX_SINGLE_ESB)

    // This is simple receiver with Enhanced ShockBurst:
	//   - RX address: 'ESB'
	//   - payload: 10 bytes
	//   - RF channel: 40 (2440MHz)
	//   - data rate: 2Mbps
	//   - CRC scheme: 2 byte

    // The transmitter sends a 10-byte packets to the address 'ESB' with Auto-ACK (ShockBurst enabled)

    // Set RF channel
    nRF24_SetRFChannel(40);

    // Set data rate
    nRF24_SetDataRate(nRF24_DR_2Mbps);

    // Set CRC scheme
    nRF24_SetCRCScheme(nRF24_CRC_2byte);

    // Set address width, its common for all pipes (RX and TX)
    nRF24_SetAddrWidth(3);

    // Configure RX PIPE
    static const uint8_t nRF24_ADDR[] = { 'E', 'S', 'B' };
    nRF24_SetAddr(nRF24_PIPE1, nRF24_ADDR); // program address for pipe
    nRF24_SetRXPipe(nRF24_PIPE1, nRF24_AA_ON, 10); // Auto-ACK: enabled, payload length: 10 bytes

    // Set TX power for Auto-ACK (maximum, to ensure that transmitter will hear ACK reply)
    nRF24_SetTXPower(nRF24_TXPWR_0dBm);

    // Set operational mode (PRX == receiver)
    nRF24_SetOperationalMode(nRF24_MODE_RX);

    // Clear any pending IRQ flags
    nRF24_ClearIRQFlags();

    // Wake the transceiver
    nRF24_SetPowerMode(nRF24_PWR_UP);

    // Put the transceiver to the RX mode
    nRF24_CE_H();


    // The main loop
    while (1) {
    	//
    	// Constantly poll the status of the RX FIFO and get a payload if FIFO is not empty
    	//
    	// This is far from best solution, but it's ok for testing purposes
    	// More smart way is to use the IRQ pin :)
    	//
    	if (nRF24_GetStatus_RXFIFO() != nRF24_STATUS_RXFIFO_EMPTY) {
    		// Get a payload from the transceiver
    		pipe = nRF24_ReadPayload(nRF24_payload, &payload_length);

    		// Clear all pending IRQ flags
			nRF24_ClearIRQFlags();

			// Print a payload contents to UART
			UART_SendStr("RCV PIPE#");
			UART_SendInt(pipe);
			UART_SendStr(" PAYLOAD:>");
			UART_SendBufHex((char *)nRF24_payload, payload_length);
			UART_SendStr("<\r\n");
    	}
    }

#endif // DEMO_RX_SINGLE_ESB

/***************************************************************************/

#if (DEMO_TX_SINGLE_ESB)

    // This is simple transmitter with Enhanced ShockBurst (to one logic address):
	//   - TX address: 'ESB'
	//   - payload: 10 bytes
	//   - RF channel: 40 (2440MHz)
	//   - data rate: 2Mbps
	//   - CRC scheme: 2 byte

    // The transmitter sends a 10-byte packets to the address 'ESB' with Auto-ACK (ShockBurst enabled)

    // Set RF channel
    nRF24_SetRFChannel(40);

    // Set data rate
    nRF24_SetDataRate(nRF24_DR_2Mbps);

    // Set CRC scheme
    nRF24_SetCRCScheme(nRF24_CRC_2byte);

    // Set address width, its common for all pipes (RX and TX)
    nRF24_SetAddrWidth(3);

    // Configure TX PIPE
    static const uint8_t nRF24_ADDR[] = { 'E', 'S', 'B' };
    nRF24_SetAddr(nRF24_PIPETX, nRF24_ADDR); // program TX address
    nRF24_SetAddr(nRF24_PIPE0, nRF24_ADDR); // program address for pipe#0, must be same as TX (for Auto-ACK)

    // Set TX power (maximum)
    nRF24_SetTXPower(nRF24_TXPWR_0dBm);

    // Configure auto retransmit: 10 retransmissions with pause of 2500s in between
    nRF24_SetAutoRetr(nRF24_ARD_2500us, 10);

    // Enable Auto-ACK for pipe#0 (for ACK packets)
    nRF24_EnableAA(nRF24_PIPE0);

    // Set operational mode (PTX == transmitter)
    nRF24_SetOperationalMode(nRF24_MODE_TX);

    // Clear any pending IRQ flags
    nRF24_ClearIRQFlags();

    // Wake the transceiver
    nRF24_SetPowerMode(nRF24_PWR_UP);


    // Some variables
    uint32_t packets_lost = 0; // global counter of lost packets
    uint8_t otx;
    uint8_t otx_plos_cnt; // lost packet count
	uint8_t otx_arc_cnt; // retransmit count


    // The main loop
    payload_length = 10;
    j = 0;
    while (1) {
    	// Prepare data packet
    	for (i = 0; i < payload_length; i++) {
    		nRF24_payload[i] = j++;
    		if (j > 0x000000FF) j = 0;
    	}

    	// Print a payload
    	UART_SendStr("PAYLOAD:>");
    	UART_SendBufHex((char *)nRF24_payload, payload_length);
    	UART_SendStr("< ... TX: ");

    	// Transmit a packet
    	tx_res = nRF24_TransmitPacket(nRF24_payload, payload_length);
		otx = nRF24_GetRetransmitCounters();
		otx_plos_cnt = (otx & nRF24_MASK_PLOS_CNT) >> 4; // packets lost counter
		otx_arc_cnt  = (otx & nRF24_MASK_ARC_CNT); // auto retransmissions counter
    	switch (tx_res) {
			case nRF24_TX_SUCCESS:
				UART_SendStr("OK");
				break;
			case nRF24_TX_TIMEOUT:
				UART_SendStr("TIMEOUT");
				break;
			case nRF24_TX_MAXRT:
				UART_SendStr("MAX RETRANSMIT");
				packets_lost += otx_plos_cnt;
				nRF24_ResetPLOS();
				break;
			default:
				UART_SendStr("ERROR");
				break;
		}
    	UART_SendStr("   ARC=");
		UART_SendInt(otx_arc_cnt);
		UART_SendStr(" LOST=");
		UART_SendInt(packets_lost);
		UART_SendStr("\r\n");

    	// Wait ~0.5s
    	Delay_ms(500);
    }


#endif // DEMO_TX_SINGLE_ESB

/***************************************************************************/

#if (DEMO_TX_RX_SINGLE)

// This is simple transmitter (to one logic address):
	//   - TX address: '0xE7 0x1C 0xE3'
	//   - payload: 5 bytes
	//   - RF channel: 115 (2515MHz)
	//   - data rate: 250kbps (minimum possible, to increase reception reliability)
	//   - CRC scheme: 2 byte

    // The transmitter sends a 5-byte packets to the address '0xE7 0x1C 0xE3' without Auto-ACK (ShockBurst disabled)

    // Disable ShockBurst for all RX pipes
    nRF24_DisableAA(0xFF);

    // Set RF channel
    nRF24_SetRFChannel(118);

    // Set data rate
    nRF24_SetDataRate(nRF24_DR_250kbps);

    // Set CRC scheme
    nRF24_SetCRCScheme(nRF24_CRC_2byte);

    // Set address width, its common for all pipes (RX and TX)
    nRF24_SetAddrWidth(3);

    // Configure TX PIPE
    static const uint8_t nRF24_ADDR[] = { 0xE6, 0x1B, 0xE2 };
    nRF24_SetAddr(nRF24_PIPETX, nRF24_ADDR); // program TX address

    // Set TX power (maximum)
    nRF24_SetTXPower(nRF24_TXPWR_0dBm);

    // Set operational mode (PTX == transmitter)
    nRF24_SetOperationalMode(nRF24_MODE_TX);

    // Clear any pending IRQ flags
    nRF24_ClearIRQFlags();

    // Wake the transceiver
    nRF24_SetPowerMode(nRF24_PWR_UP);
		
		
////////////////////////RX///////////////////////////////////
		
	 // Disable ShockBurst for all RX pipes
    nRF24_DisableAA_1(0xFF);

    // Set RF channel
    nRF24_SetRFChannel_1(115);

    // Set data rate
    nRF24_SetDataRate_1(nRF24_DR_250kbps);

    // Set CRC scheme
    nRF24_SetCRCScheme_1(nRF24_CRC_2byte);

    // Set address width, its common for all pipes (RX and TX)
    nRF24_SetAddrWidth_1(3);

    // Configure RX PIPE#1
    static const uint8_t nRF24_ADDR_1[] = { 0xE7, 0x1C, 0xE3 };
    nRF24_SetAddr_1(nRF24_PIPE1, nRF24_ADDR_1); // program address for RX pipe #1
    nRF24_SetRXPipe_1(nRF24_PIPE1, nRF24_AA_OFF, 5); // Auto-ACK: disabled, payload length: 5 bytes

    // Set operational mode (PRX == receiver)
    nRF24_SetOperationalMode_1(nRF24_MODE_RX);

    // Wake the transceiver
    nRF24_SetPowerMode_1(nRF24_PWR_UP);

    // Put the transceiver to the RX mode
    nRF24_CE_H_1();
		
////////////////////////RX///////////////////////////////////


    // The main loop
    j = 0;
    payload_length = 5;
    while (1) {
    	// Prepare data packet
//    	for (i = 0; i < payload_length; i++) {
//    		nRF24_payload_tx[i] = 'T';
//    	}
			
//////////////////////////RX////////////////////////////////
		
		if (nRF24_GetStatus_RXFIFO_1() != nRF24_STATUS_RXFIFO_EMPTY) {
    		// Get a payload from the transceiver
    		pipe = nRF24_ReadPayload_1(nRF24_payload_rx, &payload_length);

    		// Clear all pending IRQ flags
			nRF24_ClearIRQFlags_1();

			// Print a payload contents to UART
			UART_SendStr("RCV PIPE#");
			UART_SendInt(pipe);
			UART_SendStr(" PAYLOAD:>");
			UART_SendBufHex((char *)nRF24_payload_rx, payload_length);
			UART_SendStr("<\r\n");
    	}
		
//////////////////////////RX////////////////////////////////

    	

    	// Transmit a packet
    	tx_res = nRF24_TransmitPacket(nRF24_payload_rx, payload_length);
			// Print a payload
    	UART_SendStr("PAYLOAD:>");	
    	UART_SendBufHex((char *)nRF24_payload_rx, payload_length);
    	UART_SendStr("< ... TX: ");
			
    	switch (tx_res) {
			case nRF24_TX_SUCCESS:
				UART_SendStr("OK");
				break;
			case nRF24_TX_TIMEOUT:
				UART_SendStr("TIMEOUT");
				break;
			case nRF24_TX_MAXRT:
				UART_SendStr("MAX RETRANSMIT");
				break;
			default:
				UART_SendStr("ERROR");
				break;
		}
    	UART_SendStr("\r\n");

    	// Wait ~0.5s
    	Delay_ms(500);
		

    }

#endif // DEMO_TX_RX_SINGLE
}
