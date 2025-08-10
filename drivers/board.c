/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-07-26     RealThread   first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <drv_common.h>

#define V_BAT_EN    GET_PIN(B, 8)   /**< 电池电压检测使能引脚 */
#define RF_EN       GET_PIN(B, 2)   /**< 蓝牙模块电源控制引脚 */


void rt_hw_board_init()
{
    extern void hw_board_init(char *clock_src, int32_t clock_src_freq, int32_t clock_target_freq);

    /* Heap initialization */
#if defined(RT_USING_HEAP)
    rt_system_heap_init((void *) HEAP_BEGIN, (void *) HEAP_END);
#endif

    hw_board_init(BSP_CLOCK_SOURCE, BSP_CLOCK_SOURCE_FREQ_MHZ, BSP_CLOCK_SYSTEM_FREQ_MHZ);

    /* Set the shell console output device */
#if defined(RT_USING_DEVICE) && defined(RT_USING_CONSOLE)
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif

    /* Board underlying hardware initialization */
#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif
    power_io_init();
}

#ifdef BSP_USING_ADC1
/**
 * @brief ADC MSP初始化函数
 * @param hadc ADC句柄指针
 * @note 该函数配置ADC相关的GPIO和时钟
 */
void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if(hadc->Instance == ADC1)
    {
        /* 使能ADC1时钟 */
        __HAL_RCC_ADC_CLK_ENABLE();

        /* 使能GPIOC时钟 */
        __HAL_RCC_GPIOC_CLK_ENABLE();

        /**
         * ADC1 GPIO配置
         * PC0 ------> ADC1_IN1 (电池电压检测)
         */
        GPIO_InitStruct.Pin = GPIO_PIN_0;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    }
}

/**
 * @brief ADC MSP反初始化函数
 * @param hadc ADC句柄指针
 * @note 该函数释放ADC相关的GPIO和时钟资源
 */
void HAL_ADC_MspDeInit(ADC_HandleTypeDef* hadc)
{
    if(hadc->Instance == ADC1)
    {
        /* 禁用ADC1时钟 */
        __HAL_RCC_ADC_CLK_DISABLE();

        /**
         * ADC1 GPIO反初始化
         * PC0 ------> ADC1_IN1
         */
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_0);
    }
}
#endif /* BSP_USING_ADC1 */

#ifdef BSP_USING_RNG
void HAL_RNG_MspInit(RNG_HandleTypeDef *rngHandle)
{

    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    if (rngHandle->Instance == RNG)
    {
        /* USER CODE BEGIN RNG_MspInit 0 */

        /* USER CODE END RNG_MspInit 0 */

        /** Initializes the peripherals clock
         */
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RNG;
        PeriphClkInit.RngClockSelection = RCC_RNGCLKSOURCE_PLLSAI1;
        PeriphClkInit.PLLSAI1.PLLSAI1Source = RCC_PLLSOURCE_HSI;
        PeriphClkInit.PLLSAI1.PLLSAI1M = 1;
        PeriphClkInit.PLLSAI1.PLLSAI1N = 8;
        PeriphClkInit.PLLSAI1.PLLSAI1P = RCC_PLLP_DIV7;
        PeriphClkInit.PLLSAI1.PLLSAI1Q = RCC_PLLQ_DIV8;
        PeriphClkInit.PLLSAI1.PLLSAI1R = RCC_PLLR_DIV2;
        PeriphClkInit.PLLSAI1.PLLSAI1ClockOut = RCC_PLLSAI1_48M2CLK;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
        {
            Error_Handler();
        }

        /* RNG clock enable */
        __HAL_RCC_RNG_CLK_ENABLE();
        /* USER CODE BEGIN RNG_MspInit 1 */

        /* USER CODE END RNG_MspInit 1 */
    }
}

void HAL_RNG_MspDeInit(RNG_HandleTypeDef *rngHandle)
{

    if (rngHandle->Instance == RNG)
    {
        /* USER CODE BEGIN RNG_MspDeInit 0 */

        /* USER CODE END RNG_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_RNG_CLK_DISABLE();
        /* USER CODE BEGIN RNG_MspDeInit 1 */

        /* USER CODE END RNG_MspDeInit 1 */
    }
}
#endif /* BSP_USING_RNG */

void power_io_init(void)
{
    /* 蓝牙模块电源控制引脚 */
    rt_pin_mode(RF_EN, PIN_MODE_OUTPUT_OD);
    rt_pin_write(RF_EN, PIN_LOW);
}

