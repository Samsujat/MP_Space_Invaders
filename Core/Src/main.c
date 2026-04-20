/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 *
 *                             Bonjour ! J'aime beaucoup l'informatique industrielle
 *
 *                             Licorne magique
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "lwip.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_ts.h"
#include "stdio.h"
#include "semphr.h"
#include "images.h"
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

/* ============================================================================
 * HANDLES PERIPHERIQUES
 * ============================================================================ */
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc3;

CRC_HandleTypeDef hcrc;

DAC_HandleTypeDef hdac;

DMA2D_HandleTypeDef hdma2d;

LTDC_HandleTypeDef hltdc;

RNG_HandleTypeDef hrng;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim5;
TIM_HandleTypeDef htim8;

SDRAM_HandleTypeDef hsdram1;

/* ============================================================================
 * HANDLES FREERTOS — TACHES, FILES, MUTEX
 * ============================================================================ */

osThreadId GameMasterHandle;
osThreadId Joueur_1Handle;
osThreadId Block_EnemieHandle;
osThreadId ProjectileHandle;
osThreadId HUDHandle;
osThreadId chargeurHandle;
osMessageQId Queue_FHandle;
osMessageQId Queue_NHandle;
osMessageQId Queue_JHandle;
osMessageQId Queue_EHandle;
osMutexId MutexLCDHandle;
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

/* ============================================================================
 * PROTOTYPAGE DES FONCTIONS
 * ============================================================================ */

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC3_Init(void);
static void MX_LTDC_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM5_Init(void);
static void MX_TIM8_Init(void);
static void MX_DAC_Init(void);
static void MX_FMC_Init(void);
static void MX_DMA2D_Init(void);
static void MX_CRC_Init(void);
static void MX_RNG_Init(void);
static void MX_ADC1_Init(void);
void f_GameMaster(void const * argument);
void f_Joueur_1(void const * argument);
void f_block_enemie(void const * argument);
void f_projectile(void const * argument);
void f_HUD(void const * argument);
void f_chargeur(void const * argument);
void f_titre(void const * argument);

/* USER CODE BEGIN PFP */
uint8_t proba_bernoulli(uint32_t numerateur, uint32_t denominateur);
uint8_t proba_tirrage(uint8_t nombre_valeur);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* ============================================================================
 * TYPES, ENUMERATIONS ET STRUCTURES DU JEU
 * ============================================================================ */

enum Game_State
{
  TITLE_SCREEN,
  GAME_RUNNING
};

volatile enum Game_State game_state = TITLE_SCREEN;

enum Camps_missile
{
  MISSILE_AMI,
  MISSILE_ENNEMI
};

enum End_type
{
  END_TABLEAU_VIDE,
  END_MORT_JOUEUR
};

enum Sens_ennemie
{
  DROITE,
  GAUCHE
};

/* ============================================================================
 * CONSTANTES ET VARIABLES GLOBALES DU JEU
 * ============================================================================ */

const uint16_t joueur_width = 15;
const uint16_t joueur_height = 25;

const uint16_t monstre_width = 20;
const uint16_t monstre_height = 15;

const int8_t VIE_MAX = 5;
struct Missile
{
  int16_t x;
  int16_t y;
  int8_t dx;
  int8_t dy;
  enum Camps_missile equipe;
  uint8_t damage;
  uint8_t valide;
};

struct Joueur
{
  // uint32_t et pas 16 car fonction d'affichage bitmap (j'en sais pas plus)
  int32_t x;              // Position de l'angle superieur gauche
  int32_t y;              // Position de l'angle superieur gauche
  int8_t dx;              // Vitesse du joueur
  int8_t dy;              // Vitesse du joueur
  int8_t health;          // Vie du joueur
  struct Missile missile; // Missile lancé par le joueur
};


struct Monster
{
  int32_t x;
  int32_t y;
  int16_t health;
  // uint8_t type; // TODO d'autre ennemies ?
  struct Missile missile;
  uint8_t*  pbmp;
};

struct Collision
{
  uint8_t idx1;
  uint8_t idx2;
  uint8_t damage;
};

struct Led
{
  GPIO_TypeDef* port;
  uint16_t pin;
};

struct Led Leds[] = {
    {LED18_GPIO_Port, LED18_Pin},
    {LED17_GPIO_Port, LED17_Pin},
    {LED16_GPIO_Port, LED16_Pin},
    {LED15_GPIO_Port, LED15_Pin},
    {LED14_GPIO_Port, LED14_Pin},
    {LED13_GPIO_Port, LED13_Pin},
    {LED12_GPIO_Port, LED12_Pin},
    {LED11_GPIO_Port, LED11_Pin}};

// Définition des paramètres du joueurs

struct Joueur joueur = {200, 200, 1, 1, VIE_MAX, {0, 0, 0, -1, MISSILE_AMI, 1, 1}};

uint8_t LED = 1;

uint32_t LCD_COLOR_BACKGROUND = LCD_COLOR_BLACK;

// Number of waves of enemies before the game is won.
uint8_t wave = 0;
uint8_t kill = 0;
uint8_t charge = 0;

// Tableau des monstres (8 par ligne, sur 3 ligne)
struct Monster Table_ennemis[8][3];

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */

/* ============================================================================
 * FONCTION PRINCIPALE
 * ============================================================================ */

