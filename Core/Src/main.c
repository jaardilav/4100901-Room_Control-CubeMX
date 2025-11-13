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
#include "led_driver.h"
#include "ring_buffer.h"
#include "keypad_driver.h"
#include "ring_buffer.h"
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
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

volatile uint16_t last_exti_pin = 0;

// --- INICIO: Código para el Teclado Matricial ---

// 1.1. Definición del manejador del teclado (keypad handle)
// Asegúrate que los nombres de los puertos y pines coincidan con los de tu main.h
keypad_handle_t keypad = {                 // Guarda los pines y puertos del teclado (4 filas y 4 columnas).
    .row_ports = {KEYPAD_R1_GPIO_Port, KEYPAD_R2_GPIO_Port, KEYPAD_R3_GPIO_Port, KEYPAD_R4_GPIO_Port},
    .row_pins = {KEYPAD_R1_Pin, KEYPAD_R2_Pin, KEYPAD_R3_Pin, KEYPAD_R4_Pin},
    .col_ports = {KEYPAD_C1_GPIO_Port, KEYPAD_C2_GPIO_Port, KEYPAD_C3_GPIO_Port, KEYPAD_C4_GPIO_Port},
    .col_pins = {KEYPAD_C1_Pin, KEYPAD_C2_Pin, KEYPAD_C3_Pin, KEYPAD_C4_Pin}
};

// 1.2. Buffer circular para almacenar las pulsaciones del teclado
#define KEYPAD_BUFFER_LEN 16
uint8_t keypad_buffer[KEYPAD_BUFFER_LEN];
ring_buffer_t keypad_rb;  //Buffer circular para almacenar las teclas presionadas.

// 1.3. Lógica para la clave
#define PASSWORD_LEN 4
const char PASSWORD[PASSWORD_LEN] = {'1', '0', '0', '7'}; // Clave secreta
char input_buffer[PASSWORD_LEN];
uint8_t input_index = 0;   //La clave correcta y el buffer donde se guardan las teclas ingresadas.

led_handle_t led1 = {      //El LED está conectado al pin PA5.
  .port = GPIOA, 
  .pin = GPIO_PIN_5 
};

#define UART2_RX_LEN 16
uint8_t uart2_rx_buffer[UART2_RX_LEN];
ring_buffer_t uart2_rx_rb;
uint8_t uart2_rx_data; // Buffers y variables para manejar la comunicación UART.
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)         //Cada vez que llega un dato por UART2:Lo vuelve a habilitar para seguir recibiendo y Lo guarda en el buffer circular.
{
    if (huart->Instance == USART2) {
      HAL_UART_Receive_IT(&huart2, &uart2_rx_data, 1);
      ring_buffer_write(&uart2_rx_rb, uart2_rx_data);
    }
}

/* Implementación para redirigir printf/gets al UART */
int __io_putchar(int ch)       //Envía un carácter al PC.
{
  uint8_t c = (uint8_t)ch;
  HAL_UART_Transmit(&huart2, &c, 1, HAL_MAX_DELAY);
  return ch;
}

int __io_getchar(void)        //Recibe un carácter del PC.
{
  uint8_t c = 0;
  if (HAL_UART_Receive(&huart2, &c, 1, HAL_MAX_DELAY) == HAL_OK) {
    return (int)c;
  }
  return -1;
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
  MX_USART2_UART_Init();       //Inicializa el sistema, los pines y la comunicación UART.
  /* USER CODE BEGIN 2 */
  led_init(&led1);

    // Inicialización del buffer circular para el teclado
  ring_buffer_init(&keypad_rb, keypad_buffer, KEYPAD_BUFFER_LEN);
  
  // Inicialización del driver del teclado (esto "arma" las interrupciones)
  keypad_init(&keypad);

  // Mensaje de bienvenida (requiere que printf esté redirigido a UART)
  printf("Sistema listo. Ingrese la clave de %d digitos...\r\n", PASSWORD_LEN);

  ring_buffer_init(&uart2_rx_rb, uart2_rx_buffer, UART2_RX_LEN);
  HAL_UART_Receive_IT(&huart2, &uart2_rx_data, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
/* USER CODE BEGIN WHILE */
while (1)
{
    // --- 1. Procesar interrupciones pendientes del teclado ---
    if (last_exti_pin != 0)
    {
        // Escanear la tecla correspondiente fuera de la interrupción
        char key = keypad_scan(&keypad, last_exti_pin);

        // Limpiar bandera para siguiente evento
        last_exti_pin = 0;

        // Si se detectó una tecla válida, almacenarla en el buffer circular
        if (key != '\0')
        {
            ring_buffer_write(&keypad_rb, (uint8_t)key);
        }

        // Rehabilitar interrupciones del teclado después de procesar
        HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
    }

    // --- 2. Verificar si hay teclas en el buffer para procesar ---
    uint8_t key_from_buffer;

    if (ring_buffer_read(&keypad_rb, &key_from_buffer))
    {
        printf("Tecla presionada: %c\r\n", (char)key_from_buffer);

        // 3. Almacenar la tecla en el buffer de entrada
        if (input_index < PASSWORD_LEN)
        {
            input_buffer[input_index] = (char)key_from_buffer;
            input_index++;
        }

        // 4. Si ya se ingresaron los 4 dígitos, verificar la clave
        if (input_index == PASSWORD_LEN)
        {
            if (strncmp(input_buffer, PASSWORD, PASSWORD_LEN) == 0)
            {
                // CLAVE CORRECTA
                printf("Clave CORRECTA. LED encendido.\r\n");
                led_on(&led1); // Encender el LED
            }
            else
            {
                // CLAVE INCORRECTA
                printf("Clave INCORRECTA. LED parpadea.\r\n");
                for (int i = 0; i < 3; i++)
                {
                    led_on(&led1);
                    HAL_Delay(150);
                    led_off(&led1);
                    HAL_Delay(150);
                }
            }

            // 5. Reiniciar el índice para la próxima clave
            input_index = 0;
            printf("\nIngrese la clave de %d dígitos...\r\n", PASSWORD_LEN);
        }
    }

    // --- 3. Aquí podrías agregar otras tareas no bloqueantes ---
}
/* USER CODE END WHILE */


    /* USER CODE BEGIN 3 */
  
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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
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
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();           //Si algo falla, se queda en un bucle infinito.
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
  HAL_GPIO_WritePin(GPIOA, LD2_Pin|LED_EXT_Pin|KEYPAD_R1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, KEYPAD_R2_Pin|KEYPAD_R4_Pin|KEYPAD_R3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD2_Pin LED_EXT_Pin KEYPAD_R1_Pin */
  GPIO_InitStruct.Pin = LD2_Pin|LED_EXT_Pin|KEYPAD_R1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : KEYPAD_C1_Pin */
  GPIO_InitStruct.Pin = KEYPAD_C1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(KEYPAD_C1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : KEYPAD_C4_Pin */
  GPIO_InitStruct.Pin = KEYPAD_C4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(KEYPAD_C4_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : KEYPAD_C2_Pin KEYPAD_C3_Pin */
  GPIO_InitStruct.Pin = KEYPAD_C2_Pin|KEYPAD_C3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : KEYPAD_R2_Pin KEYPAD_R4_Pin KEYPAD_R3_Pin */
  GPIO_InitStruct.Pin = KEYPAD_R2_Pin|KEYPAD_R4_Pin|KEYPAD_R3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */



void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // Solo registrar el pin que generó la interrupción
    HAL_NVIC_DisableIRQ(EXTI15_10_IRQn); 
    last_exti_pin = GPIO_Pin;
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
