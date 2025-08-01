/**
 * @file drv_led.c
 * @brief RGB LED驱动实现文件
 * @details 实现RGB LED的控制功能，支持单个LED控制、RGB颜色设置和颜色预设
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
#include <board.h>
#include <drv_common.h>

#define DBG_TAG "drv.led"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/**
 * @defgroup LED_Driver_Macros LED驱动宏定义
 * @{
 */

/** @brief LED GPIO引脚定义 */
#define LED_RED_PIN     GET_PIN(A, 6)   /**< 红色LED GPIO引脚 PA6 */
#define LED_GREEN_PIN   GET_PIN(A, 7)   /**< 绿色LED GPIO引脚 PA7 */
#define LED_BLUE_PIN    GET_PIN(A, 5)   /**< 蓝色LED GPIO引脚 PA5 */

/** @brief LED电平定义（共阳极接法，低电平点亮） */
#define LED_ON_LEVEL    PIN_LOW         /**< LED点亮电平 */
#define LED_OFF_LEVEL   PIN_HIGH        /**< LED熄灭电平 */

/**
 * @}
 */

/**
 * @defgroup LED_Driver_Variables LED驱动变量定义
 * @{
 */

/** @brief LED引脚数组 */
static const rt_base_t led_pins[LED_CHANNEL_MAX] = {
    LED_RED_PIN,    /**< 红色LED引脚 */
    LED_GREEN_PIN,  /**< 绿色LED引脚 */
    LED_BLUE_PIN    /**< 蓝色LED引脚 */
};

/** @brief LED状态数组 */
static led_state_t led_states[LED_CHANNEL_MAX] = {
    LED_OFF,        /**< 红色LED状态 */
    LED_OFF,        /**< 绿色LED状态 */
    LED_OFF         /**< 蓝色LED状态 */
};

/** @brief 当前RGB颜色值 */
static rgb_color_t current_rgb_color = {0, 0, 0};

/**
 * @}
 */

/**
 * @defgroup LED_Driver_Private_Functions LED驱动私有函数
 * @{
 */

/**
 * @brief 检查LED通道参数有效性
 * @param channel LED颜色通道
 * @return rt_bool_t 检查结果
 * @retval RT_TRUE 参数有效
 * @retval RT_FALSE 参数无效
 * @note 该函数用于检查LED通道参数是否在有效范围内
 */
static rt_bool_t led_channel_is_valid(led_channel_t channel)
{
    return (channel < LED_CHANNEL_MAX);
}

/**
 * @brief 设置LED硬件状态
 * @param channel LED颜色通道
 * @param state LED状态
 * @note 该函数直接控制LED硬件引脚状态
 */
static void led_set_hardware_state(led_channel_t channel, led_state_t state)
{
    if (!led_channel_is_valid(channel))
    {
        return;
    }
    
    rt_base_t pin_level = (state == LED_ON) ? LED_ON_LEVEL : LED_OFF_LEVEL;
    rt_pin_write(led_pins[channel], pin_level);
    led_states[channel] = state;
}

/**
 * @}
 */

/**
 * @defgroup LED_Driver_Public_Functions LED驱动公共函数
 * @{
 */

/**
 * @brief LED驱动初始化函数
 * @return rt_err_t 初始化结果
 * @retval RT_EOK 初始化成功
 * @retval -RT_ERROR 初始化失败
 * @note 该函数配置所有LED的GPIO模式为开漏输出，默认设置为高电平（LED熄灭）
 */
rt_err_t drv_led_init(void)
{
    LOG_D("LED driver initializing...");
    
    /* 配置LED GPIO为开漏输出模式 */
    for (int i = 0; i < LED_CHANNEL_MAX; i++)
    {
        rt_pin_mode(led_pins[i], PIN_MODE_OUTPUT_OD);
        /* 默认设置为高电平（LED熄灭） */
        rt_pin_write(led_pins[i], LED_OFF_LEVEL);
        led_states[i] = LED_OFF;
    }
    
    /* 初始化RGB颜色值 */
    current_rgb_color.red = 0;
    current_rgb_color.green = 0;
    current_rgb_color.blue = 0;
    
    LOG_I("LED driver initialized successfully");
    LOG_D("LED GPIO pins: R=PA6, G=PA7, B=PA5");
    
    return RT_EOK;
}

/**
 * @brief LED驱动反初始化函数
 * @return rt_err_t 反初始化结果
 * @retval RT_EOK 反初始化成功
 * @note 该函数关闭所有LED并释放资源
 */
rt_err_t drv_led_deinit(void)
{
    LOG_D("LED driver deinitializing...");
    
    /* 关闭所有LED */
    drv_led_all_off();
    
    LOG_I("LED driver deinitialized successfully");
    
    return RT_EOK;
}

/**
 * @brief 控制单个LED的开关状态
 * @param channel LED颜色通道
 * @param state LED状态
 * @return rt_err_t 控制结果
 * @retval RT_EOK 控制成功
 * @retval -RT_EINVAL 参数无效
 * @note 该函数用于控制指定LED通道的开关状态
 */
rt_err_t drv_led_set_state(led_channel_t channel, led_state_t state)
{
    if (!led_channel_is_valid(channel))
    {
        LOG_E("Invalid LED channel: %d", channel);
        return -RT_EINVAL;
    }
    
    led_set_hardware_state(channel, state);
    
    /* 更新RGB颜色值 */
    switch (channel)
    {
        case LED_RED:
            current_rgb_color.red = (state == LED_ON) ? 255 : 0;
            break;
        case LED_GREEN:
            current_rgb_color.green = (state == LED_ON) ? 255 : 0;
            break;
        case LED_BLUE:
            current_rgb_color.blue = (state == LED_ON) ? 255 : 0;
            break;
        default:
            break;
    }
    
    LOG_D("LED channel %d set to %s", channel, (state == LED_ON) ? "ON" : "OFF");
    
    return RT_EOK;
}

