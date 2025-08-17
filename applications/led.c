/**
 * @file drv_led_test.c
 * @brief RGB LED驱动测试文件
 * @details 提供RGB LED驱动的测试函数和MSH命令接口
 * @author RT-Thread Team
 * @date 2024-08-01
 * @version 1.0.0
 * 
 * @copyright Copyright (c) 2024 RT-Thread Development Team
 * 
 * @par 修改日志:
 * <table>
 * <tr><th>日期       <th>版本  <th>作者     <th>说明
 * <tr><td>2024-08-01 <td>1.0.0 <td>RT-Thread Team <td>首次创建
 * </table>
 */

#include "drv_led.h"
#include <rtthread.h>
#include <rtdevice.h>

/**
 * @defgroup LED_Test_Functions LED测试函数
 * @{
 */

/**
 * @brief LED驱动基本功能测试
 * @return rt_err_t 测试结果
 * @retval RT_EOK 测试通过
 * @retval -RT_ERROR 测试失败
 * @note 该函数测试LED驱动的基本初始化和控制功能
 */
static rt_err_t led_basic_test(void)
{
    rt_err_t result;
    
    rt_kprintf("[TEST] Starting LED basic test...\n");
    
    /* 测试LED驱动初始化 */
    result = drv_led_init();
    if (result != RT_EOK)
    {
        rt_kprintf("[TEST] LED driver init failed!\n");
        return -RT_ERROR;
    }
    rt_kprintf("[TEST] LED driver init: PASS\n");
    
    /* 测试单个LED控制 */
    rt_kprintf("[TEST] Testing individual LED control...\n");
    
    /* 测试红色LED */
    drv_led_set_state(LED_RED, LED_ON);
    rt_thread_mdelay(500);
    if (drv_led_get_state(LED_RED) != LED_ON)
    {
        rt_kprintf("[TEST] Red LED control failed!\n");
        return -RT_ERROR;
    }
    drv_led_set_state(LED_RED, LED_OFF);
    rt_thread_mdelay(200);
    
    /* 测试绿色LED */
    drv_led_set_state(LED_GREEN, LED_ON);
    rt_thread_mdelay(500);
    if (drv_led_get_state(LED_GREEN) != LED_ON)
    {
        rt_kprintf("[TEST] Green LED control failed!\n");
        return -RT_ERROR;
    }
    drv_led_set_state(LED_GREEN, LED_OFF);
    rt_thread_mdelay(200);
    
    /* 测试蓝色LED */
    drv_led_set_state(LED_BLUE, LED_ON);
    rt_thread_mdelay(500);
    if (drv_led_get_state(LED_BLUE) != LED_ON)
    {
        rt_kprintf("[TEST] Blue LED control failed!\n");
        return -RT_ERROR;
    }
    drv_led_set_state(LED_BLUE, LED_OFF);
    rt_thread_mdelay(200);
    
    rt_kprintf("[TEST] Individual LED control: PASS\n");
    
    /* 测试RGB颜色设置 */
    rt_kprintf("[TEST] Testing RGB color setting...\n");
    
    rgb_color_t test_color = {255, 0, 0}; // 红色
    result = drv_led_set_rgb_color(&test_color);
    if (result != RT_EOK)
    {
        rt_kprintf("[TEST] RGB color setting failed!\n");
        return -RT_ERROR;
    }
    
    rgb_color_t current_color;
    result = drv_led_get_rgb_color(&current_color);
    if (result != RT_EOK || current_color.red != 255 || current_color.green != 0 || current_color.blue != 0)
    {
        rt_kprintf("[TEST] RGB color getting failed!\n");
        return -RT_ERROR;
    }
    
    rt_thread_mdelay(500);
    drv_led_all_off();
    
    rt_kprintf("[TEST] RGB color setting: PASS\n");
    rt_kprintf("[TEST] LED basic test: PASS\n");
    
    return RT_EOK;
}