int main(void)
{
  /* USER CODE BEGIN 1 */
  static TS_StateTypeDef TS_State;
  ADC_ChannelConfTypeDef sConfig = {0};
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;

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
  MX_ADC3_Init();
  MX_LTDC_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM5_Init();
  MX_TIM8_Init();
  MX_DAC_Init();
  MX_FMC_Init();
  MX_DMA2D_Init();
  MX_CRC_Init();
  MX_RNG_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  BSP_LCD_Init();
  BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
  BSP_LCD_LayerDefaultInit(1,
                           LCD_FB_START_ADDRESS + BSP_LCD_GetXSize() * BSP_LCD_GetYSize() * 4);
  BSP_LCD_DisplayOn();
  BSP_LCD_SelectLayer(1);
  BSP_LCD_Clear(LCD_COLOR_BLACK);
  BSP_LCD_SetFont(&Font12);
  BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
  BSP_LCD_SetBackColor(LCD_COLOR_BLACK);

  BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());

  /* USER CODE END 2 */

  /* Create the mutex(es) */
  /* definition and creation of MutexLCD */
  osMutexDef(MutexLCD);
  MutexLCDHandle = osMutexCreate(osMutex(MutexLCD));

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* definition and creation of Queue_F */
  osMessageQDef(Queue_F, 1, enum End_type);
  Queue_FHandle = osMessageCreate(osMessageQ(Queue_F), NULL);

  /* definition and creation of Queue_N */
  osMessageQDef(Queue_N, 16, struct Missile);
  Queue_NHandle = osMessageCreate(osMessageQ(Queue_N), NULL);

  /* definition and creation of Queue_J */
  osMessageQDef(Queue_J, 3, uint8_t);
  Queue_JHandle = osMessageCreate(osMessageQ(Queue_J), NULL);

  /* definition and creation of Queue_E */
  osMessageQDef(Queue_E, 8, struct Collision);
  Queue_EHandle = osMessageCreate(osMessageQ(Queue_E), NULL);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of GameMaster */
  osThreadDef(GameMaster, f_GameMaster, osPriorityHigh, 0, 1024);
  GameMasterHandle = osThreadCreate(osThread(GameMaster), NULL);

  /* definition and creation of Joueur_1 */
  osThreadDef(Joueur_1, f_Joueur_1, osPriorityAboveNormal, 0, 1024);
  Joueur_1Handle = osThreadCreate(osThread(Joueur_1), NULL);

  /* definition and creation of Block_Enemie */
  osThreadDef(Block_Enemie, f_block_enemie, osPriorityLow, 0, 1024);
  Block_EnemieHandle = osThreadCreate(osThread(Block_Enemie), NULL);

  /* definition and creation of Projectile */
  osThreadDef(Projectile, f_projectile, osPriorityNormal, 0, 1024);
  ProjectileHandle = osThreadCreate(osThread(Projectile), NULL);

  /* definition and creation of HUD */
  osThreadDef(HUD, f_HUD, osPriorityBelowNormal, 0, 1024);
  HUDHandle = osThreadCreate(osThread(HUD), NULL);

  /* definition and creation of chargeur */
  osThreadDef(chargeur, f_chargeur, osPriorityBelowNormal, 0, 128);
  chargeurHandle = osThreadCreate(osThread(chargeur), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  vQueueAddToRegistry(Queue_NHandle, "Queue Missile");
  vQueueAddToRegistry(Queue_JHandle, "Queue Joueur");
  vQueueAddToRegistry(Queue_EHandle, "Queue Ennemie");
  vQueueAddToRegistry(Queue_FHandle, "Queue Fin");
  osThreadId TitreHandle;

  osThreadDef(Titre, f_titre, osPriorityNormal, 0, 512);
  TitreHandle = osThreadCreate(osThread(Titre), NULL);
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* Code de base */
    //    HAL_GPIO_WritePin(LED13_GPIO_Port, LED13_Pin,
    //                      HAL_GPIO_ReadPin(BP1_GPIO_Port, BP1_Pin));
    //    HAL_GPIO_WritePin(LED14_GPIO_Port, LED14_Pin,
    //                      HAL_GPIO_ReadPin(BP2_GPIO_Port, BP2_Pin));
    //    sprintf(text, "BP1 : %d", HAL_GPIO_ReadPin(BP1_GPIO_Port, BP1_Pin));
    //    BSP_LCD_DisplayStringAtLine(5, (uint8_t *)text);

    ;

    sConfig.Channel = ADC_CHANNEL_7;
    HAL_ADC_ConfigChannel(&hadc3, &sConfig);
    HAL_ADC_Start(&hadc3);

    sConfig.Channel = ADC_CHANNEL_6;
    HAL_ADC_ConfigChannel(&hadc3, &sConfig);
    HAL_ADC_Start(&hadc3);
    sConfig.Channel = ADC_CHANNEL_8;
    HAL_ADC_ConfigChannel(&hadc3, &sConfig);
    HAL_ADC_Start(&hadc3);

    HAL_ADC_Start(&hadc1);

    BSP_TS_GetState(&TS_State);
    if (TS_State.touchDetected)
    {
      BSP_LCD_FillCircle(TS_State.touchX[0], TS_State.touchY[0], 4);
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 400;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC|RCC_PERIPHCLK_CLK48;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 384;
  PeriphClkInitStruct.PLLSAI.PLLSAIR = 5;
  PeriphClkInitStruct.PLLSAI.PLLSAIQ = 2;
  PeriphClkInitStruct.PLLSAI.PLLSAIP = RCC_PLLSAIP_DIV8;
  PeriphClkInitStruct.PLLSAIDivQ = 1;
  PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_8;
  PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48SOURCE_PLLSAIP;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */
  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief ADC3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC3_Init(void)
{

  /* USER CODE BEGIN ADC3_Init 0 */

  /* USER CODE END ADC3_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC3_Init 1 */

  /* USER CODE END ADC3_Init 1 */
  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc3.Instance = ADC3;
  hadc3.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc3.Init.Resolution = ADC_RESOLUTION_12B;
  hadc3.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc3.Init.ContinuousConvMode = DISABLE;
  hadc3.Init.DiscontinuousConvMode = DISABLE;
  hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc3.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc3.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc3.Init.NbrOfConversion = 1;
  hadc3.Init.DMAContinuousRequests = DISABLE;
  hadc3.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc3) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_8;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC3_Init 2 */

  /* USER CODE END ADC3_Init 2 */

}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
  hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
  hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief DAC Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC_Init(void)
{

  /* USER CODE BEGIN DAC_Init 0 */

  /* USER CODE END DAC_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC_Init 1 */

  /* USER CODE END DAC_Init 1 */
  /** DAC Initialization
  */
  hdac.Instance = DAC;
  if (HAL_DAC_Init(&hdac) != HAL_OK)
  {
    Error_Handler();
  }
  /** DAC channel OUT1 config
  */
  sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  if (HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC_Init 2 */

  /* USER CODE END DAC_Init 2 */

}

/**
  * @brief DMA2D Initialization Function
  * @param None
  * @retval None
  */
static void MX_DMA2D_Init(void)
{

  /* USER CODE BEGIN DMA2D_Init 0 */

  /* USER CODE END DMA2D_Init 0 */

  /* USER CODE BEGIN DMA2D_Init 1 */

  /* USER CODE END DMA2D_Init 1 */
  hdma2d.Instance = DMA2D;
  hdma2d.Init.Mode = DMA2D_M2M;
  hdma2d.Init.ColorMode = DMA2D_OUTPUT_ARGB8888;
  hdma2d.Init.OutputOffset = 0;
  hdma2d.LayerCfg[1].InputOffset = 0;
  hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_ARGB8888;
  hdma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
  hdma2d.LayerCfg[1].InputAlpha = 0;
  if (HAL_DMA2D_Init(&hdma2d) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DMA2D_ConfigLayer(&hdma2d, 1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DMA2D_Init 2 */

  /* USER CODE END DMA2D_Init 2 */

}

/**
  * @brief LTDC Initialization Function
  * @param None
  * @retval None
  */
static void MX_LTDC_Init(void)
{

  /* USER CODE BEGIN LTDC_Init 0 */

  /* USER CODE END LTDC_Init 0 */

  LTDC_LayerCfgTypeDef pLayerCfg = {0};

  /* USER CODE BEGIN LTDC_Init 1 */

  /* USER CODE END LTDC_Init 1 */
  hltdc.Instance = LTDC;
  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
  hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
  hltdc.Init.HorizontalSync = 40;
  hltdc.Init.VerticalSync = 9;
  hltdc.Init.AccumulatedHBP = 53;
  hltdc.Init.AccumulatedVBP = 11;
  hltdc.Init.AccumulatedActiveW = 533;
  hltdc.Init.AccumulatedActiveH = 283;
  hltdc.Init.TotalWidth = 565;
  hltdc.Init.TotalHeigh = 285;
  hltdc.Init.Backcolor.Blue = 0;
  hltdc.Init.Backcolor.Green = 0;
  hltdc.Init.Backcolor.Red = 0;
  if (HAL_LTDC_Init(&hltdc) != HAL_OK)
  {
    Error_Handler();
  }
  pLayerCfg.WindowX0 = 0;
  pLayerCfg.WindowX1 = 480;
  pLayerCfg.WindowY0 = 0;
  pLayerCfg.WindowY1 = 272;
  pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
  pLayerCfg.Alpha = 255;
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  pLayerCfg.FBStartAdress = 0xC0000000;
  pLayerCfg.ImageWidth = 480;
  pLayerCfg.ImageHeight = 272;
  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;
  if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LTDC_Init 2 */

  /* USER CODE END LTDC_Init 2 */

}

/**
  * @brief RNG Initialization Function
  * @param None
  * @retval None
  */
static void MX_RNG_Init(void)
{

  /* USER CODE BEGIN RNG_Init 0 */

  /* USER CODE END RNG_Init 0 */

  /* USER CODE BEGIN RNG_Init 1 */

  /* USER CODE END RNG_Init 1 */
  hrng.Instance = RNG;
  if (HAL_RNG_Init(&hrng) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RNG_Init 2 */

  /* USER CODE END RNG_Init 2 */

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
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
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
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_SlaveConfigTypeDef sSlaveConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sSlaveConfig.SlaveMode = TIM_SLAVEMODE_DISABLE;
  sSlaveConfig.InputTrigger = TIM_TS_ITR0;
  if (HAL_TIM_SlaveConfigSynchro(&htim3, &sSlaveConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief TIM5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM5_Init(void)
{

  /* USER CODE BEGIN TIM5_Init 0 */

  /* USER CODE END TIM5_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  htim5.Instance = TIM5;
  htim5.Init.Prescaler = 0;
  htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim5.Init.Period = 4294967295;
  htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim5, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM5_Init 2 */

  /* USER CODE END TIM5_Init 2 */

}

/**
  * @brief TIM8 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM8_Init(void)
{

  /* USER CODE BEGIN TIM8_Init 0 */

  /* USER CODE END TIM8_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM8_Init 1 */

  /* USER CODE END TIM8_Init 1 */
  htim8.Instance = TIM8;
  htim8.Init.Prescaler = 0;
  htim8.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim8.Init.Period = 65535;
  htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim8.Init.RepetitionCounter = 0;
  htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim8) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim8, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim8) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim8, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim8, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
  sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
  sBreakDeadTimeConfig.Break2Filter = 0;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim8, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM8_Init 2 */

  /* USER CODE END TIM8_Init 2 */
  HAL_TIM_MspPostInit(&htim8);

}

/* FMC initialization function */
static void MX_FMC_Init(void)
{

  /* USER CODE BEGIN FMC_Init 0 */

  /* USER CODE END FMC_Init 0 */

  FMC_SDRAM_TimingTypeDef SdramTiming = {0};

  /* USER CODE BEGIN FMC_Init 1 */

  /* USER CODE END FMC_Init 1 */

  /** Perform the SDRAM1 memory initialization sequence
  */
  hsdram1.Instance = FMC_SDRAM_DEVICE;
  /* hsdram1.Init */
  hsdram1.Init.SDBank = FMC_SDRAM_BANK1;
  hsdram1.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_8;
  hsdram1.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_12;
  hsdram1.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_16;
  hsdram1.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
  hsdram1.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_1;
  hsdram1.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
  hsdram1.Init.SDClockPeriod = FMC_SDRAM_CLOCK_DISABLE;
  hsdram1.Init.ReadBurst = FMC_SDRAM_RBURST_DISABLE;
  hsdram1.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_0;
  /* SdramTiming */
  SdramTiming.LoadToActiveDelay = 16;
  SdramTiming.ExitSelfRefreshDelay = 16;
  SdramTiming.SelfRefreshTime = 16;
  SdramTiming.RowCycleDelay = 16;
  SdramTiming.WriteRecoveryTime = 16;
  SdramTiming.RPDelay = 16;
  SdramTiming.RCDDelay = 16;

  if (HAL_SDRAM_Init(&hsdram1, &SdramTiming) != HAL_OK)
  {
    Error_Handler( );
  }

  /* USER CODE BEGIN FMC_Init 2 */

  /* USER CODE END FMC_Init 2 */
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOJ_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();
  __HAL_RCC_GPIOK_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, LED14_Pin|LED15_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(OTG_FS_PowerSwitchOn_GPIO_Port, OTG_FS_PowerSwitchOn_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED16_GPIO_Port, LED16_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_Port, LCD_BL_CTRL_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_DISP_GPIO_Port, LCD_DISP_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOH, LED13_Pin|LED17_Pin|LED11_Pin|LED12_Pin
                          |LED2_Pin|LED18_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(EXT_RST_GPIO_Port, EXT_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : PE3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : ARDUINO_SCL_D15_Pin ARDUINO_SDA_D14_Pin */
  GPIO_InitStruct.Pin = ARDUINO_SCL_D15_Pin|ARDUINO_SDA_D14_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : ULPI_D7_Pin ULPI_D6_Pin ULPI_D5_Pin ULPI_D2_Pin
                           ULPI_D1_Pin ULPI_D4_Pin */
  GPIO_InitStruct.Pin = ULPI_D7_Pin|ULPI_D6_Pin|ULPI_D5_Pin|ULPI_D2_Pin
                          |ULPI_D1_Pin|ULPI_D4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_HS;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : BP2_Pin BP1_Pin */
  GPIO_InitStruct.Pin = BP2_Pin|BP1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LED14_Pin LED15_Pin */
  GPIO_InitStruct.Pin = LED14_Pin|LED15_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : VCP_RX_Pin */
  GPIO_InitStruct.Pin = VCP_RX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(VCP_RX_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_VBUS_Pin */
  GPIO_InitStruct.Pin = OTG_FS_VBUS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OTG_FS_VBUS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Audio_INT_Pin */
  GPIO_InitStruct.Pin = Audio_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Audio_INT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : OTG_FS_PowerSwitchOn_Pin LED16_Pin */
  GPIO_InitStruct.Pin = OTG_FS_PowerSwitchOn_Pin|LED16_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : LED3_Pin LCD_DISP_Pin */
  GPIO_InitStruct.Pin = LED3_Pin|LCD_DISP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

  /*Configure GPIO pin : uSD_Detect_Pin */
  GPIO_InitStruct.Pin = uSD_Detect_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(uSD_Detect_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_BL_CTRL_Pin */
  GPIO_InitStruct.Pin = LCD_BL_CTRL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LCD_BL_CTRL_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_OverCurrent_Pin */
  GPIO_InitStruct.Pin = OTG_FS_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OTG_FS_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : TP3_Pin NC2_Pin */
  GPIO_InitStruct.Pin = TP3_Pin|NC2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  /*Configure GPIO pin : ARDUINO_SCK_D13_Pin */
  GPIO_InitStruct.Pin = ARDUINO_SCK_D13_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(ARDUINO_SCK_D13_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LED13_Pin LED17_Pin LED11_Pin LED12_Pin
                           LED2_Pin LED18_Pin */
  GPIO_InitStruct.Pin = LED13_Pin|LED17_Pin|LED11_Pin|LED12_Pin
                          |LED2_Pin|LED18_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  /*Configure GPIO pin : PI0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

  /*Configure GPIO pin : VCP_TX_Pin */
  GPIO_InitStruct.Pin = VCP_TX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(VCP_TX_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_INT_Pin */
  GPIO_InitStruct.Pin = LCD_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(LCD_INT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PC7 PC6 */
  GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : ULPI_NXT_Pin */
  GPIO_InitStruct.Pin = ULPI_NXT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_HS;
  HAL_GPIO_Init(ULPI_NXT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : BP_JOYSTICK_Pin RMII_RXER_Pin */
  GPIO_InitStruct.Pin = BP_JOYSTICK_Pin|RMII_RXER_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pin : PF7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF8_UART7;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : ULPI_STP_Pin ULPI_DIR_Pin */
  GPIO_InitStruct.Pin = ULPI_STP_Pin|ULPI_DIR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_HS;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : EXT_RST_Pin */
  GPIO_InitStruct.Pin = EXT_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(EXT_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_SCL_Pin LCD_SDA_Pin */
  GPIO_InitStruct.Pin = LCD_SCL_Pin|LCD_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  /*Configure GPIO pins : ULPI_CLK_Pin ULPI_D0_Pin */
  GPIO_InitStruct.Pin = ULPI_CLK_Pin|ULPI_D0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_HS;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB14 PB15 */
  GPIO_InitStruct.Pin = GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* ============================================================================
 * FONCTIONS UTILITAIRES DU JEU
 * ============================================================================ */

uint8_t *(tex_ennemis[4]) = {&tex_ennemi_1, &tex_ennemi_2, &tex_ennemi_3, &tex_ennemi_4};
const uint32_t Couleur_joueur = LCD_COLOR_CYAN;
const uint32_t Couleur_monstre = LCD_COLOR_RED;
const uint32_t Couleur_missile = LCD_COLOR_WHITE;
const uint32_t Couleur_vide = LCD_COLOR_BLACK;
//const ip_addr_t IP_REGULUS = (45 << (3 * 8)) + (66 << (2 * 8)) + (110 << (1 * 8)) + (180 << (0 * 8));
/* 
static int envoie_score(int score)
{
  struct udp_pcb socket;
  &socket = new_udp();

  return 0;
}
 */

/*
  * @brief  Affichage de l'écran titre
  * @param None
  * @retval None
  */

/* ============================================================================
 * ECRAN TITRE
 * ============================================================================ */

void afficher_ecran_titre(void)
{
    while (xSemaphoreTake(MutexLCDHandle, (TickType_t)10) != pdPASS);

    BSP_LCD_Clear(LCD_COLOR_BLACK);

    // Titre principal
    BSP_LCD_SetFont(&Font24);
    BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
    BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
    BSP_LCD_DisplayStringAt(0, 60, (uint8_t *)"SPACE INVADERS", CENTER_MODE);

    // Sous-titre
    BSP_LCD_SetFont(&Font12);
    BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
    BSP_LCD_DisplayStringAt(0, 95, (uint8_t *)"STM32 Edition", CENTER_MODE);

    // Bouton JOUER — rectangle centré
    // Ecran 480x272 : bouton 160x50 centré => x=160, y=170
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_DrawRect(160, 170, 160, 50);
    BSP_LCD_SetFont(&Font20);
    BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
    BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
    BSP_LCD_DisplayStringAt(0, 187, (uint8_t *)"JOUER", CENTER_MODE);

    xSemaphoreGive(MutexLCDHandle);
}

/*
  * @brief  Tache de gestion de l'écran titre
  * @param None
  * @retval None
  */
void f_titre(void const * argument)
{
    static TS_StateTypeDef TS_State;
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriodeTache = 50 / portTICK_PERIOD_MS;
    afficher_ecran_titre();

    for (;;)
    {
        if (game_state != TITLE_SCREEN)
        {
            vTaskDelete(NULL); // On supprime la tache une fois le jeu lancé
            return;
        }

        BSP_TS_GetState(&TS_State);

        if (TS_State.touchDetected)
        {
            uint16_t tx = TS_State.touchX[0];
            uint16_t ty = TS_State.touchY[0];

            // Zone du bouton JOUER : x[160..320], y[170..220]
            if (tx >= 160 && tx <= 320 && ty >= 170 && ty <= 220)
            {
                game_state = GAME_RUNNING;

                // Effacer l'écran avant de lancer le jeu
                while (xSemaphoreTake(MutexLCDHandle, (TickType_t)10) != pdPASS);
                BSP_LCD_Clear(LCD_COLOR_BLACK);
                xSemaphoreGive(MutexLCDHandle);
            }
        }

        vTaskDelayUntil(&xLastWakeTime, xPeriodeTache);
    }
}

void repopulate_ennemie_list(uint8_t wave)
{
  int idx1;
  int idx2;
  for (idx1 = 0; idx1 < 8; idx1++)
  {
    for (idx2 = 0; idx2 < 3; idx2++)
    {
      Table_ennemis[idx1][idx2].x = (2 * idx1 + 2) * monstre_width;
      Table_ennemis[idx1][idx2].y = (2 * idx2 + 1) * monstre_height;
      Table_ennemis[idx1][idx2].health = 1; // wave / 2 + 1;
      Table_ennemis[idx1][idx2].missile.x = 0;
      Table_ennemis[idx1][idx2].missile.y = 0;
      Table_ennemis[idx1][idx2].missile.dx = 1;
      Table_ennemis[idx1][idx2].missile.dy = 0;
      Table_ennemis[idx1][idx2].missile.equipe = 0;
      Table_ennemis[idx1][idx2].missile.damage = 1; // wave / 2 + 1;
      Table_ennemis[idx1][idx2].missile.valide = 1;
      if (proba_bernoulli(1, 3))
        Table_ennemis[idx1][idx2].health = 0;
      Table_ennemis[idx1][idx2].pbmp = tex_ennemis[proba_bernoulli(1, 2)];
    }
  }
}

void update_leds(){
    for (int idx = 0; idx<=8; idx++){
      HAL_GPIO_WritePin(Leds[idx].port, Leds[idx].pin, !(charge-1<idx));
    }
}
/*
  * @brief  Encapsulation du tracer d'un rectangle
  * @param  pos_x
  * @retval None
  */
void lcd_plot_rect(uint16_t pos_x, uint16_t pos_y, uint16_t taille_x, uint16_t taille_y, uint32_t color)
{
  while (xSemaphoreTake(MutexLCDHandle, (TickType_t)10) != pdPASS)
    ;
  BSP_LCD_SetTextColor(color);
  BSP_LCD_FillRect(pos_x, pos_y, taille_x, taille_y);
  xSemaphoreGive(MutexLCDHandle);
}

void lcd_plot_circ(uint16_t pos_x, uint16_t pos_y, uint16_t rad, uint32_t color)
{
  while (xSemaphoreTake(MutexLCDHandle, (TickType_t)10) != pdPASS)
    ;
  BSP_LCD_SetTextColor(color);
  if (rad == 0)
    rad = 1;
  BSP_LCD_FillCircle(pos_x, pos_y, rad);
  xSemaphoreGive(MutexLCDHandle);
}

void lcd_plot_text_pos(uint16_t pos_x, uint16_t pos_y, uint8_t *text, uint32_t color, Text_AlignModeTypdef mode)
{
  while (xSemaphoreTake(MutexLCDHandle, (TickType_t)10) != pdPASS)
    ;
  BSP_LCD_SetTextColor(color);
  BSP_LCD_SetBackColor(Couleur_vide);
  BSP_LCD_DisplayStringAt(pos_x, pos_y, text, mode);
  xSemaphoreGive(MutexLCDHandle);
}

void lcd_plot_text_line(uint16_t line, uint8_t *text, uint32_t color)
{
  while (xSemaphoreTake(MutexLCDHandle, (TickType_t)10) != pdPASS)
    ;
  BSP_LCD_SetTextColor(color);
  BSP_LCD_SetBackColor(Couleur_vide);
  BSP_LCD_DisplayStringAtLine(line, text);
  xSemaphoreGive(MutexLCDHandle);
}

void lcd_plot_bitmap(uint16_t pos_x, uint16_t pos_y,uint8_t *pbmp){
	while (xSemaphoreTake(MutexLCDHandle, (TickType_t)10) != pdPASS)
	    ;
	  BSP_LCD_DrawBitmap(pos_x, pos_y, pbmp);
	  xSemaphoreGive(MutexLCDHandle);
}

uint8_t proba_bernoulli(uint32_t numerateur, uint32_t denominateur)
{
  uint32_t nombre_aleatoire;
  while (HAL_RNG_GenerateRandomNumber(&hrng, &nombre_aleatoire) != HAL_OK)
    ;
  uint32_t limite;
  limite = UINT32_MAX;
  limite /= denominateur;
  limite *= numerateur;
  if (nombre_aleatoire > limite)
    return 0;
  else
    return 1;
}

uint8_t proba_tirrage(uint8_t nombre_valeur)
{uint32_t nombre_aleatoire;
while (HAL_RNG_GenerateRandomNumber(&hrng, &nombre_aleatoire) != HAL_OK)
  ;
uint32_t limite = UINT32_MAX/nombre_valeur;
return (uint8_t)(nombre_aleatoire/limite);
}

uint8_t colision_missile(uint16_t m_pos_x, uint16_t m_pos_y, uint16_t o_pos_x, uint16_t o_pos_y, uint16_t o_taille_x, uint16_t o_taille_y)
{
  return ((m_pos_x > o_pos_x) & (m_pos_x < o_pos_x + o_taille_x) & (m_pos_y > o_pos_y) & (m_pos_y < o_pos_y + o_taille_y));
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_f_GameMaster */
/**
  * @brief  Function implementing the GameMaster thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_f_GameMaster */

/* ============================================================================
 * TACHE : GAME MASTER — Gestion des fins de partie et des vagues
 * ============================================================================ */

void f_GameMaster(void const * argument)
{
  /* init code for LWIP */
  MX_LWIP_Init();
  /* USER CODE BEGIN 5 */
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();
  const TickType_t xPeriodeTache = 100 / portTICK_PERIOD_MS;
  repopulate_ennemie_list(wave);
  enum End_type end;
  /* Infinite loop */
  for (;;)
  {
    while (xQueueReceive(Queue_FHandle, &end, (TickType_t)10) != pdPASS)
      ; // Tant qu'il n'y a pas de nouveau message

    if (end == END_MORT_JOUEUR)
    {
      vTaskDelete(Block_EnemieHandle);
      vTaskDelete(ProjectileHandle);
      vTaskDelete(Joueur_1Handle);
      uint8_t msg[] = "GAME OVER";
      lcd_plot_text_pos(BSP_LCD_GetXSize() / 2, BSP_LCD_GetYSize() / 2, msg, Couleur_joueur, CENTER_MODE);
      //TODO L'affichage de l'écran de fin et des scores
    }

    if (end == END_TABLEAU_VIDE)
    {
      wave++;
      repopulate_ennemie_list(wave);
    }
    vTaskDelayUntil(&xLastWakeTime, xPeriodeTache);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_f_Joueur_1 */
/**
* @brief Function implementing the Joueur_1 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_f_Joueur_1 */

/* ============================================================================
 * TACHE : JOUEUR — Deplacement, tir et gestion des degats
 * ============================================================================ */

void f_Joueur_1(void const * argument)
{
	// Attendre que le joueur appuie sur JOUER
	while (game_state != GAME_RUNNING)
	    vTaskDelay(100 / portTICK_PERIOD_MS);
  /* USER CODE BEGIN f_Joueur_1 */
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();

  const TickType_t xPeriodeTache = 5 / portTICK_PERIOD_MS;
  uint32_t joystick_h, joystick_v;

  struct Missile missile;

  uint8_t bp_1_relache = 1;
  uint8_t bp_2_relache = 1;

  ADC_ChannelConfTypeDef sConfig3 = {0};
  sConfig3.Rank = ADC_REGULAR_RANK_1;
  sConfig3.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  sConfig3.Channel = ADC_CHANNEL_8;

  HAL_ADC_ConfigChannel(&hadc3, &sConfig3);
  HAL_ADC_Start(&hadc3);

  ADC_ChannelConfTypeDef sConfig1 = {0};
  sConfig1.Rank = ADC_REGULAR_RANK_1;
  sConfig1.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  sConfig1.Channel = ADC_CHANNEL_0;

  HAL_ADC_ConfigChannel(&hadc1, &sConfig1);
  HAL_ADC_Start(&hadc1);

  enum End_type end = END_MORT_JOUEUR;
  // Paramètre de l'écran pour la reprouductibilité

  uint32_t LCD_HEIGHT = BSP_LCD_GetXSize();
  uint32_t LCD_WIDTH = BSP_LCD_GetYSize();

  const uint32_t seuil_joystick = 200;
  const uint32_t centre_joystick = 2048;
  /* Infinite loop */
  for (;;)
  {
    lcd_plot_rect(joueur.x, joueur.y, joueur_width, joueur_height, Couleur_vide);
    // BSP_LCD_DrawBitmap(uint32_t Xpos, uint32_t Ypos, uint8_t *pbmp)
    HAL_ADC_ConfigChannel(&hadc3, &sConfig3);
    HAL_ADC_Start(&hadc3);
    while (HAL_ADC_PollForConversion(&hadc3, 100) != HAL_OK);
    joystick_h = HAL_ADC_GetValue(&hadc3);

    HAL_ADC_ConfigChannel(&hadc1, &sConfig1);
    HAL_ADC_Start(&hadc1);
    while (HAL_ADC_PollForConversion(&hadc1, 100) != HAL_OK);
    joystick_v = HAL_ADC_GetValue(&hadc1);

    if ((joueur.y < LCD_WIDTH - joueur_width - joueur.dy) && (joystick_h < centre_joystick - seuil_joystick))
      joueur.y += joueur.dy;
    if ((joueur.y > joueur.dy) && (joystick_h > centre_joystick + seuil_joystick))
      joueur.y -= joueur.dy;

    if ((joueur.x < LCD_HEIGHT - joueur_height - joueur.dx) && (joystick_v < centre_joystick - seuil_joystick))
      joueur.x += joueur.dx;
    if ((joueur.x > joueur.dx) && (joystick_v > centre_joystick + seuil_joystick))
      joueur.x -= joueur.dx;

    lcd_plot_bitmap(joueur.x, joueur.y, &tex_joueur);
    //BSP_LCD_DrawBitmap(joueur.x, joueur.y, &vaisseau);

    if (xQueueReceive(Queue_JHandle, &missile, 0) == pdPASS)
      joueur.health = joueur.health - missile.damage;
    // On envoie 1 si le joueur est mort et on envoie 0 si les enemis sont tous morts
    if (joueur.health <= 0)
      xQueueSend(Queue_FHandle, &end, 0);

    if ((!HAL_GPIO_ReadPin(BP1_GPIO_Port, BP1_Pin)) && bp_1_relache)
    {
      bp_1_relache = 0;
      missile = joueur.missile;
      missile.x = joueur.x + joueur_width / 2;
      missile.y = joueur.y;
      xQueueSend(Queue_NHandle, &missile, 0);
    }
    if (HAL_GPIO_ReadPin(BP1_GPIO_Port, BP1_Pin))

      bp_1_relache = 1;

    if ((!HAL_GPIO_ReadPin(BP2_GPIO_Port, BP2_Pin)) && bp_2_relache && (charge == 8))
    {
      bp_2_relache = 0;
      missile = joueur.missile;
      missile.x = joueur.x + joueur_width / 2;
      missile.y = joueur.y;
      for (int idx_tirs = -5; idx_tirs <= 5; idx_tirs++)
      {
        missile.dy = -4;
        missile.dx = idx_tirs;
        xQueueSend(Queue_NHandle, &missile, 0);
      }
      charge = 0;
      update_leds();
    }
    if (HAL_GPIO_ReadPin(BP2_GPIO_Port, BP2_Pin))

      bp_2_relache = 1;

    vTaskDelayUntil(&xLastWakeTime, xPeriodeTache);
  }
  /* USER CODE END f_Joueur_1 */
}

/* USER CODE BEGIN Header_f_block_enemie */
/**
* @brief Function implementing the Block_Enemie thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_f_block_enemie */

/* ============================================================================
 * TACHE : BLOC ENNEMIS — Deplacement, tir ennemi et detection de fin de vague
 * ============================================================================ */

void f_block_enemie(void const * argument)
{
  /* USER CODE BEGIN f_block_enemie */
	// Attendre que le joueur appuie sur JOUER
	while (game_state != GAME_RUNNING)
	    vTaskDelay(100 / portTICK_PERIOD_MS);

  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();
  const TickType_t xPeriodeTache = 200 / portTICK_PERIOD_MS; // Toute les demi secondes
  uint8_t nombre_monstre;
  struct Collision collision;

  int8_t vitesse = 5;
  enum Sens_ennemie direction = DROITE;

  struct Missile missile = {0, 0, 0, 1, MISSILE_ENNEMI, 1, 1};
  int32_t lim_droite;
  int32_t lim_gauche;
  /* Infinite loop */
  for (;;)
  {
    while (xQueueReceive(Queue_EHandle, &collision, 0) == pdPASS)
    {
      Table_ennemis[collision.idx1][collision.idx2].health -= collision.damage;
      if (Table_ennemis[collision.idx1][collision.idx2].health <= 0)
        kill++;
    }

    for (int idx1 = 0; idx1 < 8; idx1++)
    {
      for (int idx2 = 0; idx2 < 3; idx2++)
      {
        lcd_plot_rect(Table_ennemis[idx1][idx2].x, Table_ennemis[idx1][idx2].y, monstre_width, monstre_height, Couleur_vide);
      }
    }
    lim_droite = 0;
    lim_gauche = BSP_LCD_GetXSize();
    for (int idx1 = 0; idx1 < 8; idx1++)
    {
      for (int idx2 = 0; idx2 < 3; idx2++)
      {
        if ((Table_ennemis[idx1][idx2].x > lim_droite) && (Table_ennemis[idx1][idx2].health > 0))
          lim_droite = Table_ennemis[idx1][idx2].x;
        if ((Table_ennemis[idx1][idx2].x < lim_gauche) && (Table_ennemis[idx1][idx2].health > 0))
          lim_gauche = Table_ennemis[idx1][idx2].x;
      }
    }

    if ((direction == DROITE) && (lim_droite + vitesse + monstre_width >= BSP_LCD_GetXSize()))
      direction = GAUCHE;

    else if ((direction == GAUCHE) && (lim_gauche <= vitesse))
      direction = DROITE;

    for (int idx1 = 0; idx1 < 8; idx1++)
    {
      for (int idx2 = 0; idx2 < 3; idx2++)
      {
        if (direction == DROITE)
          Table_ennemis[idx1][idx2].x += vitesse;
        if (direction == GAUCHE)
          Table_ennemis[idx1][idx2].x -= vitesse;
      }
    }
    // TODO déplacement des ennemies

    nombre_monstre = 0;
    for (int idx1 = 0; idx1 < 8; idx1++)
    {
      for (int idx2 = 0; idx2 < 3; idx2++)
      {
        if (Table_ennemis[idx1][idx2].health > 0)
        {
          nombre_monstre++;
          Table_ennemis[idx1][idx2].pbmp = tex_ennemis[proba_bernoulli(1, 2) + 2*proba_bernoulli(1, 3)];
          lcd_plot_bitmap(Table_ennemis[idx1][idx2].x, Table_ennemis[idx1][idx2].y, Table_ennemis[idx1][idx2].pbmp);
        }
      }
    }
    if (!nombre_monstre)
    {
      enum End_type end = END_TABLEAU_VIDE;
      xQueueSend(Queue_FHandle, &end, 0);
    }

    for (int idx1 = 0; idx1 < 8; idx1++)
    {
      for (int idx2 = 0; idx2 < 3; idx2++)
      {
        if (Table_ennemis[idx1][idx2].health > 0)
        {
          if (proba_bernoulli(wave, 16))
          {
            missile.x = Table_ennemis[idx1][idx2].x + monstre_width / 2;
            missile.y = Table_ennemis[idx1][idx2].y + monstre_height;
            missile.dy = 1 + proba_bernoulli(1, 8);
            xQueueSend(Queue_NHandle, &missile, 0);
          }
        }
      }
    }
    vTaskDelayUntil(&xLastWakeTime, xPeriodeTache);
  }
  /* USER CODE END f_block_enemie */
}

/* USER CODE BEGIN Header_f_projectile */
/**
* @brief Function implementing the Projectile thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_f_projectile */

/* ============================================================================
 * TACHE : PROJECTILES — Deplacement et detection de collisions
 * ============================================================================ */

void f_projectile(void const * argument)
{
  /* USER CODE BEGIN f_projectile */
	// Attendre que le joueur appuie sur JOUER
	while (game_state != GAME_RUNNING)
	    vTaskDelay(100 / portTICK_PERIOD_MS);
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();
  const TickType_t xPeriodeTache = 10 / portTICK_PERIOD_MS; // Toutes les 200 ms
  const int TAILLE_LISTE_MISSILE = 250;
  /* Infinite loop */
  struct Missile liste_missile[TAILLE_LISTE_MISSILE];
  struct Missile missile = {70, 70, 0, 3, MISSILE_ENNEMI, 1, 0};
  liste_missile[0] = missile;
  uint32_t new_x;
  uint32_t new_y;
  // Paramètre de l'écran pour la reprouductibilité

  uint32_t LCD_HEIGHT = BSP_LCD_GetYSize();
  uint32_t LCD_WIDTH = BSP_LCD_GetXSize();

  for (int idx_missile = 0; idx_missile < TAILLE_LISTE_MISSILE; idx_missile++)
    liste_missile[idx_missile] = missile;
  for (;;)
  {
    while (xQueueReceive(Queue_NHandle, &missile, 0) == pdPASS)
    {
      for (int idx_missile = 0; idx_missile < TAILLE_LISTE_MISSILE; idx_missile++)
      {
        if (liste_missile[idx_missile].valide == 0)
        {
          liste_missile[idx_missile] = missile;
          break;
        }
      }
    }

    for (int idx_missile = 0; idx_missile < TAILLE_LISTE_MISSILE; idx_missile++)
    {
      if (liste_missile[idx_missile].valide)
      {
        lcd_plot_circ(liste_missile[idx_missile].x, liste_missile[idx_missile].y, liste_missile[idx_missile].damage, Couleur_vide);

        new_x = liste_missile[idx_missile].x + liste_missile[idx_missile].dx;
        new_y = liste_missile[idx_missile].y + liste_missile[idx_missile].dy;
        if ((new_x <= 0) | (new_x >= LCD_WIDTH) | (new_y <= 0) | (new_y >= LCD_HEIGHT)) //Missile hors de l'écran
          liste_missile[idx_missile].valide = 0;
        if ((liste_missile[idx_missile].equipe == MISSILE_ENNEMI) & colision_missile(new_x, new_y, joueur.x, joueur.y, joueur_width, joueur_height))
        { //TODO condition de choc avec le joueur
          xQueueSend(Queue_JHandle, &liste_missile[idx_missile], 0);
          liste_missile[idx_missile].valide = 0;
        }
        if (liste_missile[idx_missile].equipe == MISSILE_AMI)
        {
          for (int idx_mechant_1 = 0; idx_mechant_1 < 8; idx_mechant_1++)
          {
            for (int idx_mechant_2 = 0; idx_mechant_2 < 3; idx_mechant_2++)
            {
              if ((colision_missile(new_x, new_y, Table_ennemis[idx_mechant_1][idx_mechant_2].x, Table_ennemis[idx_mechant_1][idx_mechant_2].y, monstre_width, monstre_height)) && (Table_ennemis[idx_mechant_1][idx_mechant_2].health > 0))
              {
                struct Collision collision = {idx_mechant_1, idx_mechant_2, liste_missile[idx_missile].damage};
                xQueueSend(Queue_EHandle, (void *)&collision, 0);
                liste_missile[idx_missile].valide = 0;
              }
            }
          }
        }

        liste_missile[idx_missile].x = new_x;
        liste_missile[idx_missile].y = new_y;

        if (liste_missile[idx_missile].valide)
          lcd_plot_circ(liste_missile[idx_missile].x, liste_missile[idx_missile].y, liste_missile[idx_missile].damage, Couleur_missile);
        //TODO effacage
        //TODO nouvelle position
      }
    }
    vTaskDelayUntil(&xLastWakeTime, xPeriodeTache);
  }

  /* USER CODE END f_projectile */
}

/* USER CODE BEGIN Header_f_HUD */
/**
* @brief Function implementing the HUD thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_f_HUD */

/* ============================================================================
 * TACHE : HUD — Affichage du score et des points de vie
 * ============================================================================ */

void f_HUD(void const * argument)
{
  /* USER CODE BEGIN f_HUD */
	// Attendre que le joueur appuie sur JOUER
	while (game_state != GAME_RUNNING)
	    vTaskDelay(100 / portTICK_PERIOD_MS);
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();
  const TickType_t xPeriodeTache = 100 / portTICK_PERIOD_MS; // Toutes les 200 ms
  uint8_t line_hud[100] = "";
  const uint8_t base[100] = "vague : %2u  -  kill : %2u";
  /* Infinite loop */
  for (;;)
  {
	BSP_LCD_SetFont(&Font12); // On choisit une police plus petite pour le HUD
    sprintf(line_hud, base, (uint)wave, (uint)kill);
    lcd_plot_text_line(0, line_hud, Couleur_missile);
    for(int idx = 0; idx<VIE_MAX; idx++){
    	lcd_plot_bitmap(BSP_LCD_GetXSize() - 12*(5 - idx), 1, (idx<joueur.health) ? &tex_coeur_F : &tex_coeur_E);
    }
    vTaskDelayUntil(&xLastWakeTime, xPeriodeTache);
  }
  /* USER CODE END f_HUD */
}

/* USER CODE BEGIN Header_f_chargeur */
/**
* @brief Function implementing the chargeur thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_f_chargeur */

/* ============================================================================
 * TACHE : CHARGEUR — Recharge progressive de l'attaque speciale
 * ============================================================================ */

void f_chargeur(void const * argument)
{
  /* USER CODE BEGIN f_chargeur */
	// Attendre que le joueur appuie sur JOUER
	while (game_state != GAME_RUNNING)
	    vTaskDelay(100 / portTICK_PERIOD_MS);
    TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();
  const TickType_t xPeriodeTache = 5000/ 8 / portTICK_PERIOD_MS; // Toutes les 200 ms
  /* Infinite loop */
  for(;;)
  {
    if (charge < 8)
      charge++;
    update_leds();

    vTaskDelayUntil(&xLastWakeTime, xPeriodeTache);
  }
  /* USER CODE END f_chargeur */
}

 /**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */

/* ============================================================================
 * CALLBACKS ET GESTION DES ERREURS
 * ============================================================================ */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
