#include "ch32v00x.h"
#include <stdio.h>
#include <string.h>

// SPI Configuration for MCP3008
#define SPI_SCK_Pin GPIO_Pin_5  // PC5: SCK
#define SPI_CS_Pin GPIO_Pin_1   // PC1: CS
#define SPI_DIN_Pin GPIO_Pin_6  // PC6: DIN (MOSI)
#define SPI_DOUT_Pin GPIO_Pin_7 // PC7: DOUT (MISO)
#define MCP3008_CH0 0           // FSR402 channel
#define MCP3008_CH1 1           // LM35DZ channel

// I2C Configuration for MAX30102
#define I2C_SDA_Pin GPIO_Pin_4  // PC4: SDA
#define I2C_SCL_Pin GPIO_Pin_3  // PC3: SCL
#define MAX30102_ADDR 0x57      // I2C address (7-bit)
#define MAX30102_REG_FIFO_WR 0x04 // FIFO write pointer
#define MAX30102_REG_FIFO_RD 0x06 // FIFO read pointer
#define MAX30102_REG_FIFO_DATA 0x07 // FIFO data

// UART Configuration for FT232RL
#define UART_RX_Pin GPIO_Pin_0  // PC0: RX
#define UART_TX_Pin GPIO_Pin_2  // PC2: TX
#define BAUD_RATE 115200

// LED Configuration
#define LED_Pin GPIO_Pin_0      // PD0: LED
#define LED_PORT GPIOD

// Apnea Detection
#define APNEA_THRESHOLD 20      // 20 seconds no breathing
#define FSR_THRESHOLD 100       // FSR402 ADC value for breath detection

// Function Prototypes
void SPI_Setup(void);
uint16_t MCP3008_Read(uint8_t channel);
void I2C_Setup(void);
uint8_t I2C_ReadByte(uint8_t reg);
void I2C_WriteByte(uint8_t reg, uint8_t data);
void MAX30102_Setup(void);
void MAX30102_Read(uint8_t *spo2, uint16_t *hr);
void UART_Setup(void);
void UART_Send(const char *str);
void LED_Setup(void);
void SysTick_Setup(void);
void Custom_Delay_Ms(uint32_t ms);
uint32_t GetTickCount(void);

// Global Variables
volatile uint32_t sysTickCounter = 0;
uint32_t last_breath_time = 0;
uint8_t apnea_detected = 0;

int main(void) {
    // Initialize System
    SystemCoreClockUpdate();
    SysTick_Setup();

    // Initialize Peripherals
    SPI_Setup();
    I2C_Setup();
    MAX30102_Setup();
    UART_Setup();
    LED_Setup();

    // Variables for Sensor Data
    uint16_t fsr_value, temp_adc;
    uint8_t spo2;
    uint16_t heart_rate;
    char buffer[100];
    uint32_t current_time;

    while (1) {
        // Read FSR402 (Chest Movement) via MCP3008
        fsr_value = MCP3008_Read(MCP3008_CH0);
        if (fsr_value > FSR_THRESHOLD) {
            last_breath_time = GetTickCount(); // Update breath time
            apnea_detected = 0;
            GPIO_ResetBits(LED_PORT, LED_Pin); // LED off
        }

        // Check for Apnea
        current_time = GetTickCount();
        if ((current_time - last_breath_time) > (APNEA_THRESHOLD * 1000)) {
            apnea_detected = 1;
            GPIO_SetBits(LED_PORT, LED_Pin); // LED on
        }

        // Read LM35DZ (Temperature) via MCP3008
        temp_adc = MCP3008_Read(MCP3008_CH1);
        float temperature = (temp_adc * 3.3 / 1024.0) * 100.0; // Convert to °C

        // Read MAX30102 (SpO2, Heart Rate)
        MAX30102_Read(&spo2, &heart_rate);

        // Send Data via UART
        snprintf(buffer, sizeof(buffer),
                 "FSR: %u, Temp: %.1f C, SpO2: %u%%, HR: %u bpm, Apnea: %s\r\n",
                 fsr_value, temperature, spo2, heart_rate,
                 apnea_detected ? "Yes" : "No");
        UART_Send(buffer);

        // Delay for 1 second
        Custom_Delay_Ms(1000);
    }
}

// SPI Setup for MCP3008
void SPI_Setup(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    SPI_InitTypeDef SPI_InitStruct = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_SPI1, ENABLE);

    GPIO_InitStruct.GPIO_Pin = SPI_SCK_Pin | SPI_CS_Pin | SPI_DIN_Pin;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin = SPI_DOUT_Pin;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOC, &GPIO_InitStruct);

    SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
    SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_Init(SPI1, &SPI_InitStruct);

    SPI_Cmd(SPI1, ENABLE);
}

// Read MCP3008 ADC Channel
uint16_t MCP3008_Read(uint8_t channel) {
    uint8_t tx_data[3] = {0x01, (8 + channel) << 4, 0x00};
    uint8_t rx_data[3] = {0};
    uint16_t result;

    GPIO_ResetBits(GPIOC, SPI_CS_Pin); // CS low
    for (int i = 0; i < 3; i++) {
        while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
        SPI_I2S_SendData(SPI1, tx_data[i]);
        while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
        rx_data[i] = SPI_I2S_ReceiveData(SPI1);
    }
    GPIO_SetBits(GPIOC, SPI_CS_Pin); // CS high
    result = ((rx_data[1] & 0x03) << 8) | rx_data[2]; // 10-bit result
    return result;
}

