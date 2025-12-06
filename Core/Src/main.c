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
#include <stdio.h>
#include "i2c-lcd.h"
#include "stm32f4xx_hal_exti.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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

TIM_HandleTypeDef htim1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t Temp_byte1, Temp_byte2;
uint16_t TEMP;
uint16_t Temperature = 0;
uint8_t Presence = 0;
uint8_t ConfigFlag = 0;
uint8_t ClearFlag = 0; // 0: Not Clear Yet , 1: Already Clear
int16_t DesiredTemp = 32;   // nhiệt độ mong muốn, mặc định 32°C
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM1_Init(void);
/* USER CODE BEGIN PFP */
void delay_us (uint16_t us);
void Set_Pin_Output (GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void Set_Pin_Input (GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
uint8_t DS18B20_Start (void);
void DS18B20_Write (uint8_t data);
uint8_t DS18B20_Read (void);
void Display_Temp (uint16_t Temp);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start(&htim1);
  HD44780_Init(2);
  HD44780_Clear();
  HD44780_SetCursor(0,0);
  HD44780_PrintStr("INITIALISING>>>>");
  HAL_Delay(2000);
  HD44780_Clear();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      if (ConfigFlag == 0) // Not in config mode
      {
          ClearFlag = 0;  // chuẩn bị cho lần vào config tiếp theo

          Presence = DS18B20_Start();
          DS18B20_Write(0xCC);  // skip ROM
          DS18B20_Write(0x44);  // convert t

          Presence = DS18B20_Start();
          DS18B20_Write(0xCC);  // skip ROM
          DS18B20_Write(0xBE);  // Read Scratch-pad

          Temp_byte1 = DS18B20_Read();
          Temp_byte2 = DS18B20_Read();
          TEMP = ((Temp_byte2 << 8))|Temp_byte1;
          Temperature = (float)TEMP/16.0;  // resolution is 0.0625

          HAL_Delay(500);
          Display_Temp(Temperature);

          // DÙNG NGƯỠNG DesiredTemp
          if (Temperature > DesiredTemp)
          {
              HAL_GPIO_WritePin(Relay_Signal_GPIO_Port, Relay_Signal_Pin, GPIO_PIN_RESET);
              HD44780_SetCursor(0,1);
              HD44780_PrintStr("Cooling Fan: ON ");
          }
          else
          {
              HAL_GPIO_WritePin(Relay_Signal_GPIO_Port, Relay_Signal_Pin, GPIO_PIN_SET);
              HD44780_SetCursor(0,1);
              HD44780_PrintStr("Cooling Fan: OFF");
          }
      }
      else   // CONFIG MODE
      {
          char buf[20];

          if (ClearFlag == 0)
          {
              HD44780_Clear();
              ClearFlag = 1;
          }

          // Dòng 1: tiêu đề
          HD44780_SetCursor(0,0);
          HD44780_PrintStr("Desired Temp");

          // Dòng 2: hiển thị giá trị đang set
          HD44780_SetCursor(0,1);
          sprintf(buf, "Set: %3d C   ", (int)DesiredTemp);
          HD44780_PrintStr(buf);

          // ======= XỬ LÝ NÚT K2: TĂNG =======
          if (HAL_GPIO_ReadPin(Button_K2_GPIO_Port, Button_K2_Pin) == GPIO_PIN_RESET)
          {
              if (DesiredTemp < 99)   // giới hạn trên, tuỳ bạn
                  DesiredTemp++;
              HAL_Delay(200);         // delay chống dội + tránh tăng quá nhanh
          }

          // ======= XỬ LÝ NÚT K3: GIẢM =======
          if (HAL_GPIO_ReadPin(Button_K3_GPIO_Port, Button_K3_Pin) == GPIO_PIN_RESET)
          {
              if (DesiredTemp > 0)    // giới hạn dưới
                  DesiredTemp--;
              HAL_Delay(200);
          }

          // ======= NÚT K4: XÁC NHẬN / THOÁT CONFIG =======
          if (HAL_GPIO_ReadPin(Button_K4_GPIO_Port, Button_K4_Pin) == GPIO_PIN_RESET)
          {
              ConfigFlag = 0;   // quay lại normal mode
              ClearFlag  = 0;   // để lần sau vào config sẽ clear lại
              HAL_Delay(200);
          }
      }
  }

  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 84-1;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 0xffff-1;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

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
  huart2.Init.BaudRate = 115200;
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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LD2_Pin|DS18B20_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Relay_Signal_GPIO_Port, Relay_Signal_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD2_Pin DS18B20_Pin */
  GPIO_InitStruct.Pin = LD2_Pin|DS18B20_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : Relay_Signal_Pin */
  GPIO_InitStruct.Pin = Relay_Signal_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(Relay_Signal_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Button_K1_Pin */
  GPIO_InitStruct.Pin = Button_K1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING ;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(Button_K1_GPIO_Port, &GPIO_InitStruct);
  /* Cấu hình NVIC cho EXTI0 */
   HAL_NVIC_SetPriority(EXTI15_10_IRQn , 2, 0);
   HAL_NVIC_EnableIRQ( EXTI15_10_IRQn );
  /*Configure GPIO pins : Button_K2_Pin Button_K4_Pin Button_K3_Pin */
  GPIO_InitStruct.Pin = Button_K2_Pin|Button_K4_Pin|Button_K3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void delay_us (uint16_t us)
{
	__HAL_TIM_SET_COUNTER(&htim1,0);  // set the counter value a 0
	while (__HAL_TIM_GET_COUNTER(&htim1) < us);  // wait for the counter to reach the us input in the parameter
}
void Set_Pin_Output (GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

void Set_Pin_Input (GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}
uint8_t DS18B20_Start (void)
{
	uint8_t Response = 0;
	Set_Pin_Output(DS18B20_GPIO_Port, DS18B20_Pin);   // set the pin as output
	HAL_GPIO_WritePin (DS18B20_GPIO_Port,DS18B20_Pin, 0);  // pull the pin low
	delay_us (480);   // delay according to datasheet
	Set_Pin_Input(DS18B20_GPIO_Port, DS18B20_Pin);    // set the pin as input
    delay_us (80);    // delay according to datasheet
    if (!(HAL_GPIO_ReadPin (DS18B20_GPIO_Port,  DS18B20_Pin)))
    	Response = 1;    // if the pin is low i.e the presence pulse is detected
    else
    	Response = -1;
    delay_us (400); // 480 us delay totally.
return Response;
}
void DS18B20_Write (uint8_t data)
{
	Set_Pin_Output(DS18B20_GPIO_Port, DS18B20_Pin);  // set as output

	for (int i=0; i<8; i++)
	{
		if ((data & (1<<i))!=0)  // if the bit is high
		{
			// write 1
			Set_Pin_Output(DS18B20_GPIO_Port, DS18B20_Pin);  // set as output
			HAL_GPIO_WritePin (DS18B20_GPIO_Port, DS18B20_Pin, 0);  // pull the pin LOW
			delay_us (1);  // wait for 1 us

			Set_Pin_Input(DS18B20_GPIO_Port, DS18B20_Pin);  // set as input
			delay_us (60);  // wait for 60 us
		}

		else  // if the bit is low
		{
			// write 0
			Set_Pin_Output(DS18B20_GPIO_Port, DS18B20_Pin);
			HAL_GPIO_WritePin (DS18B20_GPIO_Port, DS18B20_Pin, 0);  // pull the pin LOW
			delay_us (60);  // wait for 60 us

			Set_Pin_Input(DS18B20_GPIO_Port,DS18B20_Pin);
		}
	}
}
uint8_t DS18B20_Read (void)
{
	uint8_t value=0;
	Set_Pin_Input(DS18B20_GPIO_Port, DS18B20_Pin);  // set as input

	for (int i=0;i<8;i++)
	{
		Set_Pin_Output(DS18B20_GPIO_Port, DS18B20_Pin);  // set as output
		HAL_GPIO_WritePin (DS18B20_GPIO_Port, DS18B20_Pin, 0);  // pull the pin LOW
		delay_us (2);  // wait for 2 us
		Set_Pin_Input(DS18B20_GPIO_Port, DS18B20_Pin);  // set as input
		if (HAL_GPIO_ReadPin (DS18B20_GPIO_Port, DS18B20_Pin))  // if the pin is HIGH
		{
			value |= 1<<i;  // read = 1
		}
		delay_us (60);  // wait for 60 us
	}
	return value;
}
void Display_Temp (uint16_t Temp)
{
	char str[20] = {0};
    HD44780_SetCursor(0,0);
	sprintf(str, "TEMP: %d C   ", Temp);
	HD44780_PrintStr(str);
	//HD44780_PrintSpecialChar('C');
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_10)
  {
    // Việc bạn muốn làm khi nhấn nút
    // ví dụ: đổi trạng thái LED
   ConfigFlag = 1;
  }
}

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
#ifdef USE_FULL_ASSERT
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
