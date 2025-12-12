#include "ArduCAM.h"
#include "main.h"
#include "ov5642_regs.h"
#include <stdio.h>
#include <string.h>

extern SPI_HandleTypeDef hspi1;
extern I2C_HandleTypeDef hi2c1;

// SPI Chip Select
void ArduCAM_CS_LOW(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
}

void ArduCAM_CS_HIGH(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
}

void ArduCAM_WriteReg(uint8_t addr, uint8_t data) {
    ArduCAM_CS_LOW();
    uint8_t buffer[2] = {addr | 0x80, data}; // Write bit 7 is 1 for ArduChip
    HAL_SPI_Transmit(&hspi1, buffer, 2, 100);
    ArduCAM_CS_HIGH();
}

uint8_t ArduCAM_ReadReg(uint8_t addr) {
    uint8_t data = 0;
    ArduCAM_CS_LOW();
    uint8_t tx_data = addr & 0x7F; // Read bit 7 is 0
    HAL_SPI_Transmit(&hspi1, &tx_data, 1, 100);
    HAL_SPI_Receive(&hspi1, &data, 1, 100);
    ArduCAM_CS_HIGH();
    return data;
}

void ArduCAM_WriteSensorReg8_8(uint8_t regID, uint8_t regDat) {
    uint8_t buffer[2] = {regID, regDat};
    HAL_I2C_Master_Transmit(&hi2c1, OV5642_ADDRESS, buffer, 2, 100);
}

void ArduCAM_WriteSensorReg16_8(uint16_t regID, uint8_t regDat) {
    uint8_t buffer[3] = {(regID >> 8) & 0xFF, regID & 0xFF, regDat};
    HAL_I2C_Master_Transmit(&hi2c1, OV5642_ADDRESS, buffer, 3, 100);
    HAL_Delay(2); // Critical Delay from Reference Implementation
}

uint8_t ArduCAM_ReadSensorReg8_8(uint8_t regID) {
    uint8_t data = 0;
    HAL_I2C_Master_Transmit(&hi2c1, OV5642_ADDRESS, &regID, 1, 100);
    HAL_I2C_Master_Receive(&hi2c1, OV5642_ADDRESS, &data, 1, 100);
    return data;
}

uint8_t ArduCAM_ReadSensorReg16_8(uint16_t regID) {
    uint8_t data = 0;
    uint8_t regAddr[2] = {(regID >> 8) & 0xFF, regID & 0xFF};
    HAL_I2C_Master_Transmit(&hi2c1, OV5642_ADDRESS, regAddr, 2, 100);
    HAL_I2C_Master_Receive(&hi2c1, OV5642_ADDRESS, &data, 1, 100);
    return data;
}

void ArduCAM_Init(void) {
    ArduCAM_CS_HIGH();
    // Reset or Init logic if needed
    ArduCAM_WriteReg(ARDUCHIP_TEST1, 0x55);
    
    // GPIO Init removed: Writing 0x00 or 0x01 caused Sensor FAIL (Reset assertion).
    // Using default High-Z state works for I2C. 
}

uint8_t ArduCAM_CheckSPI(void) {
    ArduCAM_WriteReg(ARDUCHIP_TEST1, 0x55);
    uint8_t temp = ArduCAM_ReadReg(ARDUCHIP_TEST1);
    if (temp != 0x55) return 0;
    
    ArduCAM_WriteReg(ARDUCHIP_TEST1, 0xAA);
    temp = ArduCAM_ReadReg(ARDUCHIP_TEST1);
    if (temp != 0xAA) return 0;
    
    return 1;
}

uint8_t ArduCAM_CheckSensor(void) {
    uint8_t vid, pid;
    // OV5642 IDs are high byte 0x56, low byte 0x42
    // Registers 0x300A, 0x300B usually
    vid = ArduCAM_ReadSensorReg16_8(0x300A);
    pid = ArduCAM_ReadSensorReg16_8(0x300B);
    if ((vid == 0x56) && (pid == 0x42)) return 1;
    return 0;
}

void ArduCAM_FlushFIFO(void) {
    ArduCAM_WriteReg(ARDUCHIP_FIFO, FIFO_CLEAR_MASK);
}

void ArduCAM_StartCapture(void) {
    ArduCAM_WriteReg(ARDUCHIP_FIFO, FIFO_START_MASK);
}

void ArduCAM_ClearFIFOFlag(void) {
    ArduCAM_WriteReg(ARDUCHIP_FIFO, FIFO_CLEAR_MASK);
}

