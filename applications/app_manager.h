/**
 * @file app_manager.h
 * @brief 应用管理器头文件
 * @details 应用程序的顶层管理器，协调各个子模块的工作
 * @author RT-Thread Team
 * @date 2024-12-27
 * @version 1.0.0
 * 
 * @copyright Copyright (c) 2024 RT-Thread Development Team
 * 
 * @par 修改日志:
 * <table>
 * <tr><th>日期       <th>版本  <th>作者     <th>说明
 * <tr><td>2024-12-27 <td>1.0.0 <td>RT-Thread Team <td>创建应用管理器
 * </table>
 */

#ifndef __APP_MANAGER_H__
#define __APP_MANAGER_H__

#include <rtthread.h>
#include <rtdevice.h>
#include "system_state.h"
#include "config_manager.h"
#include "network_manager.h"
#include "key_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup App_Manager_Types 应用管理器类型定义
 * @{
 */

/**
 * @brief 应用管理器运行模式
 */
typedef enum
{
    APP_MODE_NORMAL = 0,                /**< 正常模式 */
    APP_MODE_LOW_POWER,                 /**< 低功耗模式 */
    APP_MODE_FACTORY_TEST,              /**< 工厂测试模式 */
    APP_MODE_DEBUG,                     /**< 调试模式 */
    APP_MODE_MAX
} app_mode_t;

/**
 * @brief 应用管理器状态信息
 */
typedef struct
{
    rt_bool_t initialized;              /**< 是否已初始化 */
    app_mode_t mode;                    /**< 运行模式 */
    rt_uint32_t uptime;                 /**< 运行时间(秒) */
    rt_uint32_t total_key_events;       /**< 总按键事件数 */
    rt_uint32_t total_network_joins;    /**< 总入网次数 */
    rt_uint32_t last_error_code;        /**< 最后错误码 */
    sys_state_t current_state;          /**< 当前系统状态 */
} app_manager_info_t;

/**
 * @}
 */

/**
 * @defgroup App_Manager_Functions 应用管理器函数
 * @{
 */

/**
 * @brief 初始化应用管理器
 * @return rt_err_t 初始化结果
 * @retval RT_EOK 初始化成功
 * @retval -RT_ERROR 初始化失败
 */
rt_err_t app_manager_init(void);

/**
 * @brief 反初始化应用管理器
 * @return rt_err_t 反初始化结果
 * @retval RT_EOK 反初始化成功
 */
rt_err_t app_manager_deinit(void);

/**
 * @brief 启动应用管理器
 * @return rt_err_t 启动结果
 * @retval RT_EOK 启动成功
 * @retval -RT_ERROR 启动失败
 */
rt_err_t app_manager_start(void);

/**
 * @brief 停止应用管理器
 * @return rt_err_t 停止结果
 * @retval RT_EOK 停止成功
 */
rt_err_t app_manager_stop(void);

/**
 * @brief 设置运行模式
 * @param mode 运行模式
 * @return rt_err_t 设置结果
 * @retval RT_EOK 设置成功
 * @retval -RT_EINVAL 模式无效
 */
rt_err_t app_manager_set_mode(app_mode_t mode);

/**
 * @brief 获取运行模式
 * @return app_mode_t 当前运行模式
 */
app_mode_t app_manager_get_mode(void);

/**
 * @brief 获取应用管理器信息
 * @param info 信息结构体指针
 * @return rt_err_t 获取结果
 * @retval RT_EOK 获取成功
 * @retval -RT_EINVAL 参数无效
 */
rt_err_t app_manager_get_info(app_manager_info_t *info);

/**
 * @brief 处理按键长按事件（用于入网控制）
 * @param key_id 按键ID
 * @return rt_err_t 处理结果
 * @retval RT_EOK 处理成功
 * @retval -RT_EINVAL 参数无效
 */
rt_err_t app_manager_handle_long_press(key_id_t key_id);

/**
 * @brief 重启系统
 * @param delay_ms 延时时间(毫秒)
 * @return rt_err_t 重启结果
 * @retval RT_EOK 重启开始
 */
rt_err_t app_manager_reboot(rt_uint32_t delay_ms);

/**
 * @brief 进入低功耗模式
 * @return rt_err_t 进入结果
 * @retval RT_EOK 进入成功
 * @retval -RT_ERROR 进入失败
 */
rt_err_t app_manager_enter_low_power(void);

/**
 * @brief 退出低功耗模式
 * @return rt_err_t 退出结果
 * @retval RT_EOK 退出成功
 */
rt_err_t app_manager_exit_low_power(void);

/**
 * @brief 获取运行模式字符串
 * @param mode 运行模式
 * @return const char* 模式字符串
 */
const char* app_mode_to_string(app_mode_t mode);

/**
 * @brief 打印系统信息
 */
void app_manager_print_info(void);

/**
 * @brief 执行自检
 * @return rt_err_t 自检结果
 * @retval RT_EOK 自检通过
 * @retval -RT_ERROR 自检失败
 */
rt_err_t app_manager_self_test(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __APP_MANAGER_H__ */
