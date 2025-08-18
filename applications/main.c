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

extern int e35_example_init(void);
rt_err_t my_button_init(void);
int rf_init(void);
int main(void)
{
    led_app_init();
    MX_RNG_Init();
    my_button_init();
    rf_init();
    while (1)
    {
        rt_thread_mdelay(1000);
    }

    return RT_EOK;
}
