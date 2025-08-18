/**
 * @file applications.h
 * @brief 应用程序头文件
 * @details 定义应用程序的公共接口和数据结构
 * @author RT-Thread Team
 * @date 2024-08-18
 * @version 2.0.0
 * 
 * @copyright Copyright (c) 2024 RT-Thread Development Team
 * 
 * @par 修改日志:
 * <table>
 * <tr><th>日期       <th>版本  <th>作者     <th>说明
 * <tr><td>2024-08-18 <td>2.0.0 <td>RT-Thread Team <td>创建应用程序头文件
 * </table>
 */

#ifndef __APPLICATIONS_H__
#define __APPLICATIONS_H__

#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup App_Exported_Types 应用程序导出类型定义
 * @{
 */

/**
 * @brief 系统状态枚举定义
 * @note 定义系统的各种运行状态
 */
typedef enum
{
    SYS_STATE_NOT_JOINED = 0,       /**< 未入网状态 */
    SYS_STATE_JOINING,              /**< 入网中状态 */
    SYS_STATE_JOINED,               /**< 已入网状态 */
    SYS_STATE_KEY_UPLOADING,        /**< 上传按键中状态 */
    SYS_STATE_KEY_UPLOAD_DONE,      /**< 按键上传完成状态 */
    SYS_STATE_SLEEP,                /**< 休眠状态 */
    SYS_STATE_MAX                   /**< 状态数量 */
} sys_state_t;

/**
 * @}
 */

/**
 * @defgroup App_Exported_Functions 应用程序导出函数
 * @{
 */

/**
 * @brief 应用程序初始化函数
 * @return rt_err_t 初始化结果
 * @retval RT_EOK 初始化成功
 * @retval -RT_ERROR 初始化失败
 * @note 该函数初始化应用程序的所有资源和线程
 */
rt_err_t app_init(void);

/**
 * @brief 注册按键回调函数
 * @return rt_err_t 注册结果
 * @retval RT_EOK 注册成功
 * @retval -RT_ERROR 注册失败
 * @note 该函数为所有按键注册统一的回调函数
 */
rt_err_t app_register_button_callbacks(void);

/**
 * @brief 处理PRESS_ACK帧应答
 * @note 该函数应在e35-2g4t.c中收到PRESS_ACK帧时被调用
 */
void app_handle_press_ack(void);

/**
 * @brief 获取当前系统状态
 * @return sys_state_t 当前系统状态
 * @note 该函数线程安全地获取当前系统状态
 */
sys_state_t app_get_system_state(void);

/**
 * @brief 应用程序反初始化函数
 * @return rt_err_t 反初始化结果
 * @retval RT_EOK 反初始化成功
 * @note 该函数清理应用程序的所有资源
 */
rt_err_t app_deinit(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __APPLICATIONS_H__ */
