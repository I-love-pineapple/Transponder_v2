/**
 * @file drv_button.c
 * @brief 按键驱动实现文件
 * @details 基于button软件包实现6个按键的驱动程序，支持单击、双击、长按等多种按键事件
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

#include "drv_button.h"
#include "button.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <drv_common.h>

#define DBG_TAG "drv.button"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#ifdef PKG_USING_BUTTON

/**
 * @defgroup Button_Driver_Macros 按键驱动宏定义
 * @{
 */

/** @brief 按键GPIO引脚定义 */
#define KEY1_PIN    GET_PIN(C, 4)   /**< 按键1 GPIO引脚 PC4 */
#define KEY2_PIN    GET_PIN(B, 14)  /**< 按键2 GPIO引脚 PB14 */
#define KEY3_PIN    GET_PIN(A, 0)   /**< 按键3 GPIO引脚 PA0 */
#define KEY4_PIN    GET_PIN(A, 8)   /**< 按键4 GPIO引脚 PA8 */
#define KEY5_PIN    GET_PIN(B, 7)   /**< 按键5 GPIO引脚 PB7 */
#define KEY6_PIN    GET_PIN(A, 15)  /**< 按键6 GPIO引脚 PA15 */

/** @brief 按键触发电平定义 */
#define KEY_TRIGGER_LEVEL   PIN_LOW /**< 按键按下时的电平状态 */

/**
 * @}
 */

/**
 * @defgroup Button_Driver_Variables 按键驱动变量定义
 * @{
 */

/** @brief 按键实体结构体定义 */
static Button_t key1_btn;  /**< 按键1实体 */
static Button_t key2_btn;  /**< 按键2实体 */
static Button_t key3_btn;  /**< 按键3实体 */
static Button_t key4_btn;  /**< 按键4实体 */
static Button_t key5_btn;  /**< 按键5实体 */
static Button_t key6_btn;  /**< 按键6实体 */

/**
 * @}
 */

/**
 * @defgroup Button_Driver_Functions 按键驱动函数实现
 * @{
 */

/**
 * @brief 读取按键1的GPIO电平状态
 * @return rt_uint8_t 返回按键1的电平状态
 * @retval PIN_HIGH 高电平
 * @retval PIN_LOW  低电平
 * @note 该函数被按键驱动框架调用，用于获取按键的实时状态
 */
static rt_uint8_t key1_read_level(void)
{
    return rt_pin_read(KEY1_PIN);
}

/**
 * @brief 读取按键2的GPIO电平状态
 * @return rt_uint8_t 返回按键2的电平状态
 * @retval PIN_HIGH 高电平
 * @retval PIN_LOW  低电平
 * @note 该函数被按键驱动框架调用，用于获取按键的实时状态
 */
static rt_uint8_t key2_read_level(void)
{
    return rt_pin_read(KEY2_PIN);
}

/**
 * @brief 读取按键3的GPIO电平状态
 * @return rt_uint8_t 返回按键3的电平状态
 * @retval PIN_HIGH 高电平
 * @retval PIN_LOW  低电平
 * @note 该函数被按键驱动框架调用，用于获取按键的实时状态
 */
static rt_uint8_t key3_read_level(void)
{
    return rt_pin_read(KEY3_PIN);
}

/**
 * @brief 读取按键4的GPIO电平状态
 * @return rt_uint8_t 返回按键4的电平状态
 * @retval PIN_HIGH 高电平
 * @retval PIN_LOW  低电平
 * @note 该函数被按键驱动框架调用，用于获取按键的实时状态
 */
static rt_uint8_t key4_read_level(void)
{
    return rt_pin_read(KEY4_PIN);
}

/**
 * @brief 读取按键5的GPIO电平状态
 * @return rt_uint8_t 返回按键5的电平状态
 * @retval PIN_HIGH 高电平
 * @retval PIN_LOW  低电平
 * @note 该函数被按键驱动框架调用，用于获取按键的实时状态
 */
static rt_uint8_t key5_read_level(void)
{
    return rt_pin_read(KEY5_PIN);
}

/**
 * @brief 读取按键6的GPIO电平状态
 * @return rt_uint8_t 返回按键6的电平状态
 * @retval PIN_HIGH 高电平
 * @retval PIN_LOW  低电平
 * @note 该函数被按键驱动框架调用，用于获取按键的实时状态
 */
static rt_uint8_t key6_read_level(void)
{
    return rt_pin_read(KEY6_PIN);
}

/**
 * @brief 按键事件回调函数
 * @param btn 触发事件的按键实体指针
 * @note 该函数处理所有按键的事件回调，可根据需要自定义处理逻辑
 */
static void button_callback(void *btn)
{
    Button_t *button = (Button_t *)btn;
    rt_uint8_t event = Get_Button_Event(button);
    
    LOG_D("Button [%s] event: %d", button->Name, event);
    
    switch(event)
    {
        case BUTTON_DOWM:
            LOG_I("Button [%s] pressed", button->Name);
            break;
            
        case BUTTON_UP:
            LOG_I("Button [%s] released", button->Name);
            break;
            
        case BUTTON_DOUBLE:
            LOG_I("Button [%s] double clicked", button->Name);
            break;
            
        case BUTTON_LONG:
            LOG_I("Button [%s] long pressed", button->Name);
            break;
            
        case BUTTON_LONG_FREE:
            LOG_I("Button [%s] long press released", button->Name);
            break;
            
        case BUTTON_CONTINUOS:
            LOG_I("Button [%s] continuous pressed", button->Name);
            break;
            
        case BUTTON_CONTINUOS_FREE:
            LOG_I("Button [%s] continuous press released", button->Name);
            break;
            
        default:
            break;
    }
}

