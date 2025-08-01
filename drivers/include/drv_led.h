/**
 * @file drv_led.h
 * @brief RGB LED驱动头文件
 * @details 定义RGB LED驱动的接口函数、宏定义和数据结构
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

#ifndef __DRV_LED_H__
#define __DRV_LED_H__

#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup LED_Driver_Exported_Types LED驱动导出类型定义
 * @{
 */

/**
 * @brief LED颜色通道枚举定义
 * @note 用于标识不同的LED颜色通道
 */
typedef enum
{
    LED_RED = 0,        /**< 红色LED */
    LED_GREEN,          /**< 绿色LED */
    LED_BLUE,           /**< 蓝色LED */
    LED_CHANNEL_MAX     /**< LED通道最大数量 */
} led_channel_t;

/**
 * @brief LED状态枚举定义
 * @note 用于表示LED的开关状态
 */
typedef enum
{
    LED_OFF = 0,        /**< LED关闭 */
    LED_ON              /**< LED开启 */
} led_state_t;

/**
 * @brief RGB颜色结构体定义
 * @note 用于表示RGB颜色值
 */
typedef struct
{
    rt_uint8_t red;     /**< 红色分量 (0-255) */
    rt_uint8_t green;   /**< 绿色分量 (0-255) */
    rt_uint8_t blue;    /**< 蓝色分量 (0-255) */
} rgb_color_t;

/**
 * @}
 */

/**
 * @defgroup LED_Driver_Exported_Macros LED驱动导出宏定义
 * @{
 */

/** @brief LED数量定义 */
#define LED_COUNT           3

/** @brief LED颜色预设值定义 */
#define RGB_COLOR_BLACK     {0,   0,   0}      /**< 黑色（全灭） */
#define RGB_COLOR_WHITE     {255, 255, 255}    /**< 白色 */
#define RGB_COLOR_RED       {255, 0,   0}      /**< 红色 */
#define RGB_COLOR_GREEN     {0,   255, 0}      /**< 绿色 */
#define RGB_COLOR_BLUE      {0,   0,   255}    /**< 蓝色 */
#define RGB_COLOR_YELLOW    {255, 255, 0}      /**< 黄色 */
#define RGB_COLOR_MAGENTA   {255, 0,   255}    /**< 紫色（洋红） */
#define RGB_COLOR_CYAN      {0,   255, 255}    /**< 青色 */
#define RGB_COLOR_ORANGE    {255, 165, 0}      /**< 橙色 */
#define RGB_COLOR_PURPLE    {128, 0,   128}    /**< 紫色 */
#define RGB_COLOR_PINK      {255, 192, 203}    /**< 粉色 */

/**
 * @}
 */

/**
 * @defgroup LED_Driver_Exported_Functions LED驱动导出函数
 * @{
 */

/**
 * @brief LED驱动初始化函数
 * @return rt_err_t 初始化结果
 * @retval RT_EOK 初始化成功
 * @retval -RT_ERROR 初始化失败
 * @note 该函数配置所有LED的GPIO模式为开漏输出，默认设置为高电平（LED熄灭）
 * 
 * @par 示例:
 * @code
 * rt_err_t result = drv_led_init();
 * if (result == RT_EOK)
 * {
 *     rt_kprintf("LED driver initialized successfully\n");
 * }
 * @endcode
 */
rt_err_t drv_led_init(void);

/**
 * @brief LED驱动反初始化函数
 * @return rt_err_t 反初始化结果
 * @retval RT_EOK 反初始化成功
 * @note 该函数关闭所有LED并释放资源
 * 
 * @par 示例:
 * @code
 * rt_err_t result = drv_led_deinit();
 * if (result == RT_EOK)
 * {
 *     rt_kprintf("LED driver deinitialized successfully\n");
 * }
 * @endcode
 */
rt_err_t drv_led_deinit(void);

/**
 * @brief 控制单个LED的开关状态
 * @param channel LED颜色通道
 * @param state LED状态
 * @return rt_err_t 控制结果
 * @retval RT_EOK 控制成功
 * @retval -RT_EINVAL 参数无效
 * @note 该函数用于控制指定LED通道的开关状态
 * 
 * @par 示例:
 * @code
 * // 打开红色LED
 * drv_led_set_state(LED_RED, LED_ON);
 * // 关闭绿色LED
 * drv_led_set_state(LED_GREEN, LED_OFF);
 * @endcode
 */
