/**
 * @file network_manager.h
 * @brief 入网管理器头文件
 * @details 提供设备入网功能的统一管理接口
 * @author RT-Thread Team
 * @date 2024-12-27
 * @version 1.0.0
 * 
 * @copyright Copyright (c) 2024 RT-Thread Development Team
 * 
 * @par 修改日志:
 * <table>
 * <tr><th>日期       <th>版本  <th>作者     <th>说明
 * <tr><td>2024-12-27 <td>1.0.0 <td>RT-Thread Team <td>创建入网管理器
 * </table>
 */

#ifndef __NETWORK_MANAGER_H__
#define __NETWORK_MANAGER_H__

#include <rtthread.h>
#include <rtdevice.h>
#include "config_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup Network_Types 入网类型定义
 * @{
 */

/**
 * @brief 入网状态枚举
 */
typedef enum
{
    NET_STATUS_NONE = 0,                /**< 未入网 */
    NET_STATUS_JOINING,                 /**< 入网中 */
    NET_STATUS_JOINED,                  /**< 已入网 */
    NET_STATUS_FAILED,                  /**< 入网失败 */
    NET_STATUS_TIMEOUT,                 /**< 入网超时 */
    NET_STATUS_MAX
} net_status_t;

/**
 * @brief 入网信息结构体
 */
typedef struct
{
    net_status_t status;                /**< 入网状态 */
    rt_uint32_t gateway_id;             /**< 网关ID */
    rt_uint32_t node_id;                /**< 节点ID */
    rt_uint8_t work_channel;            /**< 工作信道 */
    rt_uint8_t timeslot;                /**< 时隙 */
    rt_uint8_t rssi;                    /**< 信号强度 */
    rt_uint32_t join_time;              /**< 入网时间戳 */
    rt_uint8_t retry_count;             /**< 重试次数 */
} network_info_t;

/**
 * @brief 入网事件回调函数类型
 * @param event 入网事件
 * @param info 入网信息
 * @param user_data 用户数据
 */
typedef void (*network_event_callback_t)(net_status_t event, const network_info_t *info, void *user_data);

/**
 * @}
 */

/**
 * @defgroup Network_Functions 入网管理函数
 * @{
 */

/**
 * @brief 初始化入网管理器
 * @return rt_err_t 初始化结果
 * @retval RT_EOK 初始化成功
 * @retval -RT_ERROR 初始化失败
 */
rt_err_t network_manager_init(void);

/**
 * @brief 反初始化入网管理器
 * @return rt_err_t 反初始化结果
 * @retval RT_EOK 反初始化成功
 */
rt_err_t network_manager_deinit(void);

/**
 * @brief 开始入网流程
 * @return rt_err_t 开始结果
 * @retval RT_EOK 开始成功
 * @retval -RT_EBUSY 已在入网中
 * @retval -RT_ERROR 开始失败
 */
rt_err_t network_start_join(void);

/**
 * @brief 停止入网流程
 * @return rt_err_t 停止结果
 * @retval RT_EOK 停止成功
 */
rt_err_t network_stop_join(void);

/**
 * @brief 离开网络
 * @return rt_err_t 离网结果
 * @retval RT_EOK 离网成功
 */
rt_err_t network_leave(void);

/**
 * @brief 获取入网状态
 * @return net_status_t 入网状态
 */
net_status_t network_get_status(void);

/**
 * @brief 获取入网信息
 * @param info 入网信息结构体指针
 * @return rt_err_t 获取结果
 * @retval RT_EOK 获取成功
 * @retval -RT_EINVAL 参数无效
 */
rt_err_t network_get_info(network_info_t *info);

/**
 * @brief 检查是否已入网
 * @return rt_bool_t 是否已入网
 * @retval RT_TRUE 已入网
 * @retval RT_FALSE 未入网
 */
rt_bool_t network_is_joined(void);

/**
 * @brief 注册入网事件回调函数
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return rt_err_t 注册结果
 * @retval RT_EOK 注册成功
 * @retval -RT_EINVAL 参数无效
 * @retval -RT_EFULL 回调函数已满
 */
rt_err_t network_register_callback(network_event_callback_t callback, void *user_data);

/**
 * @brief 注销入网事件回调函数
 * @param callback 回调函数
 * @return rt_err_t 注销结果
 * @retval RT_EOK 注销成功
 * @retval -RT_EINVAL 参数无效
 */
rt_err_t network_unregister_callback(network_event_callback_t callback);

/**
 * @brief 获取入网状态字符串
 * @param status 入网状态
 * @return const char* 状态字符串
 */
const char* network_status_to_string(net_status_t status);

/**
 * @brief 设置节点ID
 * @param node_id 节点ID
 * @return rt_err_t 设置结果
 * @retval RT_EOK 设置成功
 * @retval -RT_EINVAL 参数无效
 */
rt_err_t network_set_node_id(rt_uint32_t node_id);

/**
 * @brief 获取节点ID
 * @return rt_uint32_t 节点ID
 */
rt_uint32_t network_get_node_id(void);

/**
 * @brief 强制重新入网
 * @return rt_err_t 重新入网结果
 * @retval RT_EOK 开始重新入网
 * @retval -RT_ERROR 开始失败
 */
rt_err_t network_rejoin(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __NETWORK_MANAGER_H__ */