/**
 * @brief 按键驱动初始化函数
 * @return rt_err_t 初始化结果
 * @retval RT_EOK 初始化成功
 * @retval -RT_ERROR 初始化失败
 * @note 该函数配置所有按键的GPIO模式并创建按键实体
 */
rt_err_t drv_button_init(void)
{
    LOG_D("Button driver initializing...");
    
    /* 配置按键GPIO为输入上拉模式 */
    rt_pin_mode(KEY1_PIN, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(KEY2_PIN, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(KEY3_PIN, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(KEY4_PIN, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(KEY5_PIN, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(KEY6_PIN, PIN_MODE_INPUT_PULLUP);
    
    /* 创建按键实体 */
    Button_Create("key1", &key1_btn, key1_read_level, KEY_TRIGGER_LEVEL);
    Button_Create("key2", &key2_btn, key2_read_level, KEY_TRIGGER_LEVEL);
    Button_Create("key3", &key3_btn, key3_read_level, KEY_TRIGGER_LEVEL);
    Button_Create("key4", &key4_btn, key4_read_level, KEY_TRIGGER_LEVEL);
    Button_Create("key5", &key5_btn, key5_read_level, KEY_TRIGGER_LEVEL);
    Button_Create("key6", &key6_btn, key6_read_level, KEY_TRIGGER_LEVEL);
    
    /* 绑定按键事件回调函数 */
    Button_Attach(&key1_btn, BUTTON_ALL_RIGGER, button_callback);
    Button_Attach(&key2_btn, BUTTON_ALL_RIGGER, button_callback);
    Button_Attach(&key3_btn, BUTTON_ALL_RIGGER, button_callback);
    Button_Attach(&key4_btn, BUTTON_ALL_RIGGER, button_callback);
    Button_Attach(&key5_btn, BUTTON_ALL_RIGGER, button_callback);
    Button_Attach(&key6_btn, BUTTON_ALL_RIGGER, button_callback);
    
    LOG_I("Button driver initialized successfully");
    
    return RT_EOK;
}

/**
 * @brief 获取指定按键的实体指针
 * @param key_name 按键名称字符串
 * @return Button_t* 按键实体指针
 * @retval 非NULL 成功获取按键实体指针
 * @retval NULL 未找到对应的按键实体
 * @note 该函数用于根据按键名称获取对应的按键实体指针
 */
Button_t* drv_button_get_handle(const char *key_name)
{
    if (key_name == RT_NULL)
    {
        return RT_NULL;
    }

    if (rt_strcmp(key_name, "key1") == 0)
    {
        return &key1_btn;
    }
    else if (rt_strcmp(key_name, "key2") == 0)
    {
        return &key2_btn;
    }
    else if (rt_strcmp(key_name, "key3") == 0)
    {
        return &key3_btn;
    }
    else if (rt_strcmp(key_name, "key4") == 0)
    {
        return &key4_btn;
    }
    else if (rt_strcmp(key_name, "key5") == 0)
    {
        return &key5_btn;
    }
    else if (rt_strcmp(key_name, "key6") == 0)
    {
        return &key6_btn;
    }

    return RT_NULL;
}

/**
 * @brief 按键驱动反初始化函数
 * @return rt_err_t 反初始化结果
 * @retval RT_EOK 反初始化成功
 * @note 该函数删除所有按键实体并释放资源
 */
rt_err_t drv_button_deinit(void)
{
    LOG_D("Button driver deinitializing...");

    /* 删除按键实体 */
    Button_Delete(&key1_btn);
    Button_Delete(&key2_btn);
    Button_Delete(&key3_btn);
    Button_Delete(&key4_btn);
    Button_Delete(&key5_btn);
    Button_Delete(&key6_btn);

    LOG_I("Button driver deinitialized successfully");

    return RT_EOK;
}

/**
 * @brief 按键状态处理函数
 * @note 该函数需要在定时器或线程中周期性调用，建议调用周期为20-50ms
 * @warning 必须周期性调用此函数，否则按键事件无法正常检测
 */
void drv_button_process(void)
{
    Button_Process();
}

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
 */
rt_uint8_t drv_button_get_state(const char *key_name)
{
    Button_t *btn = drv_button_get_handle(key_name);
    if (btn != RT_NULL)
    {
        return Get_Button_State(btn);
    }
    return NONE_TRIGGER;
}

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
 */
rt_uint8_t drv_button_get_event(const char *key_name)
{
    Button_t *btn = drv_button_get_handle(key_name);
    if (btn != RT_NULL)
    {
        return Get_Button_Event(btn);
    }
    return NONE_TRIGGER;
}

/**
 * @brief 为指定按键绑定事件回调函数
 * @param key_name 按键名称字符串
 * @param event 按键事件类型
 * @param callback 回调函数指针
 * @return rt_err_t 绑定结果
 * @retval RT_EOK 绑定成功
 * @retval -RT_ERROR 绑定失败
 * @note 该函数用于为指定按键的特定事件绑定自定义回调函数
 */
rt_err_t drv_button_attach_callback(const char *key_name, Button_Event event, Button_CallBack callback)
{
    Button_t *btn = drv_button_get_handle(key_name);
    if (btn != RT_NULL && callback != RT_NULL)
    {
        Button_Attach(btn, event, callback);
        return RT_EOK;
    }
    return -RT_ERROR;
}

/**
 * @}
 */

#endif /* PKG_USING_BUTTON */