rt_err_t drv_led_set_state(led_channel_t channel, led_state_t state);

/**
 * @brief 获取单个LED的当前状态
 * @param channel LED颜色通道
 * @return led_state_t LED状态
 * @retval LED_ON LED开启
 * @retval LED_OFF LED关闭
 * @note 该函数用于获取指定LED通道的当前状态
 * 
 * @par 示例:
 * @code
 * led_state_t state = drv_led_get_state(LED_RED);
 * if (state == LED_ON)
 * {
 *     rt_kprintf("Red LED is on\n");
 * }
 * @endcode
 */
led_state_t drv_led_get_state(led_channel_t channel);

/**
 * @brief 设置RGB颜色
 * @param color RGB颜色结构体指针
 * @return rt_err_t 设置结果
 * @retval RT_EOK 设置成功
 * @retval -RT_EINVAL 参数无效
 * @note 该函数根据RGB颜色值控制三个LED的亮度，实现颜色混合效果
 * 
 * @par 示例:
 * @code
 * rgb_color_t red_color = RGB_COLOR_RED;
 * drv_led_set_rgb_color(&red_color);
 * @endcode
 */
rt_err_t drv_led_set_rgb_color(const rgb_color_t *color);

/**
 * @brief 获取当前RGB颜色
 * @param color RGB颜色结构体指针，用于存储当前颜色值
 * @return rt_err_t 获取结果
 * @retval RT_EOK 获取成功
 * @retval -RT_EINVAL 参数无效
 * @note 该函数获取当前RGB LED的颜色状态
 * 
 * @par 示例:
 * @code
 * rgb_color_t current_color;
 * drv_led_get_rgb_color(&current_color);
 * rt_kprintf("RGB: R=%d, G=%d, B=%d\n", 
 *            current_color.red, current_color.green, current_color.blue);
 * @endcode
 */
rt_err_t drv_led_get_rgb_color(rgb_color_t *color);

/**
 * @brief 关闭所有LED
 * @return rt_err_t 操作结果
 * @retval RT_EOK 操作成功
 * @note 该函数关闭所有LED，相当于设置为黑色
 * 
 * @par 示例:
 * @code
 * drv_led_all_off();
 * @endcode
 */
rt_err_t drv_led_all_off(void);

/**
 * @brief 打开所有LED（白色）
 * @return rt_err_t 操作结果
 * @retval RT_EOK 操作成功
 * @note 该函数打开所有LED，显示白色
 * 
 * @par 示例:
 * @code
 * drv_led_all_on();
 * @endcode
 */
rt_err_t drv_led_all_on(void);

/**
 * @}
 */

/**
 * @defgroup LED_Driver_Color_Presets LED驱动颜色预设函数
 * @{
 */

/**
 * @brief 设置LED为红色
 * @return rt_err_t 设置结果
 * @retval RT_EOK 设置成功
 */
rt_err_t drv_led_set_red(void);

/**
 * @brief 设置LED为绿色
 * @return rt_err_t 设置结果
 * @retval RT_EOK 设置成功
 */
rt_err_t drv_led_set_green(void);

/**
 * @brief 设置LED为蓝色
 * @return rt_err_t 设置结果
 * @retval RT_EOK 设置成功
 */
rt_err_t drv_led_set_blue(void);

/**
 * @brief 设置LED为黄色
 * @return rt_err_t 设置结果
 * @retval RT_EOK 设置成功
 */
rt_err_t drv_led_set_yellow(void);

/**
 * @brief 设置LED为紫色（洋红）
 * @return rt_err_t 设置结果
 * @retval RT_EOK 设置成功
 */
rt_err_t drv_led_set_magenta(void);

/**
 * @brief 设置LED为青色
 * @return rt_err_t 设置结果
 * @retval RT_EOK 设置成功
 */
rt_err_t drv_led_set_cyan(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __DRV_LED_H__ */