/**
 * @brief LED颜色预设测试
 * @return rt_err_t 测试结果
 * @retval RT_EOK 测试通过
 * @retval -RT_ERROR 测试失败
 * @note 该函数测试LED驱动的颜色预设功能
 */
static rt_err_t led_color_preset_test(void)
{
    rt_kprintf("[TEST] Starting LED color preset test...\n");
    
    /* 测试各种颜色预设 */
    rt_kprintf("[TEST] Testing color presets...\n");
    
    rt_kprintf("  Red...\n");
    drv_led_set_red();
    rt_thread_mdelay(800);
    
    rt_kprintf("  Green...\n");
    drv_led_set_green();
    rt_thread_mdelay(800);
    
    rt_kprintf("  Blue...\n");
    drv_led_set_blue();
    rt_thread_mdelay(800);
    
    rt_kprintf("  Yellow...\n");
    drv_led_set_yellow();
    rt_thread_mdelay(800);
    
    rt_kprintf("  Magenta...\n");
    drv_led_set_magenta();
    rt_thread_mdelay(800);
    
    rt_kprintf("  Cyan...\n");
    drv_led_set_cyan();
    rt_thread_mdelay(800);
    
    rt_kprintf("  White (All ON)...\n");
    drv_led_all_on();
    rt_thread_mdelay(800);
    
    rt_kprintf("  Black (All OFF)...\n");
    drv_led_all_off();
    rt_thread_mdelay(500);
    
    rt_kprintf("[TEST] LED color preset test: PASS\n");
    
    return RT_EOK;
}

/**
 * @brief LED驱动完整测试函数
 * @return rt_err_t 测试结果
 * @retval RT_EOK 测试通过
 * @retval -RT_ERROR 测试失败
 * @note 该函数执行完整的LED驱动测试
 */
rt_err_t drv_led_full_test(void)
{
    rt_err_t result;
    
    rt_kprintf("\n=== RGB LED Driver Full Test ===\n");
    
    /* 执行基本功能测试 */
    result = led_basic_test();
    if (result != RT_EOK)
    {
        rt_kprintf("[TEST] Basic test failed!\n");
        return -RT_ERROR;
    }
    
    /* 执行颜色预设测试 */
    result = led_color_preset_test();
    if (result != RT_EOK)
    {
        rt_kprintf("[TEST] Color preset test failed!\n");
        return -RT_ERROR;
    }
    
    rt_kprintf("[TEST] LED GPIO pins:\n");
    rt_kprintf("  Red LED:   PA6\n");
    rt_kprintf("  Green LED: PA7\n");
    rt_kprintf("  Blue LED:  PA5\n");
    rt_kprintf("=== All Tests Passed ===\n\n");
    
    return RT_EOK;
}

/**
 * @}
 */

/**
 * @defgroup LED_MSH_Commands LED MSH命令接口
 * @{
 */

/**
 * @brief LED测试MSH命令
 * @note 该函数提供LED驱动测试的MSH命令接口
 */
static void led_test(void)
{
    drv_led_full_test();
}

/**
 * @brief LED初始化MSH命令
 * @note 该函数提供LED驱动初始化的MSH命令接口
 */
static void led_init(void)
{
    rt_err_t result = drv_led_init();
    if (result == RT_EOK)
    {
        rt_kprintf("LED driver initialized successfully\n");
    }
    else
    {
        rt_kprintf("LED driver initialization failed\n");
    }
}

/**
 * @brief LED控制MSH命令
 * @param argc 参数个数
 * @param argv 参数数组
 * @note 该函数提供LED控制的MSH命令接口
 * 
 * @par 用法:
 * @code
 * led_ctrl red on      # 打开红色LED
 * led_ctrl green off   # 关闭绿色LED
 * led_ctrl all off     # 关闭所有LED
 * @endcode
 */
