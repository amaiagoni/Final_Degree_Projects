
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2020 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_hal.h"

/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include "signalProcessing.h"
#include "config.h"

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

RTC_HandleTypeDef hrtc;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart6;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
uint64_t prueba = 0;
/* GENERAL CONFIGURATIONS AND DATA STRUCTURES --------------------------------*/

#define NUMBER_OF_RMS 					12
#define NUMBER_OF_DATA_TO_SEND	(int)(SECS_BETWEEN_WIFI_DUMP/((FRAME_LENGTH*FRAMES_PER_DATA_UNIT/48000.0)))

struct Coordinates {
	float lat;
	float lon;
	float alt;
};
struct Data_Unit{
	float noise[NUMBER_OF_RMS];
	uint64_t millis;
	struct Coordinates *coordinates;
};

/* MICROPHONE ----------------------------------------------------------------*/

static uint32_t inBuffer[BUFFER_LENGTH];
struct Data_Unit outBuffer[NUMBER_OF_DATA_TO_SEND];
int rmsArrayPosition = 0;
 
/* WIFI MODULE ---------------------------------------------------------------*/

#define READ_REPLY_SIZE 				1
#define REPLY_BUFFER_SIZE 			1500
#define FULL_POST_SIZE 					433 + strlen(DATABASE_NAME) + strlen(SERVICE_IP) + strlen(SERVICE_PORT) + strlen(VEHICLE_ID)*2
#define WAIT_STATE 							10

volatile uint16_t replyIndex=0;

char *messageAT = "AT\r\n";
char *messageConnectWIFI = "AT+CWJAP=\"" WIFI_SSID "\",\"" WIFI_PASSWORD "\"\r\n";;
char *messageConnectService = "AT+CIPSTART=\"" SERVICE_PROTOCOL "\",\"" SERVICE_IP "\"," SERVICE_PORT "\r\n";
char *messageSetSendingMode = "AT+CIPMODE=1\r\n";
char *messageStartSend = "AT+CIPSEND\r\n";
char *messageEndOfSend = "+++";
char *messageExitSendingMode = "AT+CIPMODE=0\r\n";
char *messageCloseConnection = "AT+CIPCLOSE\r\n";
char *messageReset = "AT+RST\r\n";
char *fullPost;

char readReply, reply[REPLY_BUFFER_SIZE];

int status = WAIT_STATE;
int statusTimeout = 0;
int resendMessage = 0;
int dataSent = 0;

/* RTC -----------------------------------------------------------------------*/

RTC_TimeTypeDef sTime;
RTC_AlarmTypeDef sAlarm;
RTC_DateTypeDef sDate;

const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

struct tm currentTime;

volatile uint16_t secondsSinceStatusChange = 0;

/* GPS MODULE ----------------------------------------------------------------*/

#define READ_REPLY_SIZE_GPS 		1
#define REPLY_BUFFER_SIZE_GPS 	800
#define WAIT_STATE_GPS 					4
#define NUMBER_OF_GPS_VALUES		((NUMBER_OF_DATA_TO_SEND/NUMBER_OF_SECONDS_WITH_SAME_GPS) + (((int)NUMBER_OF_DATA_TO_SEND% (int)NUMBER_OF_SECONDS_WITH_SAME_GPS) != 0))

volatile uint16_t replyIndexGPS=0;

char *messagePowerGPS = "AT+CGPSPWR=1\r\n";
char *messageRetrieveGPS = "AT+CGPSINF=0\r\n";

char readReplyGPS, replyGPS[REPLY_BUFFER_SIZE_GPS], *replyGPSAux;

int statusGPS = WAIT_STATE_GPS;
int statusTimeoutGPS = 0;
int resendMessageGPS = 0;
volatile uint16_t secondsSinceStatusChangeGPS = 0;

int wordIndex = 0, auxGPSDataPosition = 0, lastPositionedData = 0;

int coordinateIndex = 0;

