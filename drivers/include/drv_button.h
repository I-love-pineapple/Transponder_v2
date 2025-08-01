/**
 * @file drv_button.h
 * @brief 按键驱动头文件
 * @details 定义按键驱动的接口函数、宏定义和数据结构
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

#ifndef __DRV_BUTTON_H__
#define __DRV_BUTTON_H__

#include <rtthread.h>
#include <rtdevice.h>
#include "button.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup Button_Driver_Exported_Types 按键驱动导出类型定义
 * @{
 */

/**
 * @brief 按键名称枚举定义
 * @note 用于标识不同的按键
 */
typedef enum
{
    BUTTON_KEY1 = 0,    /**< 按键1 */
    BUTTON_KEY2,        /**< 按键2 */
    BUTTON_KEY3,        /**< 按键3 */
    BUTTON_KEY4,        /**< 按键4 */
    BUTTON_KEY5,        /**< 按键5 */
    BUTTON_KEY6,        /**< 按键6 */
    BUTTON_KEY_MAX      /**< 按键最大数量 */
} button_key_t;

/**
 * @}
 */

/**
 * @defgroup Button_Driver_Exported_Macros 按键驱动导出宏定义
 * @{
 */

/** @brief 按键数量定义 */
#define BUTTON_COUNT    6

/** @brief 按键名称字符串定义 */
#define BUTTON_KEY1_NAME    "key1"  /**< 按键1名称 */
#define BUTTON_KEY2_NAME    "key2"  /**< 按键2名称 */
#define BUTTON_KEY3_NAME    "key3"  /**< 按键3名称 */
#define BUTTON_KEY4_NAME    "key4"  /**< 按键4名称 */
#define BUTTON_KEY5_NAME    "key5"  /**< 按键5名称 */
#define BUTTON_KEY6_NAME    "key6"  /**< 按键6名称 */

/**
 * @}
 */

/**
 * @defgroup Button_Driver_Exported_Functions 按键驱动导出函数
 * @{
 */

/**
 * @brief 按键驱动初始化函数
 * @return rt_err_t 初始化结果
 * @retval RT_EOK 初始化成功
 * @retval -RT_ERROR 初始化失败
 * @note 该函数配置所有按键的GPIO模式并创建按键实体
 * 
 * @par 示例:
 * @code
 * rt_err_t result = drv_button_init();
 * if (result == RT_EOK)
 * {
 *     rt_kprintf("Button driver initialized successfully\n");
 * }
 * @endcode
 */
rt_err_t drv_button_init(void);

/**
 * @brief 按键驱动反初始化函数
 * @return rt_err_t 反初始化结果
 * @retval RT_EOK 反初始化成功
 * @note 该函数删除所有按键实体并释放资源
 * 
 * @par 示例:
 * @code
 * rt_err_t result = drv_button_deinit();
 * if (result == RT_EOK)
 * {
 *     rt_kprintf("Button driver deinitialized successfully\n");
 * }
 * @endcode
 */
rt_err_t drv_button_deinit(void);

/**
 * @brief 按键状态处理函数
 * @note 该函数需要在定时器或线程中周期性调用，建议调用周期为20-50ms
 * @warning 必须周期性调用此函数，否则按键事件无法正常检测
 * 
 * @par 示例:
 * @code
 * // 在定时器回调函数中调用
 * void timer_callback(void *parameter)
 * {
 *     drv_button_process();
 * }
 * @endcode
 */
void drv_button_process(void);

/**
 * @brief 获取指定按键的实体指针
 * @param key_name 按键名称字符串
 * @return Button_t* 按键实体指针
 * @retval 非NULL 成功获取按键实体指针
 * @retval NULL 未找到对应的按键实体
 * @note 该函数用于根据按键名称获取对应的按键实体指针
 * 
 * @par 示例:
 * @code
 * Button_t *btn = drv_button_get_handle("key1");
 * if (btn != RT_NULL)
 * {
 *     rt_kprintf("Got button handle for key1\n");
 * }
 * @endcode
 */
Button_t* drv_button_get_handle(const char *key_name);

/**
 * @brief 获取按键当前状态
 * @param key_name 按键名称字符串
 * @return rt_uint8_t 按键状态
 * @retval NONE_TRIGGER 无触发
 * @retval BUTTON_DOWM 按键按下
 * @retval BUTTON_UP 按键释放
 * @retval BUTTON_DOUBLE 双击
 * @retval BUTTON_LONG 长按
 * @note 该函数用于获取指定按键的当前状态
 * 
 * @par 示例:
 * @code
 * rt_uint8_t state = drv_button_get_state("key1");
 * if (state == BUTTON_DOWM)
 * {
 *     rt_kprintf("Key1 is pressed\n");
 * }
 * @endcode
 */
rt_uint8_t drv_button_get_state(const char *key_name);

/**
 * @brief 获取按键当前事件
 * @param key_name 按键名称字符串
 * @return rt_uint8_t 按键事件
 * @retval NONE_TRIGGER 无事件
 * @retval BUTTON_DOWM 按键按下事件
 * @retval BUTTON_UP 按键释放事件
 * @retval BUTTON_DOUBLE 双击事件
 * @retval BUTTON_LONG 长按事件
 * @note 该函数用于获取指定按键的当前事件
 * 
 * @par 示例:
 * @code
 * rt_uint8_t event = drv_button_get_event("key1");
 * if (event == BUTTON_DOUBLE)
 * {
 *     rt_kprintf("Key1 double clicked\n");
 * }
 * @endcode
 */
rt_uint8_t drv_button_get_event(const char *key_name);

/**
 * @brief 为指定按键绑定事件回调函数
 * @param key_name 按键名称字符串
 * @param event 按键事件类型
 * @param callback 回调函数指针
 * @return rt_err_t 绑定结果
 * @retval RT_EOK 绑定成功
 * @retval -RT_ERROR 绑定失败
 * @note 该函数用于为指定按键的特定事件绑定自定义回调函数
 * 
 * @par 示例:
 * @code
 * void my_button_callback(void *btn)
 * {
 *     rt_kprintf("Button event triggered\n");
 * }
 * 
 * rt_err_t result = drv_button_attach_callback("key1", BUTTON_DOWM, my_button_callback);
 * if (result == RT_EOK)
 * {
 *     rt_kprintf("Callback attached successfully\n");
 * }
 * @endcode
 */
rt_err_t drv_button_attach_callback(const char *key_name, Button_Event event, Button_CallBack callback);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __DRV_BUTTON_H__ */
