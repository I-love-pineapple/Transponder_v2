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


void power_io_init(void)
{
    /* 电池电压检测使能引脚 */
    rt_pin_mode(V_BAT_EN, PIN_MODE_OUTPUT);
    rt_pin_write(V_BAT_EN, PIN_HIGH);

    /* 蓝牙模块电源控制引脚 */
    rt_pin_mode(RF_EN, PIN_MODE_OUTPUT_OD);
    rt_pin_write(RF_EN, PIN_LOW);
}

//#define SAMPLE_UART_NAME       "lpuart1"    /* 串口设备名称 */
//static rt_device_t serial;                /* 串口设备句柄 */
//struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;  /* 初始化配置参数 */

//void lpuart1_reconfig(void)
//{
//    serial = rt_device_find(SAMPLE_UART_NAME);
//    if (serial == RT_NULL)
//    {
//        rt_kprintf("find %s failed!\n", SAMPLE_UART_NAME);
//        return;
//    }
//
//    config.baud_rate = BAUD_RATE_9600;        //修改波特率为 9600
//
//    rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &config);
//}
