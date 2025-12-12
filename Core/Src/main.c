/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ArduCAM.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2; // [NEW] SPI2 for ZYBO

UART_HandleTypeDef huart1; // [NEW] UART1 for Trigger
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
volatile uint8_t zybo_trigger = 0;
volatile uint8_t pc_save_request = 0; // [NEW] Button trigger for PC
uint8_t rx1_data; // Buffer for UART1 (ZYBO)
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void); // [NEW] UART1
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);        // [NEW] SPI2
/* USER CODE BEGIN PFP */
void ArduCAM_CaptureImage(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// 1. UART Receive Callback (ZYBO Trigger on UART1)
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    // Check if interrupt came from UART1 (ZYBO)
    if (huart->Instance == USART1)
    {
        if (rx1_data == 's' || rx1_data == 'S') {
            zybo_trigger = 1;
        }
        // Listen again
        HAL_UART_Receive_IT(&huart1, &rx1_data, 1);
    }
}

// 2. Button Interrupt (Manual Trigger for PC Snapshot)
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == B1_Pin)
    {
        pc_save_request = 1; // Button -> PC Save
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init(); // [NEW] Init UART1
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();        // [NEW] Init SPI2

  /* USER CODE BEGIN 2 */
  // Debug Message to PC (UART2)
  HAL_UART_Transmit(&huart2, (uint8_t*)"[System] Dual Stream Mode Ready.\r\n", 35, 100);
  HAL_UART_Transmit(&huart2, (uint8_t*)" - UART2: Video Stream (PC)\r\n", 30, 100);
  HAL_UART_Transmit(&huart2, (uint8_t*)" - UART1: Waiting for 's' (ZYBO)\r\n", 35, 100);

  // ArduCam Init
  ArduCAM_Init();
  
  // Check SPI
  HAL_UART_Transmit(&huart2, (uint8_t*)"[System] Checking SPI...\r\n", 26, 100);
  ArduCAM_WriteReg(0x00, 0x55);
  uint8_t temp = ArduCAM_ReadReg(0x00);
  if (temp == 0x55) {
      HAL_UART_Transmit(&huart2, (uint8_t*)"SPI OK\r\n", 8, 100);
      HAL_UART_Transmit(&huart2, (uint8_t*)"Reg Verify OK\r\n", 15, 100); // Satisfy viewer.py
  } else {
      HAL_UART_Transmit(&huart2, (uint8_t*)"SPI FAIL\r\n", 10, 100);
  }
  
  // Check Sensor
  uint8_t vid, pid;
  
  // OV5642 has 16-bit register addresses
  vid = ArduCAM_ReadSensorReg16_8(0x300a);
  pid = ArduCAM_ReadSensorReg16_8(0x300b);
  
  if (vid == 0x56 && pid == 0x42) {
      HAL_UART_Transmit(&huart2, (uint8_t*)"Sensor OK\r\n", 11, 100);
  } else {
      // Debug: print what we actually read
      char msg[30];
      sprintf(msg, "Sensor FAIL (ID: %02X%02X)\r\n", vid, pid);
      HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
  }

  HAL_UART_Transmit(&huart2, (uint8_t*)"Warming up Sensor\r\n", 19, 100);
  HAL_Delay(500);
  ArduCAM_OV5642_Init_JPEG();
  ArduCAM_SetMode(0); // 0 = MCU Loop Mode
  
  // Start Listening on UART1 (ZYBO)
  HAL_UART_Receive_IT(&huart1, &rx1_data, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      // 1. Capture Frame (Continuous)
      ArduCAM_CaptureImage();
      
      // Delay for stability
      HAL_Delay(100);
      
      // (Note: ArduCAM_CaptureImage was modified to send to both if triggers are set? 
      //  No, I need to modify ArduCAM_CaptureImage separately. 
      //  Actually, ArduCAM_CaptureImage in previous step had hardcoded UART2 transmit.
      //  I should update that function too. For now, let's look at the function definition below.)

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief ArduCAM_CaptureImage function
  * @retval None
  */
void ArduCAM_CaptureImage(void)
{
    ArduCAM_FlushFIFO();
    ArduCAM_ClearFIFOFlag();
    ArduCAM_StartCapture();
    
    // START TIMEOUT
    uint32_t start_time = HAL_GetTick();
    uint8_t capture_done = 0;
    while ((HAL_GetTick() - start_time) < 2000) { 
        if (ArduCAM_GetBit(ARDUCHIP_TRIG, CAP_DONE_MASK)) {
            capture_done = 1;
            break;
        }
    }
    if (!capture_done) {
        ArduCAM_ClearFIFOFlag();
        return;
    }
    
    uint32_t length = ArduCAM_ReadFIFO();
    if (length == 0 || length >= 0x7FFFFF) {
        ArduCAM_ClearFIFOFlag();
        return;
    }

    // [NEW] Check for PC Save Request (Button)
    if (pc_save_request) {
        // Send a magic string to PC so save_snapshot.py knows to save THIS frame
        HAL_UART_Transmit(&huart2, (uint8_t*)"SAVE_NOW", 8, 100);
        pc_save_request = 0; 
    }

    ArduCAM_WriteReg(ARDUCHIP_FIFO, FIFO_RDPTR_RST_MASK);
    
    // CS LOW (SPI1 for Camera)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    
    uint8_t cmd = 0x3C;
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    
    uint8_t buffer[512]; 
    uint32_t count = length;
    
    while (count > 0) {
        uint16_t bytes_to_read = (count > 512) ? 512 : count;
        
        // 1. Read from Camera (SPI1)
        HAL_SPI_Receive(&hspi1, buffer, bytes_to_read, 100);
        
        // 2. Send to PC (UART2) - Always
        HAL_UART_Transmit(&huart2, buffer, bytes_to_read, 1000);
        
        // 3. Send to ZYBO (SPI2) - Only if triggered
        if (zybo_trigger) {
             HAL_SPI_Transmit(&hspi2, buffer, bytes_to_read, 1000);
        }
        
        count -= bytes_to_read;
    }
    
    // CS HIGH
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
    
    ArduCAM_ClearFIFOFlag();
    
    // Clear trigger after FULL frame sent
    if (zybo_trigger) {
        zybo_trigger = 0;
    }
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{
  // [MANUAL MSP INIT] Because we are not regenerating via CubeMX
  __HAL_RCC_USART1_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /**USART1 GPIO Configuration
  PA9     ------> USART1_TX
  PA10     ------> USART1_RX
  */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // NVIC for UART1
  HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);

  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{
  // [MANUAL MSP INIT] 
  __HAL_RCC_SPI2_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /**SPI2 GPIO Configuration
  PB13     ------> SPI2_SCK
  PB15     ------> SPI2_MOSI
  */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4; // Fast speed
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
}


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16; 
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 921600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA4 (CS) */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
