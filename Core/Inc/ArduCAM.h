#ifndef _ARDUCAM_H
#define _ARDUCAM_H

#include <stdint.h>
#include "main.h"
#include "memorysaver.h"

// Register definitions
#define ARDUCHIP_TEST1       0x00
// ...

struct sensor_reg {
	uint16_t reg;
	uint8_t val;
};

#define ARDUCHIP_FRAMES      0x01
#define ARDUCHIP_MODE        0x02
#define ARDUCHIP_TIM         0x03
#define ARDUCHIP_TD          0x04
#define ARDUCHIP_GPIO        0x06
#define ARDUCHIP_REV         0x40
#define ARDUCHIP_TRIG        0x41
#define ARDUCHIP_FIFO        0x04
#define ARDUCHIP_GPIO_WRITE  0x44

#define FIFO_CLEAR_MASK      0x01
#define FIFO_START_MASK      0x02
#define FIFO_RDPTR_RST_MASK  0x10
#define FIFO_WRPTR_RST_MASK  0x20

#define ARDUCHIP_TRIG_SRC    0x44
#define VSYNC_MASK           0x01
#define SHUTTER_MASK         0x02
#define CAP_DONE_MASK        0x08

#define FIFO_SIZE1           0x42
#define FIFO_SIZE2           0x43
#define FIFO_SIZE3           0x44

// I2C Addresses (Shifted for HAL)
#define OV5642_ADDRESS       (0x3C << 1)

// Function Prototypes
void ArduCAM_Init(void);
void ArduCAM_WriteReg(uint8_t addr, uint8_t data);
uint8_t ArduCAM_ReadReg(uint8_t addr);
void ArduCAM_WriteSensorReg8_8(uint8_t regID, uint8_t regDat);
void ArduCAM_WriteSensorReg16_8(uint16_t regID, uint8_t regDat);
uint8_t ArduCAM_ReadSensorReg8_8(uint8_t regID);
uint8_t ArduCAM_ReadSensorReg16_8(uint16_t regID);

void ArduCAM_CS_LOW(void);
void ArduCAM_CS_HIGH(void);

void ArduCAM_FlushFIFO(void);
void ArduCAM_StartCapture(void);
void ArduCAM_ClearFIFOFlag(void);
uint32_t ArduCAM_ReadFIFO(void);
uint8_t ArduCAM_ReadFIFOByte(void);
void ArduCAM_SetFIFOBurst(void);
uint8_t ArduCAM_GetBit(uint8_t addr, uint8_t bit);
void ArduCAM_SetBit(uint8_t addr, uint8_t bit);
void ArduCAM_ClearBit(uint8_t addr, uint8_t bit);
void ArduCAM_SetMode(uint8_t mode);

// Check connections
uint8_t ArduCAM_CheckSPI(void);
uint8_t ArduCAM_CheckSensor(void);

// Resolution settings (Mock function for porting)
void ArduCAM_OV5642_Init_JPEG(void);

#endif /* _ARDUCAM_H */