static void led_ctrl(int argc, char **argv)
{
    if (argc < 3)
    {
        rt_kprintf("Usage: led_ctrl <channel> <state>\n");
        rt_kprintf("  channel: red, green, blue, all\n");
        rt_kprintf("  state: on, off\n");
        rt_kprintf("Example: led_ctrl red on\n");
        return;
    }
    
    const char *channel = argv[1];
    const char *state = argv[2];
    
    led_state_t led_state = (rt_strcmp(state, "on") == 0) ? LED_ON : LED_OFF;
    
    if (rt_strcmp(channel, "red") == 0)
    {
        drv_led_set_state(LED_RED, led_state);
        rt_kprintf("Red LED %s\n", (led_state == LED_ON) ? "ON" : "OFF");
    }
    else if (rt_strcmp(channel, "green") == 0)
    {
        drv_led_set_state(LED_GREEN, led_state);
        rt_kprintf("Green LED %s\n", (led_state == LED_ON) ? "ON" : "OFF");
    }
    else if (rt_strcmp(channel, "blue") == 0)
    {
        drv_led_set_state(LED_BLUE, led_state);
        rt_kprintf("Blue LED %s\n", (led_state == LED_ON) ? "ON" : "OFF");
    }
    else if (rt_strcmp(channel, "all") == 0)
    {
        if (led_state == LED_ON)
        {
            drv_led_all_on();
            rt_kprintf("All LEDs ON (White)\n");
        }
        else
        {
            drv_led_all_off();
            rt_kprintf("All LEDs OFF\n");
        }
    }
    else
    {
        rt_kprintf("Invalid channel: %s\n", channel);
    }
}

/**
 * @brief LED颜色设置MSH命令
 * @param argc 参数个数
 * @param argv 参数数组
 * @note 该函数提供LED颜色设置的MSH命令接口
 * 
 * @par 用法:
 * @code
 * led_color red        # 设置为红色
 * led_color green      # 设置为绿色
 * led_color blue       # 设置为蓝色
 * led_color yellow     # 设置为黄色
 * led_color magenta    # 设置为紫色
 * led_color cyan       # 设置为青色
 * @endcode
 */
static void led_color(int argc, char **argv)
{
    if (argc < 2)
    {
        rt_kprintf("Usage: led_color <color>\n");
        rt_kprintf("  color: red, green, blue, yellow, magenta, cyan, white, black\n");
        rt_kprintf("Example: led_color red\n");
        return;
    }
    
    const char *color = argv[1];
    
    if (rt_strcmp(color, "red") == 0)
    {
        drv_led_set_red();
        rt_kprintf("LED color set to RED\n");
    }
    else if (rt_strcmp(color, "green") == 0)
    {
        drv_led_set_green();
        rt_kprintf("LED color set to GREEN\n");
    }
    else if (rt_strcmp(color, "blue") == 0)
    {
        drv_led_set_blue();
        rt_kprintf("LED color set to BLUE\n");
    }
    else if (rt_strcmp(color, "yellow") == 0)
    {
        drv_led_set_yellow();
        rt_kprintf("LED color set to YELLOW\n");
    }
    else if (rt_strcmp(color, "magenta") == 0)
    {
        drv_led_set_magenta();
        rt_kprintf("LED color set to MAGENTA\n");
    }
    else if (rt_strcmp(color, "cyan") == 0)
    {
        drv_led_set_cyan();
        rt_kprintf("LED color set to CYAN\n");
    }
    else if (rt_strcmp(color, "white") == 0)
    {
        drv_led_all_on();
        rt_kprintf("LED color set to WHITE\n");
    }
    else if (rt_strcmp(color, "black") == 0)
    {
        drv_led_all_off();
        rt_kprintf("LED color set to BLACK (OFF)\n");
    }
    else
    {
        rt_kprintf("Invalid color: %s\n", color);
    }
}

/* 导出MSH命令 */
MSH_CMD_EXPORT(led_test, RGB LED driver test);
MSH_CMD_EXPORT(led_init, Initialize RGB LED driver);
MSH_CMD_EXPORT(led_ctrl, Control individual LED);
MSH_CMD_EXPORT(led_color, Set LED color);

/**
 * @}
 */