struct Coordinates allCoordinates[(int)NUMBER_OF_GPS_VALUES];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART6_UART_Init(void);
static void MX_RTC_Init(void);
static void MX_TIM2_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART1_UART_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* RTC -----------------------------------------------------------------------*/
void setRTC(void);
uint64_t currentTimeInMillis(void);

/* UART ----------------------------------------------------------------------*/
void emptyReply( char *reply, int replySize, volatile uint16_t *index );
void nextMessage(void);
void nextMessageGPS(void);

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

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
  MX_DMA_Init();
  MX_USART6_UART_Init();
  MX_RTC_Init();
  MX_TIM2_Init();
  MX_ADC1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
	
	/* Initialize timer and DMA for microphone data acquisition */
	
	for (int i = 0; i < BUFFER_LENGTH; i++) inBuffer[i] = 0;
	
	HAL_TIM_Base_Start(&htim2);
  HAL_ADC_Start_DMA(&hadc1,(uint32_t *) inBuffer, BUFFER_LENGTH);
	init_processing();
	
	/* Initialize WiFi module and related */
	
	setRTC();
	HAL_UART_Receive_IT(&huart6, (uint8_t *)&readReply, READ_REPLY_SIZE);
	emptyReply(reply, REPLY_BUFFER_SIZE, &replyIndex);
	
	/* Initialize GPS module and related */
	
	HAL_UART_Receive_IT(&huart1, (uint8_t *)&readReplyGPS, READ_REPLY_SIZE_GPS);
	emptyReply(replyGPS, REPLY_BUFFER_SIZE_GPS, &replyIndexGPS);
	for (int i = 0; i < (int)NUMBER_OF_GPS_VALUES; i++){
		allCoordinates[i].lon = 0;
		allCoordinates[i].lat = 0;
		allCoordinates[i].alt = 0;
	}
	
	/* Initialize data */
	for (int i = 0; i < SECS_BETWEEN_WIFI_DUMP; i++){
		for (int j = 0; j < NUMBER_OF_RMS; j++) outBuffer[i].noise[j] = 0;
		outBuffer[i].coordinates = &allCoordinates[0];
	}

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) 
  {

  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
		
	/* ADD MILLISECONDS TO DATA */
	
	if (outBuffer[rmsArrayPosition].millis == 0) outBuffer[rmsArrayPosition].millis = currentTimeInMillis();
		
	/* GPS MODULE */
		
	if (statusGPS == WAIT_STATE_GPS && lastPositionedData != rmsArrayPosition && rmsArrayPosition % NUMBER_OF_SECONDS_WITH_SAME_GPS == 0){
		statusGPS = 0; 
		statusTimeoutGPS = 0;
		secondsSinceStatusChangeGPS = secondsSinceStatusChange;
		lastPositionedData = rmsArrayPosition;
	}
	
	if(statusGPS != WAIT_STATE_GPS){
		switch (statusGPS) 
		{
			case 0:
				// AT Command to check SIM808 is working
				HAL_UART_Transmit(&huart1, (uint8_t *)messageAT, strlen(messageAT), MAX_SECS_BETWEEN_STATUS_GPS*1000);			
				nextMessageGPS();
				break;

			case 1:
				// Turn on the GPS power supply
				if (strstr(replyGPS, "\r\nOK\r\n") != NULL || resendMessageGPS == 1){
					emptyReply(replyGPS, REPLY_BUFFER_SIZE_GPS, &replyIndexGPS);
					HAL_UART_Transmit(&huart1, (uint8_t *)messagePowerGPS, strlen(messagePowerGPS), MAX_SECS_BETWEEN_STATUS_GPS*1000);
					nextMessageGPS();
				}
				break;
					
			case 2:
				// Ask for GPS Position
				if (strstr(replyGPS, "\r\nOK\r\n") != NULL || resendMessageGPS == 1){
					emptyReply(replyGPS, REPLY_BUFFER_SIZE_GPS, &replyIndexGPS);
					HAL_UART_Transmit(&huart1, (uint8_t *)messageRetrieveGPS, strlen(messageRetrieveGPS), MAX_SECS_BETWEEN_STATUS_GPS*1000);
					nextMessageGPS();
				}
				break;
				
			case 3:
				// Store obtained GPS Position in variables
				if (((strstr(replyGPS, "\r\nOK\r\n") != NULL) && (strstr(replyGPS, "+CGPSINF:") != NULL))|| resendMessageGPS == 1){
						replyGPSAux = strtok(replyGPS, ",");
						while(wordIndex < 3)
						{
							replyGPSAux = strtok(NULL, ",");
							wordIndex++;
							switch (wordIndex){
								case 1:
									allCoordinates[coordinateIndex].lon = atof(replyGPSAux)/100.0;
									break;
								case 2:
									allCoordinates[coordinateIndex].lat = atof(replyGPSAux)/100.0;
									break;
								case 3:
									allCoordinates[coordinateIndex].alt = atof(replyGPSAux);
									break;	
							}			
						}
						wordIndex = 0;
						if (++coordinateIndex == (int)NUMBER_OF_GPS_VALUES) coordinateIndex = 0;
						for (int i = 0; i < (int)NUMBER_OF_SECONDS_WITH_SAME_GPS; i++){
							auxGPSDataPosition = (rmsArrayPosition-1)- i;
							if (auxGPSDataPosition < 0) auxGPSDataPosition = (int)NUMBER_OF_DATA_TO_SEND + (rmsArrayPosition -1 - i);
							outBuffer[auxGPSDataPosition].coordinates = &allCoordinates[coordinateIndex];
						}
						emptyReply(replyGPS, REPLY_BUFFER_SIZE_GPS, &replyIndexGPS);
						statusTimeoutGPS = 0;
						nextMessageGPS();
					}
					break;
				
			default:
				// default statements
				break;
			}	
			if (replyIndexGPS < (REPLY_BUFFER_SIZE_GPS-1)){
				replyGPS[replyIndexGPS] = readReplyGPS;
			}		
			if ((secondsSinceStatusChange - secondsSinceStatusChangeGPS) > MAX_SECS_BETWEEN_STATUS_GPS){
				statusTimeoutGPS++;
				statusGPS--;
				resendMessageGPS = 1;
				emptyReply(replyGPS, REPLY_BUFFER_SIZE_GPS, &replyIndexGPS);
				secondsSinceStatusChangeGPS = secondsSinceStatusChange;
			}
			if (statusTimeoutGPS > (RETRY_NUMBER_GPS)){
				statusGPS = WAIT_STATE_GPS;		
			}
		}
	
	/* WIFI MODULE */
		
	if(status != WAIT_STATE){
		switch (status)
		{
			case 0:
				// AT Command to check ESP8266 is working
				HAL_UART_Transmit(&huart6, (uint8_t *)messageAT, strlen(messageAT), MAX_SECS_BETWEEN_STATUS*1000);
				nextMessage();
				break;

			case 1:
				// Connect to WiFi
				if (strstr(reply, "\r\nOK\r\n") != NULL || resendMessage == 1){
					emptyReply(reply, REPLY_BUFFER_SIZE, &replyIndex);
					HAL_UART_Transmit(&huart6, (uint8_t *)messageConnectWIFI, strlen(messageConnectWIFI), MAX_SECS_BETWEEN_STATUS*1000);
					nextMessage();
				}
				break;
					
			case 2:
				// Connect to service in which the database is running
				if (strstr(reply, "\r\nOK\r\n") != NULL || resendMessage == 1){
					emptyReply(reply, REPLY_BUFFER_SIZE, &replyIndex);
					HAL_UART_Transmit(&huart6, (uint8_t *)messageConnectService, strlen(messageConnectService), MAX_SECS_BETWEEN_STATUS*1000);
					nextMessage();
				}
				break;
				
			case 3:
				// Select sending mode in ESP8266
				if (strstr(reply, "\r\nOK\r\n") != NULL || resendMessage == 1){
					emptyReply(reply, REPLY_BUFFER_SIZE, &replyIndex);
					HAL_UART_Transmit(&huart6, (uint8_t *)messageSetSendingMode, strlen(messageSetSendingMode), MAX_SECS_BETWEEN_STATUS*1000);
					nextMessage();
				}
				break;
				
			case 4:
				// Command to ESP8266 the start of a message
				if (strstr(reply, "\r\nOK\r\n") != NULL || resendMessage == 1){
					emptyReply(reply, REPLY_BUFFER_SIZE, &replyIndex);
					HAL_UART_Transmit(&huart6, (uint8_t *)messageStartSend, strlen(messageStartSend), MAX_SECS_BETWEEN_STATUS*1000);
					nextMessage();
				}
				break;
				
			case 5:
				// Send POST message
				if (strstr(reply, "\r\nOK\r\n\r\n>") != NULL || resendMessage == 1){
					emptyReply(reply, REPLY_BUFFER_SIZE, &replyIndex);
					fullPost = malloc(FULL_POST_SIZE);
//					test = outBuffer[9].coordinates->alt;
					snprintf(fullPost, FULL_POST_SIZE, "POST /write?db=%s HTTP/1.1\r\nHost: %s:%s\r\nUser-Agent: curl/7.55.1\r\nAccept: */*\r\nContent-Length:298\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\nzaratamap_data,host=%s,key=%s-%llu Laeq=%07.3f,LaeqA=%07.3f,o63=%07.3f,o125=%07.3f,o250=%07.3f,o500=%07.3f,o1000=%07.3f,o2000=%07.3f,o4000=%07.3f,o8000=%07.3f,o16000=%07.3f,lon=%08.3f,lat=%08.3f,alt=%08.3f,aux_time=%llu,class=\"undefined\" %llu000000\r\n", 
										DATABASE_NAME, SERVICE_IP, SERVICE_PORT, VEHICLE_ID, VEHICLE_ID, outBuffer[dataSent].millis, outBuffer[dataSent].noise[0], outBuffer[dataSent].noise[11], outBuffer[dataSent].noise[2], outBuffer[dataSent].noise[3],
										outBuffer[dataSent].noise[4], outBuffer[dataSent].noise[5], outBuffer[dataSent].noise[6], outBuffer[dataSent].noise[7], outBuffer[dataSent].noise[8],
										outBuffer[dataSent].noise[9], outBuffer[dataSent].noise[10],
										outBuffer[dataSent].coordinates->lon, outBuffer[dataSent].coordinates->lat, outBuffer[dataSent].coordinates->alt,
										outBuffer[dataSent].millis, outBuffer[dataSent].millis);
					HAL_UART_Transmit(&huart6, (uint8_t *)fullPost, strlen(fullPost), MAX_SECS_BETWEEN_STATUS*1000);
					free(fullPost);
					outBuffer[dataSent].millis = 0;
					nextMessage();
				}
				break;
				
			case 6:
				// Command to ESP8266 the end of the message
				if ((strstr(reply, "GMT\r\n") != NULL && strstr(reply, "HTTP/1.1 204") != NULL) || (secondsSinceStatusChange > SECS_BETWEEN_DATA_MESSAGES) || resendMessage == 1){
					emptyReply(reply, REPLY_BUFFER_SIZE, &replyIndex);
					if (++dataSent == NUMBER_OF_DATA_TO_SEND){
						dataSent = 0;
						HAL_UART_Transmit(&huart6, (uint8_t *)messageEndOfSend, strlen(messageEndOfSend), MAX_SECS_BETWEEN_STATUS*1000);
						nextMessage();
					}else{
						status--;
						resendMessage = 1;
						emptyReply(reply, REPLY_BUFFER_SIZE, &replyIndex);
						secondsSinceStatusChange = 0;
					}
				}
				break;
			
			case 7:
				// Exit sending mode
				if ((secondsSinceStatusChange > SECS_FOR_NO_REPLY_MESSAGE) || resendMessage == 1){
					emptyReply(reply, REPLY_BUFFER_SIZE, &replyIndex);
					HAL_UART_Transmit(&huart6, (uint8_t *)messageExitSendingMode, strlen(messageExitSendingMode), MAX_SECS_BETWEEN_STATUS*1000);
					nextMessage();
				}
				break;
				
			case 8:
				// Close connection
				if (strstr(reply, "\r\nOK\r\n") != NULL || resendMessage == 1){
					emptyReply(reply, REPLY_BUFFER_SIZE, &replyIndex);
					HAL_UART_Transmit(&huart6, (uint8_t *)messageCloseConnection, strlen(messageCloseConnection), MAX_SECS_BETWEEN_STATUS*1000);
					nextMessage();
				}
				break;
				
			case 9:
				// Wait for close confirmation
				if (strstr(reply, "\r\nOK\r\n") != NULL  || resendMessage == 1){
					emptyReply(reply, REPLY_BUFFER_SIZE, &replyIndex);
					nextMessage();
				}
				break;

			case 404:
				// Close connection because of error
				emptyReply(reply, REPLY_BUFFER_SIZE, &replyIndex);
				HAL_UART_Transmit(&huart6, (uint8_t *)messageCloseConnection, strlen(messageCloseConnection), MAX_SECS_BETWEEN_STATUS*1000);
				nextMessage();
				break;
				
			case 405:
				// Reset ESP8266
				if (strstr(reply, "\r\nOK\r\n") != NULL || (secondsSinceStatusChange > SECS_FOR_NO_REPLY_MESSAGE) || resendMessage == 1){
					emptyReply(reply, REPLY_BUFFER_SIZE, &replyIndex);
					HAL_UART_Transmit(&huart6, (uint8_t *)messageReset, strlen(messageReset), MAX_SECS_BETWEEN_STATUS*1000);
					nextMessage();
				}
				break;
			
			case 406:
				// Wait for confirmation of reset and return to WAIT_STATE
				if (strstr(reply, "\r\nOK\r\n") != NULL || (secondsSinceStatusChange > SECS_FOR_NO_REPLY_MESSAGE) || resendMessage == 1){
					emptyReply(reply, REPLY_BUFFER_SIZE, &replyIndex);
					nextMessage();
					status = WAIT_STATE;
				}
				break;
			default:
				// default statements
				break;
		}

		if (replyIndex < (REPLY_BUFFER_SIZE-1)){
			reply[replyIndex] = readReply;
		} 

		if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == 0 || statusTimeout > (RETRY_NUMBER-1)){
			HAL_Delay(5);
			status = 404;		
		}
		
		if (secondsSinceStatusChange > MAX_SECS_BETWEEN_STATUS){
			statusTimeout++;
			status--;
			resendMessage = 1;
			emptyReply(reply, REPLY_BUFFER_SIZE, &replyIndex);
			secondsSinceStatusChange = 0;
		}
	}
		
	if (status == WAIT_STATE && statusGPS == WAIT_STATE_GPS && getFrameIndex() == 0 && secondsSinceStatusChange > SECS_BETWEEN_WIFI_DUMP){
		status = 0; 
		secondsSinceStatusChange = 0;
		emptyReply(reply, REPLY_BUFFER_SIZE, &replyIndex);
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

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

    /**Configure the main internal regulator output voltage 
    */
  __HAL_RCC_PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 84;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* ADC1 init function */
static void MX_ADC1_Init(void)
{

  ADC_ChannelConfTypeDef sConfig;

    /**Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion) 
    */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
  hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_TRGO;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
    */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* RTC init function */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime;
  RTC_DateTypeDef sDate;
  RTC_AlarmTypeDef sAlarm;

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

    /**Initialize RTC Only 
    */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initialize RTC and set the Time and Date 
    */
  sTime.Hours = 0;
  sTime.Minutes = 0;
  sTime.Seconds = 0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 1;
  sDate.Year = 0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Enable the Alarm A 
    */
  sAlarm.AlarmTime.Hours = 0;
  sAlarm.AlarmTime.Minutes = 0;
  sAlarm.AlarmTime.Seconds = 0;
  sAlarm.AlarmTime.SubSeconds = 0;
  sAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
  sAlarm.AlarmMask = RTC_ALARMMASK_NONE;
  sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
  sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
  sAlarm.AlarmDateWeekDay = 1;
  sAlarm.Alarm = RTC_ALARM_A;
  if (HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* TIM2 init function */
static void MX_TIM2_Init(void)
{

  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 1749;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* USART1 init function */
static void MX_USART1_UART_Init(void)
{

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
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* USART6 init function */
static void MX_USART6_UART_Init(void)
{

  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/** 
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void) 
{
  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

}

/** Configure pins as 
        * Analog 
        * Input 
        * Output
        * EVENT_OUT
        * EXTI
*/
static void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin : RESET_BUTTON_Pin */
  GPIO_InitStruct.Pin = RESET_BUTTON_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(RESET_BUTTON_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc){
	if (status == WAIT_STATE && statusGPS == WAIT_STATE_GPS) process(&inBuffer[FRAME_LENGTH],&outBuffer[rmsArrayPosition].noise[0],FRAME_LENGTH, &rmsArrayPosition);
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc){
	if (status == WAIT_STATE && statusGPS == WAIT_STATE_GPS) process(&inBuffer[0],&outBuffer[rmsArrayPosition].noise[0],FRAME_LENGTH, &rmsArrayPosition);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
    HAL_UART_Receive_IT(&huart6,(uint8_t *) &readReply, READ_REPLY_SIZE);
		HAL_UART_Receive_IT(&huart1,(uint8_t *) &readReplyGPS, READ_REPLY_SIZE_GPS);
}

void emptyReply( char *reply, int replySize, volatile uint16_t *index ) {
   for (int i = 0; i < replySize; i++) reply[i] = 0;
	 *index = 0;
}

void nextMessage(void){
	status++;
	secondsSinceStatusChange = 0;
	if (resendMessage == 0) statusTimeout = 0;
	else resendMessage = 0;
}

void nextMessageGPS(void){
	statusGPS++;
	if (resendMessageGPS == 0) statusTimeoutGPS = 0;
	else resendMessageGPS = 0;
	secondsSinceStatusChangeGPS = secondsSinceStatusChange;
}

void setRTC(void){
	// Set time 
	sTime.Hours = (__TIME__[0] - 0x30)*10 +  (__TIME__[1] - 0x30);
  sTime.Minutes = (__TIME__[3] - 0x30)*10 +  (__TIME__[4] - 0x30);
  sTime.Seconds = (__TIME__[6] - 0x30)*10 +  (__TIME__[7] - 0x30);
	HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	
	// Set date
	char month[4];
	strncpy(month, __DATE__, 3);
	month[3] = '\0';
	for (int i = 0; i < 12; i++){
		if (strcmp(months[i], month) == 0){
			if (i < 9) sDate.Month = ('0' - 0x30)*10 +  (i + 1 + '0' - 0x30);
			else sDate.Month = ('1' - 0x30)*10 +  (((i+1)%10) + '0' - 0x30);		
		}
	}
  sDate.Date = (isdigit(__DATE__[4]) ? (__DATE__[4] - 0x30)*10 : 0) +  (__DATE__[5] - 0x30);
  sDate.Year = (__DATE__[9] - 0x30)*10 + (__DATE__[10] - 0x30);
	HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
	
	// Set alarm to interrupt every second
	RTC_AlarmTypeDef sAlarm;
	HAL_RTC_GetAlarm(&hrtc, &sAlarm, RTC_ALARM_A, RTC_FORMAT_BIN);
	sAlarm.AlarmMask = RTC_ALARMMASK_ALL;
	sAlarm.Alarm = RTC_ALARM_A;
  if (HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }
}

uint64_t currentTimeInMillis(void){
	HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
	
	HAL_Delay(10);
	
	currentTime.tm_year = sDate.Year + 100;
	currentTime.tm_mon = sDate.Month - 1;
	currentTime.tm_mday = sDate.Date;
	currentTime.tm_hour = sTime.Hours;
	currentTime.tm_min = sTime.Minutes;
	currentTime.tm_sec = sTime.Seconds;
	currentTime.tm_isdst = -1;
	
	return  mktime(&currentTime)*1000ULL + sTime.SubSeconds;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
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
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
