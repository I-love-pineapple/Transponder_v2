/*
 * Copyright (c) 2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-01-XX     user         STM32L431RCT6低功耗管理模块 - STOP1模式按键唤醒
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <drv_common.h>

#define DBG_TAG "lowpower"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/* 按键引脚定义（根据您的硬件连接修改） */
#define WAKEUP_BUTTON_PIN    GET_PIN(C, 13)  /* 假设按键连接到PC13 */

/* 低功耗管理结构体 */
typedef struct {
    rt_bool_t enabled;           /* 低功耗模式是否启用 */
    rt_bool_t button_configured; /* 按键唤醒是否已配置 */
} lowpower_config_t;

static lowpower_config_t g_lp_config = {
    .enabled = RT_FALSE,
    .button_configured = RT_FALSE
};

/**
 * @brief 配置所有GPIO为低功耗状态
 * @note  为了达到最低功耗，需要将所有未使用的GPIO配置为模拟输入模式
 */
static void lowpower_gpio_config(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* 使能所有GPIO时钟 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    
    LOG_I("配置GPIO为低功耗状态...");
    
    /* 配置GPIOA的所有引脚为模拟输入模式（除了调试引脚PA13/PA14和UART引脚） */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                          GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 |
                          GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 |
                          GPIO_PIN_12 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    /* 配置GPIOB的所有引脚为模拟输入模式（除了PB2和PB8电源控制引脚） */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3 | GPIO_PIN_4 |
                          GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_9 |
                          GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 |
                          GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    /* 配置GPIOC的引脚为模拟输入模式（除了PC0 ADC和PC13按键） */
    GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 |
                          GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 |
                          GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 |
                          GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    
    /* 配置GPIOD的所有引脚为模拟输入模式 */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                          GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 |
                          GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 |
                          GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    
    /* 配置GPIOH的所有引脚为模拟输入模式（除了HSE相关引脚） */
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
    
    LOG_I("GPIO低功耗配置完成");
}

/**
 * @brief 配置按键唤醒源
 */
static void lowpower_button_wakeup_config(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* 确保GPIOC时钟使能 */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    
    /* 配置PC13作为按键唤醒源 */
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;  /* 下降沿触发 */
    GPIO_InitStruct.Pull = GPIO_PULLUP;           /* 内部上拉 */
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    
    /* 配置EXTI中断 */
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
    
    g_lp_config.button_configured = RT_TRUE;
    LOG_I("按键唤醒源配置完成 (PC13)");
}

/**
 * @brief 系统时钟重新配置（从STOP1模式唤醒后）
 * @note STOP1模式唤醒后，系统时钟会切换到HSI或MSI，需要重新配置到目标频率
 */
static void lowpower_sysclock_reconfig(void)
{
    /* 重新初始化系统时钟 */
    extern void SystemClock_Config(void);
    SystemClock_Config();
    
    /* 重新启动SysTick */
    HAL_ResumeTick();
    
    LOG_I("系统时钟重新配置完成");
}

/**
 * @brief 进入STOP1模式
 * @return RT_EOK 成功，其他值失败
 */
rt_err_t lowpower_enter_stop1_mode(void)
{
    if (!g_lp_config.enabled) {
        LOG_W("低功耗模式未启用");
        return -RT_ERROR;
    }
    
    if (!g_lp_config.button_configured) {
        LOG_W("按键唤醒未配置");
        return -RT_ERROR;
    }
    
    LOG_I("准备进入STOP1模式...");
    LOG_I("按下PC13按键可以唤醒系统");
    
    /* 延时确保日志输出完成 */
    rt_thread_mdelay(100);
    
    /* 暂停SysTick以避免在低功耗模式下产生中断 */
    HAL_SuspendTick();
    
    /* 清除所有唤醒标志 */
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF1 | PWR_FLAG_WUF2 | PWR_FLAG_WUF3 | 
                         PWR_FLAG_WUF4 | PWR_FLAG_WUF5);
    
    /* 进入STOP1模式 */
    LOG_I("进入STOP1模式");
    HAL_PWREx_EnterSTOP1Mode(PWR_STOPENTRY_WFI);
    
    /* 从STOP1模式唤醒后会执行到这里 */
    LOG_I("从STOP1模式唤醒");
    
    /* 重新配置系统时钟 */
    lowpower_sysclock_reconfig();
    
    return RT_EOK;
}

/**
 * @brief 启用低功耗模式（STOP1 + 按键唤醒）
 */
void lowpower_enable(void)
{
    LOG_I("启用低功耗模式 (STOP1 + 按键唤醒)");
    
    /* 配置GPIO为低功耗状态 */
    lowpower_gpio_config();
    
    /* 配置按键唤醒源 */
    lowpower_button_wakeup_config();
    
    /* 标记为已启用 */
    g_lp_config.enabled = RT_TRUE;
    
    LOG_I("低功耗模式启用完成");
}

/**
 * @brief 禁用低功耗模式
 */