uint32_t ArduCAM_ReadFIFO(void) {
    uint32_t len1, len2, len3;
    len1 = ArduCAM_ReadReg(FIFO_SIZE1);
    len2 = ArduCAM_ReadReg(FIFO_SIZE2);
    len3 = ArduCAM_ReadReg(FIFO_SIZE3) & 0x7F;
    return ((len3 << 16) | (len2 << 8) | len1) & 0x07FFFFF;
}

uint8_t ArduCAM_ReadFIFOByte(void) {
    uint8_t data = 0;
    ArduCAM_CS_LOW();
    uint8_t cmd = 0x3D; // Single Read Command
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    HAL_SPI_Receive(&hspi1, &data, 1, 100);
    ArduCAM_CS_HIGH();
    return data;
}

void ArduCAM_ReadFIFO_Burst(uint8_t* buffer, uint16_t length) {
    ArduCAM_CS_LOW();
    uint8_t cmd = 0x3C; // Burst Read Command
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    HAL_SPI_Receive(&hspi1, buffer, length, 1000);
    ArduCAM_CS_HIGH();  
}

uint8_t ArduCAM_GetBit(uint8_t addr, uint8_t bit) {
    uint8_t temp = ArduCAM_ReadReg(addr);
    return (temp & bit);
}

void ArduCAM_SetBit(uint8_t addr, uint8_t bit) {
    uint8_t temp = ArduCAM_ReadReg(addr);
    ArduCAM_WriteReg(addr, temp | bit);
}

void ArduCAM_ClearBit(uint8_t addr, uint8_t bit) {
    uint8_t temp = ArduCAM_ReadReg(addr);
    ArduCAM_WriteReg(addr, temp & (~bit));
}

void ArduCAM_SetFIFOBurst(void) {
    // Not strictly needed if using Single Read Byte, keeping empty for compatibility
}

void ArduCAM_SetMode(uint8_t mode) {
    ArduCAM_WriteReg(ARDUCHIP_MODE, mode);
}

void wrSensorRegs16_8(const struct sensor_reg reglist[]) {
    const struct sensor_reg *curr = reglist;
    uint16_t reg_addr;
    uint8_t reg_val;
    
    while (1) {
        reg_addr = curr->reg;
        reg_val = curr->val;
        if ((reg_addr == 0xffff) && (reg_val == 0xff)) {
            break;
        }
        ArduCAM_WriteSensorReg16_8(reg_addr, reg_val);
        HAL_Delay(5); // Increased delay to 5ms for stability
        curr++;
    }
}

void ArduCAM_OV5642_Init_JPEG(void) {
    // 1. Software Reset
    ArduCAM_WriteSensorReg16_8(0x3008, 0x80); 
    HAL_Delay(100);
    
    // 2. Load QVGA Preview (Base Setup) (320x240 YUV)
    wrSensorRegs16_8(OV5642_QVGA_Preview); // Uppercase 'OV' per new header
    HAL_Delay(100); 

    // 3. Load JPEG Capture Setup (QSXGA base)
    // This enables the JPEG engine blocks and sets PCLK=0x01
    // It also correctly enables AEC (0x3503=0x00)
    wrSensorRegs16_8(OV5642_JPEG_Capture_QSXGA);
    HAL_Delay(100);

    // 4. Load Resolution Specific Setup (320x240)
    wrSensorRegs16_8(ov5642_320x240);
    HAL_Delay(100);

    // 5. Manual Tweaks
    ArduCAM_WriteSensorReg16_8(0x3818, 0xa8); // Mirror/Bit settings (Matches camera project)
    ArduCAM_WriteSensorReg16_8(0x3621, 0x10); 
    ArduCAM_WriteSensorReg16_8(0x3801, 0xb0); 
    ArduCAM_WriteSensorReg16_8(0x4407, 0x04); 

    // [FIX TIMING] Slow down PCLK manually
    // New header sets 0x3824=0x01 (Fast). We want 0x11 (Slow) for stability.
    ArduCAM_WriteSensorReg16_8(0x3824, 0x11); 

    // 6. VSYNC Setup
    ArduCAM_WriteReg(ARDUCHIP_TIM, 0x02);
    HAL_Delay(50);
    
    // 7. Final Kickstart
    ArduCAM_WriteSensorReg16_8(0x3002, 0x0C); 
    HAL_Delay(10);
    ArduCAM_WriteSensorReg16_8(0x3002, 0x00); 
    HAL_Delay(100);
}


