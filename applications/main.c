/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-07-26     RT-Thread    first version
 */

#include <rtthread.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/* 外部函数声明 */
extern int led_app_init(void);
extern rt_err_t my_button_init(void);
extern int rf_init(void);
extern rt_err_t app_init(void);
extern rt_err_t app_register_button_callbacks(void);
extern void MX_RNG_Init(void);

/**
 * @brief 主函数
 * @return int 程序退出码
 * @retval RT_EOK 程序正常退出
 * @note 系统主入口函数，负责初始化所有模块
 */
int main(void)
{
    rt_err_t result;

    LOG_I("系统启动中...");

    /* 初始化随机数生成器 */
    MX_RNG_Init();
    LOG_D("随机数生成器初始化完成");

    /* 初始化LED应用程序 */
    result = led_app_init();
    if (result != 0)
    {
        LOG_E("LED应用程序初始化失败");
        return -RT_ERROR;
    }
    LOG_D("LED应用程序初始化完成");

    /* 初始化按键驱动 */
    result = my_button_init();
    if (result != RT_EOK)
    {
        LOG_E("按键驱动初始化失败");
        return -RT_ERROR;
    }
    LOG_D("按键驱动初始化完成");

    /* 注册按键回调函数 */
    result = app_register_button_callbacks();
    if (result != RT_EOK)
    {
        LOG_E("按键回调注册失败");
        return -RT_ERROR;
    }
    LOG_D("按键回调注册完成");

    /* 初始化应用程序逻辑 */
    result = app_init();
    if (result != RT_EOK)
    {
        LOG_E("应用程序逻辑初始化失败");
        return -RT_ERROR;
    }
    LOG_D("应用程序逻辑初始化完成");

    /* 初始化RF通信模块 */
    result = rf_init();
    if (result != 0)
    {
        LOG_E("RF通信模块初始化失败");
        return -RT_ERROR;
    }
    LOG_D("RF通信模块初始化完成");

    LOG_I("系统初始化完成，进入主循环");

    /* 主循环 */
    while (1)
    {
        rt_thread_mdelay(1000);
        /* 可以在这里添加系统监控逻辑 */
    }

    return RT_EOK;
}
