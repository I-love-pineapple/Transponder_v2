/**
 * @file key_manager.h
 * @brief 按键管理器头文件
 * @details 提供按键事件处理和数据上传的统一管理接口
 * @author RT-Thread Team
 * @date 2024-12-27
 * @version 1.0.0
 * 
 * @copyright Copyright (c) 2024 RT-Thread Development Team
 * 
 * @par 修改日志:
 * <table>
 * <tr><th>日期       <th>版本  <th>作者     <th>说明
 * <tr><td>2024-12-27 <td>1.0.0 <td>RT-Thread Team <td>创建按键管理器
 * </table>
 */

#ifndef __KEY_MANAGER_H__
#define __KEY_MANAGER_H__

#include <rtthread.h>
#include <rtdevice.h>
#include "config_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup Key_Types 按键类型定义
 * @{
 */

/**
 * @brief 按键事件枚举
 */
typedef enum
{
    KEY_EVENT_NONE = 0,                 /**< 无事件 */
    KEY_EVENT_PRESS,                    /**< 按下事件 */
    KEY_EVENT_RELEASE,                  /**< 释放事件 */
    KEY_EVENT_LONG_PRESS,               /**< 长按事件 */
    KEY_EVENT_DOUBLE_CLICK,             /**< 双击事件 */
    KEY_EVENT_MAX
} key_event_t;

/**
 * @brief 按键ID枚举
 */
typedef enum
{
    KEY_ID_1 = 1,                       /**< 按键1 */
    KEY_ID_2,                           /**< 按键2 */
    KEY_ID_3,                           /**< 按键3 */
    KEY_ID_4,                           /**< 按键4 */
    KEY_ID_5,                           /**< 按键5 */
    KEY_ID_6,                           /**< 按键6 */
    KEY_ID_MAX
} key_id_t;

/**
 * @brief 按键上传状态枚举
 */
typedef enum
{
    KEY_UPLOAD_IDLE = 0,                /**< 空闲状态 */
    KEY_UPLOAD_BUSY,                    /**< 上传中 */
    KEY_UPLOAD_SUCCESS,                 /**< 上传成功 */
    KEY_UPLOAD_FAILED,                  /**< 上传失败 */
    KEY_UPLOAD_TIMEOUT,                 /**< 上传超时 */
    KEY_UPLOAD_MAX
} key_upload_status_t;

/**
 * @brief 按键数据结构体
 */
typedef struct
{
    key_id_t key_id;                    /**< 按键ID */
    key_event_t event;                  /**< 按键事件 */
    rt_uint32_t timestamp;              /**< 时间戳 */
    rt_uint8_t battery_level;           /**< 电池电量 */
} key_data_t;

/**
 * @brief 按键上传信息结构体
 */
typedef struct
{
    key_upload_status_t status;         /**< 上传状态 */
    rt_uint16_t sequence;               /**< 序列号 */
    key_id_t key_id;                    /**< 按键ID */
    rt_uint8_t retry_count;             /**< 重试次数 */
    rt_uint32_t start_time;             /**< 开始时间 */
    rt_uint32_t backoff_window;         /**< 退避窗口 */
} key_upload_info_t;

/**
 * @brief 按键事件回调函数类型
 * @param data 按键数据
 * @param user_data 用户数据
 */
typedef void (*key_event_callback_t)(const key_data_t *data, void *user_data);

/**
 * @brief 按键上传状态回调函数类型
 * @param info 上传信息
 * @param user_data 用户数据
 */
typedef void (*key_upload_callback_t)(const key_upload_info_t *info, void *user_data);

/**
 * @}
 */

/**
 * @defgroup Key_Functions 按键管理函数
 * @{
 */

/**
 * @brief 初始化按键管理器
 * @return rt_err_t 初始化结果
 * @retval RT_EOK 初始化成功
 * @retval -RT_ERROR 初始化失败
 */
rt_err_t key_manager_init(void);

/**
 * @brief 反初始化按键管理器
 * @return rt_err_t 反初始化结果
 * @retval RT_EOK 反初始化成功
 */
rt_err_t key_manager_deinit(void);

/**
 * @brief 处理按键事件
 * @param key_id 按键ID
 * @param event 按键事件
 * @return rt_err_t 处理结果
 * @retval RT_EOK 处理成功
 * @retval -RT_EINVAL 参数无效
 * @retval -RT_ERROR 处理失败
 */
rt_err_t key_manager_handle_event(key_id_t key_id, key_event_t event);

/**
 * @brief 上传按键数据
 * @param key_id 按键ID
 * @return rt_err_t 上传结果
 * @retval RT_EOK 开始上传
 * @retval -RT_EBUSY 正在上传中
 * @retval -RT_ERROR 上传失败
 */
rt_err_t key_manager_upload(key_id_t key_id);

/**
 * @brief 获取上传状态
 * @return key_upload_status_t 上传状态
 */
key_upload_status_t key_manager_get_upload_status(void);

/**
 * @brief 获取上传信息
 * @param info 上传信息结构体指针
 * @return rt_err_t 获取结果
 * @retval RT_EOK 获取成功
 * @retval -RT_EINVAL 参数无效
 */
rt_err_t key_manager_get_upload_info(key_upload_info_t *info);

/**
 * @brief 停止当前上传
 * @return rt_err_t 停止结果
 * @retval RT_EOK 停止成功
 */
rt_err_t key_manager_stop_upload(void);

/**
 * @brief 处理上传应答
 * @param sequence 序列号
 * @param status 应答状态
 * @return rt_err_t 处理结果
 * @retval RT_EOK 处理成功
 * @retval -RT_EINVAL 参数无效
 */
rt_err_t key_manager_handle_upload_ack(rt_uint16_t sequence, rt_uint8_t status);

/**
 * @brief 注册按键事件回调函数
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return rt_err_t 注册结果
 * @retval RT_EOK 注册成功
 * @retval -RT_EINVAL 参数无效
 * @retval -RT_EFULL 回调函数已满
 */
rt_err_t key_manager_register_event_callback(key_event_callback_t callback, void *user_data);

/**
 * @brief 注册上传状态回调函数
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return rt_err_t 注册结果
 * @retval RT_EOK 注册成功
 * @retval -RT_EINVAL 参数无效
 * @retval -RT_EFULL 回调函数已满
 */
rt_err_t key_manager_register_upload_callback(key_upload_callback_t callback, void *user_data);

/**
 * @brief 注销按键事件回调函数
 * @param callback 回调函数
 * @return rt_err_t 注销结果
 * @retval RT_EOK 注销成功
 * @retval -RT_EINVAL 参数无效
 */
rt_err_t key_manager_unregister_event_callback(key_event_callback_t callback);

/**
 * @brief 注销上传状态回调函数
 * @param callback 回调函数
 * @return rt_err_t 注销结果
 * @retval RT_EOK 注销成功
 * @retval -RT_EINVAL 参数无效
 */
rt_err_t key_manager_unregister_upload_callback(key_upload_callback_t callback);

/**
 * @brief 获取按键事件字符串
 * @param event 按键事件
 * @return const char* 事件字符串
 */
const char* key_event_to_string(key_event_t event);

/**
 * @brief 获取按键ID字符串
 * @param key_id 按键ID
 * @return const char* 按键ID字符串
 */
const char* key_id_to_string(key_id_t key_id);

/**
 * @brief 获取上传状态字符串
 * @param status 上传状态
 * @return const char* 状态字符串
 */
const char* key_upload_status_to_string(key_upload_status_t status);

/**
 * @brief 获取统计信息
 * @param total_uploads 总上传次数
 * @param success_uploads 成功上传次数
 * @param failed_uploads 失败上传次数
 * @return rt_err_t 获取结果
 * @retval RT_EOK 获取成功
 */
rt_err_t key_manager_get_statistics(rt_uint32_t *total_uploads, rt_uint32_t *success_uploads, rt_uint32_t *failed_uploads);

/**
 * @brief 重置统计信息
 * @return rt_err_t 重置结果
 * @retval RT_EOK 重置成功
 */
rt_err_t key_manager_reset_statistics(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __KEY_MANAGER_H__ */

