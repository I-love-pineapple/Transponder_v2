/**
 * @file system_state.h
 * @brief 系统状态管理器头文件
 * @details 提供系统状态机的统一管理接口
 * @author RT-Thread Team
 * @date 2024-12-27
 * @version 1.0.0
 * 
 * @copyright Copyright (c) 2024 RT-Thread Development Team
 * 
 * @par 修改日志:
 * <table>
 * <tr><th>日期       <th>版本  <th>作者     <th>说明
 * <tr><td>2024-12-27 <td>1.0.0 <td>RT-Thread Team <td>创建系统状态管理器
 * </table>
 */

#ifndef __SYSTEM_STATE_H__
#define __SYSTEM_STATE_H__

#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup System_State_Types 系统状态类型定义
 * @{
 */

/**
 * @brief 系统状态枚举定义
 * @note 定义系统的各种运行状态
 */
typedef enum
{
    SYS_STATE_INIT = 0,                 /**< 初始化状态 */
    SYS_STATE_NOT_JOINED,               /**< 未入网状态 */
    SYS_STATE_JOINING,                  /**< 入网中状态 */
    SYS_STATE_JOINED,                   /**< 已入网状态 */
    SYS_STATE_KEY_UPLOADING,            /**< 上传按键中状态 */
    SYS_STATE_KEY_UPLOAD_DONE,          /**< 按键上传完成状态 */
    SYS_STATE_SLEEP,                    /**< 休眠状态 */
    SYS_STATE_ERROR,                    /**< 错误状态 */
    SYS_STATE_MAX                       /**< 状态数量 */
} sys_state_t;

/**
 * @brief 状态变化事件枚举
 */
typedef enum
{
    SYS_EVENT_INIT_COMPLETE = 0,        /**< 初始化完成事件 */
    SYS_EVENT_JOIN_REQUEST,             /**< 入网请求事件 */
    SYS_EVENT_JOIN_SUCCESS,             /**< 入网成功事件 */
    SYS_EVENT_JOIN_FAILED,              /**< 入网失败事件 */
    SYS_EVENT_LEAVE_NETWORK,            /**< 离网事件 */
    SYS_EVENT_KEY_PRESS,                /**< 按键按下事件 */
    SYS_EVENT_KEY_UPLOAD_START,         /**< 按键上传开始事件 */
    SYS_EVENT_KEY_UPLOAD_SUCCESS,       /**< 按键上传成功事件 */
    SYS_EVENT_KEY_UPLOAD_FAILED,        /**< 按键上传失败事件 */
    SYS_EVENT_ENTER_SLEEP,              /**< 进入休眠事件 */
    SYS_EVENT_WAKEUP,                   /**< 唤醒事件 */
    SYS_EVENT_ERROR,                    /**< 错误事件 */
    SYS_EVENT_MAX                       /**< 事件数量 */
} sys_event_t;

/**
 * @brief 状态变化回调函数类型
 * @param old_state 旧状态
 * @param new_state 新状态
 * @param user_data 用户数据
 */
typedef void (*state_change_callback_t)(sys_state_t old_state, sys_state_t new_state, void *user_data);

/**
 * @}
 */

/**
 * @defgroup System_State_Functions 系统状态管理函数
 * @{
 */

/**
 * @brief 初始化系统状态管理器
 * @return rt_err_t 初始化结果
 * @retval RT_EOK 初始化成功
 * @retval -RT_ERROR 初始化失败
 */
rt_err_t system_state_init(void);

/**
 * @brief 反初始化系统状态管理器
 * @return rt_err_t 反初始化结果
 * @retval RT_EOK 反初始化成功
 */
rt_err_t system_state_deinit(void);

/**
 * @brief 获取当前系统状态
 * @return sys_state_t 当前系统状态
 */
sys_state_t system_state_get(void);

/**
 * @brief 设置系统状态
 * @param new_state 新的系统状态
 * @return rt_err_t 设置结果
 * @retval RT_EOK 设置成功
 * @retval -RT_EINVAL 状态无效
 * @retval -RT_ERROR 状态转换失败
 * @note 该函数会检查状态转换的合法性
 */
rt_err_t system_state_set(sys_state_t new_state);

/**
 * @brief 发送系统事件
 * @param event 系统事件
 * @param data 事件数据（可选）
 * @return rt_err_t 发送结果
 * @retval RT_EOK 发送成功
 * @retval -RT_EINVAL 事件无效
 * @retval -RT_ERROR 事件处理失败
 */
rt_err_t system_state_post_event(sys_event_t event, void *data);

/**
 * @brief 注册状态变化回调函数
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return rt_err_t 注册结果
 * @retval RT_EOK 注册成功
 * @retval -RT_EINVAL 参数无效
 * @retval -RT_EFULL 回调函数已满
 */
rt_err_t system_state_register_callback(state_change_callback_t callback, void *user_data);

/**
 * @brief 注销状态变化回调函数
 * @param callback 回调函数
 * @return rt_err_t 注销结果
 * @retval RT_EOK 注销成功
 * @retval -RT_EINVAL 参数无效
 */
rt_err_t system_state_unregister_callback(state_change_callback_t callback);

/**
 * @brief 检查状态转换是否合法
 * @param from 源状态
 * @param to 目标状态
 * @return rt_bool_t 是否合法
 * @retval RT_TRUE 转换合法
 * @retval RT_FALSE 转换非法
 */
rt_bool_t system_state_is_transition_valid(sys_state_t from, sys_state_t to);

/**
 * @brief 获取状态字符串
 * @param state 系统状态
 * @return const char* 状态字符串
 */
const char* system_state_to_string(sys_state_t state);

/**
 * @brief 获取事件字符串
 * @param event 系统事件
 * @return const char* 事件字符串
 */
const char* system_event_to_string(sys_event_t event);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_STATE_H__ */