/**
 * @brief 获取单个LED的当前状态
 * @param channel LED颜色通道
 * @return led_state_t LED状态
 * @retval LED_ON LED开启
 * @retval LED_OFF LED关闭
 * @note 该函数用于获取指定LED通道的当前状态
 */
led_state_t drv_led_get_state(led_channel_t channel)
{
    if (!led_channel_is_valid(channel))
    {
        LOG_E("Invalid LED channel: %d", channel);
        return LED_OFF;
    }
    
    return led_states[channel];
}

/**
 * @brief 设置RGB颜色
 * @param color RGB颜色结构体指针
 * @return rt_err_t 设置结果
 * @retval RT_EOK 设置成功
 * @retval -RT_EINVAL 参数无效
 * @note 该函数根据RGB颜色值控制三个LED的亮度，实现颜色混合效果
 */
rt_err_t drv_led_set_rgb_color(const rgb_color_t *color)
{
    if (color == RT_NULL)
    {
        LOG_E("RGB color pointer is NULL");
        return -RT_EINVAL;
    }
    
    /* 根据颜色值控制LED状态（简化实现：>0为开启，=0为关闭） */
    led_set_hardware_state(LED_RED, (color->red > 0) ? LED_ON : LED_OFF);
    led_set_hardware_state(LED_GREEN, (color->green > 0) ? LED_ON : LED_OFF);
    led_set_hardware_state(LED_BLUE, (color->blue > 0) ? LED_ON : LED_OFF);
    
    /* 保存当前RGB颜色值 */
    current_rgb_color = *color;
    
    LOG_D("RGB color set to R=%d, G=%d, B=%d", color->red, color->green, color->blue);
    
    return RT_EOK;
}

/**
 * @brief 获取当前RGB颜色
 * @param color RGB颜色结构体指针，用于存储当前颜色值
 * @return rt_err_t 获取结果
 * @retval RT_EOK 获取成功
 * @retval -RT_EINVAL 参数无效
 * @note 该函数获取当前RGB LED的颜色状态
 */
rt_err_t drv_led_get_rgb_color(rgb_color_t *color)
{
    if (color == RT_NULL)
    {
        LOG_E("RGB color pointer is NULL");
        return -RT_EINVAL;
    }
    
    *color = current_rgb_color;
    
    return RT_EOK;
}

/**
 * @brief 关闭所有LED
 * @return rt_err_t 操作结果
 * @retval RT_EOK 操作成功
 * @note 该函数关闭所有LED，相当于设置为黑色
 */
rt_err_t drv_led_all_off(void)
{
    rgb_color_t black_color = RGB_COLOR_BLACK;
    return drv_led_set_rgb_color(&black_color);
}

/**
 * @brief 打开所有LED（白色）
 * @return rt_err_t 操作结果
 * @retval RT_EOK 操作成功
 * @note 该函数打开所有LED，显示白色
 */
rt_err_t drv_led_all_on(void)
{
    rgb_color_t white_color = RGB_COLOR_WHITE;
    return drv_led_set_rgb_color(&white_color);
}

/**
 * @}
 */

/**
 * @defgroup LED_Driver_Color_Presets LED驱动颜色预设函数实现
 * @{
 */

/**
 * @brief 设置LED为红色
 * @return rt_err_t 设置结果
 * @retval RT_EOK 设置成功
 * @note 该函数设置LED显示红色
 */
rt_err_t drv_led_set_red(void)
{
    rgb_color_t red_color = RGB_COLOR_RED;
    return drv_led_set_rgb_color(&red_color);
}

/**
 * @brief 设置LED为绿色
 * @return rt_err_t 设置结果
 * @retval RT_EOK 设置成功
 * @note 该函数设置LED显示绿色
 */
rt_err_t drv_led_set_green(void)
{
    rgb_color_t green_color = RGB_COLOR_GREEN;
    return drv_led_set_rgb_color(&green_color);
}

/**
 * @brief 设置LED为蓝色
 * @return rt_err_t 设置结果
 * @retval RT_EOK 设置成功
 * @note 该函数设置LED显示蓝色
 */
rt_err_t drv_led_set_blue(void)
{
    rgb_color_t blue_color = RGB_COLOR_BLUE;
    return drv_led_set_rgb_color(&blue_color);
}

/**
 * @brief 设置LED为黄色
 * @return rt_err_t 设置结果
 * @retval RT_EOK 设置成功
 * @note 该函数设置LED显示黄色（红+绿）
 */
rt_err_t drv_led_set_yellow(void)
{
    rgb_color_t yellow_color = RGB_COLOR_YELLOW;
    return drv_led_set_rgb_color(&yellow_color);
}

/**
 * @brief 设置LED为紫色（洋红）
 * @return rt_err_t 设置结果
 * @retval RT_EOK 设置成功
 * @note 该函数设置LED显示紫色（红+蓝）
 */
rt_err_t drv_led_set_magenta(void)
{
    rgb_color_t magenta_color = RGB_COLOR_MAGENTA;
    return drv_led_set_rgb_color(&magenta_color);
}

/**
 * @brief 设置LED为青色
 * @return rt_err_t 设置结果
 * @retval RT_EOK 设置成功
 * @note 该函数设置LED显示青色（绿+蓝）
 */
rt_err_t drv_led_set_cyan(void)
{
    rgb_color_t cyan_color = RGB_COLOR_CYAN;
    return drv_led_set_rgb_color(&cyan_color);
}

/**
 * @}
 */