void lowpower_disable(void)
{
    LOG_I("禁用低功耗模式");
    
    /* 禁用按键中断 */
    HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
    
    g_lp_config.enabled = RT_FALSE;
    g_lp_config.button_configured = RT_FALSE;
    
    LOG_I("低功耗模式已禁用");
}

/**
 * @brief 进入睡眠模式（STOP1）
 */
void lowpower_sleep(void)
{
    if (g_lp_config.enabled) {
        lowpower_enter_stop1_mode();
    } else {
        LOG_W("低功耗模式未启用，请先调用 lowpower_enable()");
    }
}

/**
 * @brief 检查低功耗模式是否启用
 * @return RT_TRUE 已启用，RT_FALSE 未启用
 */
rt_bool_t lowpower_is_enabled(void)
{
    return g_lp_config.enabled;
}

/* ===== 终端命令接口 ===== */

/**
 * @brief 低功耗测试命令
 */
static void cmd_lowpower_test(void)
{
    rt_kprintf("=== STM32L431RCT6 低功耗测试 (STOP1模式) ===\n");
    rt_kprintf("1. 启用低功耗模式\n");
    rt_kprintf("2. 配置PC13按键作为唤醒源\n");
    rt_kprintf("3. 3秒后进入STOP1模式\n");
    rt_kprintf("4. 按下PC13按键唤醒系统\n\n");
    
    /* 启用低功耗模式 */
    lowpower_enable();
    
    rt_kprintf("3秒后进入STOP1模式，按下PC13按键唤醒...\n");
    rt_thread_mdelay(3000);
    
    /* 进入STOP1模式 */
    lowpower_sleep();
    
    rt_kprintf("=== 已从STOP1模式唤醒! ===\n");
}
MSH_CMD_EXPORT(cmd_lowpower_test, STM32L431 STOP1模式测试);

/**
 * @brief 启用低功耗命令
 */
static void cmd_lp_enable(void)
{
    lowpower_enable();
    rt_kprintf("低功耗模式 (STOP1 + 按键唤醒) 已启用\n");
}
MSH_CMD_EXPORT(cmd_lp_enable, 启用低功耗模式);

/**
 * @brief 禁用低功耗命令
 */
static void cmd_lp_disable(void)
{
    lowpower_disable();
    rt_kprintf("低功耗模式已禁用\n");
}
MSH_CMD_EXPORT(cmd_lp_disable, 禁用低功耗模式);

/**
 * @brief 进入睡眠命令
 */
static void cmd_lp_sleep(void)
{
    if (lowpower_is_enabled()) {
        rt_kprintf("进入STOP1模式，按下PC13按键唤醒...\n");
        rt_thread_mdelay(100);  /* 等待打印完成 */
        lowpower_sleep();
        rt_kprintf("从STOP1模式唤醒成功!\n");
    } else {
        rt_kprintf("错误：请先使用 cmd_lp_enable 启用低功耗模式\n");
    }
}
MSH_CMD_EXPORT(cmd_lp_sleep, 进入STOP1睡眠模式);

/**
 * @brief 查看低功耗状态命令
 */
static void cmd_lp_status(void)
{
    rt_kprintf("=== 低功耗模式状态 ===\n");
    rt_kprintf("模式: STOP1 + 按键唤醒\n");
    rt_kprintf("状态: %s\n", g_lp_config.enabled ? "已启用" : "未启用");
    rt_kprintf("按键配置: %s\n", g_lp_config.button_configured ? "已配置 (PC13)" : "未配置");
    rt_kprintf("唤醒方式: PC13按键下降沿触发\n");
}
MSH_CMD_EXPORT(cmd_lp_status, 查看低功耗状态);

/* ===== 中断处理函数 ===== */

/**
 * @brief GPIO中断处理函数（用于按键唤醒）
 */
void EXTI15_10_IRQHandler(void)
{
    /* 进入中断 */
    rt_interrupt_enter();
    
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_13) != RESET) {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_13);
        /* 在中断中不打印日志，避免影响唤醒性能 */
    }
    
    /* 退出中断 */
    rt_interrupt_leave();
}

/**
 * @brief 低功耗模块初始化
 */
int lowpower_init(void)
{
    LOG_I("STM32L431RCT6 低功耗模块初始化");
    
    /* 启用PWR时钟 */
    __HAL_RCC_PWR_CLK_ENABLE();
    
    /* 初始化配置 */
    g_lp_config.enabled = RT_FALSE;
    g_lp_config.button_configured = RT_FALSE;
    
    LOG_I("低功耗模块初始化完成");
    LOG_I("使用命令测试:");
    LOG_I("  cmd_lowpower_test  - 完整测试流程");
    LOG_I("  cmd_lp_enable      - 启用低功耗");
    LOG_I("  cmd_lp_sleep       - 进入睡眠");
    LOG_I("  cmd_lp_status      - 查看状态");
    
    return RT_EOK;
}
INIT_DEVICE_EXPORT(lowpower_init);