// I2C Setup for MAX30102
void I2C_Setup(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    I2C_InitTypeDef I2C_InitStruct = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    GPIO_InitStruct.GPIO_Pin = I2C_SDA_Pin | I2C_SCL_Pin;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStruct);

    I2C_InitStruct.I2C_ClockSpeed = 100000;
    I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStruct.I2C_OwnAddress1 = 0x00;
    I2C_InitStruct.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(I2C1, &I2C_InitStruct);

    I2C_Cmd(I2C1, ENABLE);
}

// I2C Read Byte from MAX30102
uint8_t I2C_ReadByte(uint8_t reg) {
    I2C_GenerateSTART(I2C1, ENABLE);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
    I2C_Send7bitAddress(I2C1, MAX30102_ADDR << 1, I2C_Direction_Transmitter);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
    I2C_SendData(I2C1, reg);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    I2C_GenerateSTART(I2C1, ENABLE);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
    I2C_Send7bitAddress(I2C1, MAX30102_ADDR << 1, I2C_Direction_Receiver);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));
    I2C_AcknowledgeConfig(I2C1, DISABLE);
    I2C_GenerateSTOP(I2C1, ENABLE);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));
    uint8_t data = I2C_ReceiveData(I2C1);
    I2C_AcknowledgeConfig(I2C1, ENABLE);
    return data;
}

// I2C Write Byte to MAX30102
void I2C_WriteByte(uint8_t reg, uint8_t data) {
    I2C_GenerateSTART(I2C1, ENABLE);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
    I2C_Send7bitAddress(I2C1, MAX30102_ADDR << 1, I2C_Direction_Transmitter);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
    I2C_SendData(I2C1, reg);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    I2C_SendData(I2C1, data);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    I2C_GenerateSTOP(I2C1, ENABLE);
}

// MAX30102 Setup
void MAX30102_Setup(void) {
    I2C_WriteByte(0x09, 0x03); // MODE_CONFIG: SpO2 mode
    I2C_WriteByte(0x0A, 0x27); // SpO2_CONFIG: 100 sps, 411us, 16-bit ADC
    uint8_t id = I2C_ReadByte(0x00); // Read PART_ID
    char buf[20];
    snprintf(buf, sizeof(buf), "MAX30102 ID: 0x%02X\r\n", id);
    UART_Send(buf);
}

// Read MAX30102 (Basic SpO2/HR)
void MAX30102_Read(uint8_t *spo2, uint16_t *hr) {
    uint8_t fifo_data[6];
    uint8_t wr_ptr = I2C_ReadByte(MAX30102_REG_FIFO_WR);
    uint8_t rd_ptr = I2C_ReadByte(MAX30102_REG_FIFO_RD);
    if (wr_ptr != rd_ptr) {
        I2C_GenerateSTART(I2C1, ENABLE);
        while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
        I2C_Send7bitAddress(I2C1, MAX30102_ADDR << 1, I2C_Direction_Transmitter);
        while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
        I2C_SendData(I2C1, MAX30102_REG_FIFO_DATA);
        while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
        I2C_GenerateSTART(I2C1, ENABLE);
        while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
        I2C_Send7bitAddress(I2C1, MAX30102_ADDR << 1, I2C_Direction_Receiver);
        while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));
        for (int i = 0; i < 5; i++) {
            while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));
            fifo_data[i] = I2C_ReceiveData(I2C1);
        }
        I2C_AcknowledgeConfig(I2C1, DISABLE);
        I2C_GenerateSTOP(I2C1, ENABLE);
        while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));
        fifo_data[5] = I2C_ReceiveData(I2C1);
        I2C_AcknowledgeConfig(I2C1, ENABLE);

        uint32_t ir = (fifo_data[0] << 16) | (fifo_data[1] << 8) | fifo_data[2];
        uint32_t red = (fifo_data[3] << 16) | (fifo_data[4] << 8) | fifo_data[5];
        *spo2 = (ir > 0) ? 98 : 0; // Placeholder
        *hr = (red > 0) ? 70 : 0;  // Placeholder
    } else {
        *spo2 = 0;
        *hr = 0;
    }
}

// UART Setup for FT232RL
void UART_Setup(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    USART_InitTypeDef USART_InitStruct = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_USART1, ENABLE);

    GPIO_InitStruct.GPIO_Pin = UART_TX_Pin;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin = UART_RX_Pin;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOC, &GPIO_InitStruct);

    USART_InitStruct.USART_BaudRate = BAUD_RATE;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits = USART_StopBits_1;
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStruct);

    USART_Cmd(USART1, ENABLE);
}

// Send String via UART
void UART_Send(const char *str) {
    while (*str) {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, *str++);
    }
}

// LED Setup
void LED_Setup(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);

    GPIO_InitStruct.GPIO_Pin = LED_Pin;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_PORT, &GPIO_InitStruct);

    GPIO_ResetBits(LED_PORT, LED_Pin);
}

// SysTick Setup for Timing
void SysTick_Setup(void) {
    // Configure SysTick for 1ms ticks
    SysTick->CTLR = 0; // Disable SysTick
    SysTick->CNT = 0;  // Clear counter
    SysTick->CMP = SystemCoreClock / 1000 - 1; // 1ms period
    SysTick->CTLR = 0x0F; // Enable SysTick, interrupt, and clock source
    NVIC_EnableIRQ(SysTick_IRQn);
}

// SysTick Interrupt Handler
void SysTick_Handler(void) {
    sysTickCounter++;
    SysTick->SR = 0; // Clear interrupt flag
}

// Get System Tick
uint32_t GetTickCount(void) {
    return sysTickCounter;
}

// Delay Function
void Custom_Delay_Ms(uint32_t ms) {
    uint32_t start = GetTickCount();
    while (GetTickCount() - start < ms);
}// Main MCU code 
