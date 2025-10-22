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
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "MLX90640_I2C_Driver.h"
#include "MLX90640_API.h"
#include <stdio.h>
#include <string.h>

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

/* USER CODE BEGIN PV */

#define SINGLE_CAPTURE_MODE ///$$$$$ SET MODE HERE $$$$$////
#define  FPS2HZ   0x02
#define  FPS4HZ   0x03
#define  FPS8HZ   0x04
#define  FPS16HZ  0x05
#define  FPS32HZ  0x06

#define  MLX90640_ADDR 0x33
#define	 RefreshRate FPS16HZ
#define  TA_SHIFT 8 //Default shift for MLX90640 in open air

static uint16_t eeData[MLX90640_EEPROM_DUMP_NUM]; //empty buffer for eePROM data
static float mlx90640To[768];
static uint16_t frameData[834]; //frame buffer
float emissivity=0.95;
int status;

//for push button capture
volatile uint8_t capture_requested = 0;
volatile uint8_t frames_to_capture = 0;
float captured_frames[3][768];  // 3 frames of 32x24

//push button debounce
uint32_t last_button_press_time = 0;
const uint32_t debounce_delay_ms = 50;  // debounce delay


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

void send_mlx90640_frame_binary_quantized(float *frame) {
	//fastest mlx clock rate that can be used = 16Hz
	//16Hz = 8fps => 32*24*16(2 bytes per pixel)*8 = 98304bps < 115200bps

    uint8_t start_bytes[2] = {0xAA, 0x55};
    uint8_t end_bytes[2]   = {0x55, 0xAA};

    int16_t temp_i16[768];
     for (int i = 0; i < 768; i++) {
         temp_i16[i] = (int16_t)(frame[i] * 100);  // e.g. 25.31°C → 2531
     }

    // Send start delimiter
    HAL_UART_Transmit(&huart2, start_bytes, 2, HAL_MAX_DELAY);

    // Send 768 floats = 3072 bytes
    HAL_UART_Transmit(&huart2, (uint8_t *)temp_i16, sizeof(temp_i16), HAL_MAX_DELAY);

    // Send end delimiter
    HAL_UART_Transmit(&huart2, end_bytes, 2, HAL_MAX_DELAY);
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) { //interrupt handler for push button press

    if (GPIO_Pin == GPIO_PIN_13) {  // PC13 = stm32 nucleo push button pin
      // HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);


    	uint32_t current_time = HAL_GetTick(); //get current time

    	//de-bounce make sure 50ms between button presses
    	if ((current_time - last_button_press_time) >= debounce_delay_ms) {

            capture_requested = 1;
            frames_to_capture = 3; //update with number of frames to capture

        	last_button_press_time = current_time;

    	}

    }
}



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
  /* USER CODE BEGIN 2 */

  HAL_Delay(2000);  // Let sensor power up


  	  MLX90640_SetRefreshRate(MLX90640_ADDR, RefreshRate); //set refresh rate
  	  MLX90640_SetChessMode(MLX90640_ADDR); //set pixel mode


  paramsMLX90640 mlx_params;  //initialize params struct


  int status = MLX90640_DumpEE(0x33, eeData); //dumps eeprom register values into eeData

  if (status == 0) {
      printf("EEPROM read successful\r\n");
  } else {
      printf("EEPROM read failed: %d\r\n", status);
  }

  status = MLX90640_ExtractParameters(eeData, &mlx_params); //extract calibration parameters from EERPOM read

  if (status == 0) {
      printf("Extract Params  successful\r\n");
      printf("Kvdd value %d\r\n",mlx_params.kVdd);


  } else {
      printf("Extract Params failed: %d\r\n", status);
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {

		int frame_status = MLX90640_GetFrameData(0x33, frameData); //Read raw frame data (1 or 2)

		#ifdef MLX90640_DEBUG
			printf("Page%d = [", page);
			for (int i = 0; i < 834; i++) {
			  printf("0x%x, ", FrameData[i]);
			}
			printf("]\r\n")
		#endif

		if (frame_status < 0) {
		  printf("Get frame data failed: %d\r\n", status);
		  continue;
		}
		float Ta = MLX90640_GetTa(frameData, &mlx_params); //get ambient temp value
		float tr = Ta - TA_SHIFT; //reflected temp based on sensor ambient temperature
		#ifdef MLX90640_DEBUG
			printf("vdd:  %f Tr: %f\r\n",vdd,tr);
		#endif
		MLX90640_CalculateTo(frameData, &mlx_params, emissivity , tr, mlx90640To);


		#ifdef SINGLE_CAPTURE_MODE //button press capture mode

		if (capture_requested && frames_to_capture > 0) {
		    // Assume frame is already read into 'frame'
		    memcpy(captured_frames[3 - frames_to_capture], mlx90640To, sizeof(float) * 768);
		    frames_to_capture--;

		    if (frames_to_capture == 0) {
		        capture_requested = 0;
		        // Send over 3 frames over UART
		        for (int f = 0; f < 3; f++) {
		        	send_mlx90640_frame_binary_quantized(captured_frames[f]);  // send frame over UART
		        	//toggle LED

		            HAL_Delay(10);
		        }
		        //Toggle Green LED when the frames are sent
		        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
		        HAL_Delay(1000);
		        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
		        //printf("Frame Sent\r\n");

		    }
		} //end of capture

		#endif

		#ifdef STREAM_MODE

		send_mlx90640_frame_binary_quantized(mlx90640To); //send frame using 2 bytes per pixel instead of 4

		#endif

		#ifdef MLX90640_DEBUG
			printf("<FRAME>\r\n");

			for(int i = 0; i < 768; i++){
				if(i%32 == 0 && i != 0){
					printf("\r\n");
				}
				printf("%2.2f ",mlx90640To[i]);
			}
			printf("\r\n");
			printf("</FRAME>\r\n");
		#endif

  	 }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    //end if statement

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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

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
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
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
