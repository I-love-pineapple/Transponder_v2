/**
 * @file e35_module.h
 * @brief E35模块头文件
 * @details 定义E35模块的公共接口和数据结构
 * @author RT-Thread Team
 * @date 2024-08-18
 * @version 2.0.0
 * 
 * @copyright Copyright (c) 2024 RT-Thread Development Team
 * 
 * @par 修改日志:
 * <table>
 * <tr><th>日期       <th>版本  <th>作者     <th>说明
 * <tr><td>2024-08-18 <td>2.0.0 <td>RT-Thread Team <td>创建E35模块头文件
 * </table>
 */

#ifndef __E35_MODULE_H__
#define __E35_MODULE_H__

#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup E35_Exported_Types E35模块导出类型定义
 * @{
 */

/**
 * @brief 入网状态枚举定义
 * @note 定义设备的入网状态
 */
typedef enum {
    JOIN_STATUS_NONE = 0,       /**< 不入网 */
    JOIN_STATUS_JOINING,        /**< 入网中 */
    JOIN_STATUS_JOINED,         /**< 已入网 */
} join_status_t;

/**
 * @brief E35模块配置结构体
 * @note 用于管理E35模块的配置参数
 */
typedef struct
{
    rt_uint16_t address_high;   /**< 地址高位 */
    rt_uint16_t address_low;    /**< 地址低位 */
    rt_uint8_t channel;         /**< 工作信道 */
    rt_uint8_t power;           /**< 发射功率 */
    rt_uint8_t rate;            /**< 空中速率 */
    rt_bool_t encrypt_enabled;  /**< 是否启用加密 */
    rt_uint16_t encrypt_key0;   /**< 加密密钥0 */
    rt_uint16_t encrypt_key1;   /**< 加密密钥1 */
} e35_config_t;

/**
 * @brief E35模块状态结构体
 * @note 用于管理E35模块的运行状态
 */
typedef struct
{
    rt_bool_t initialized;      /**< 初始化标志 */
    rt_bool_t in_trans_mode;    /**< 是否在透传模式 */
    e35_config_t config;        /**< 模块配置 */
    rt_uint32_t gateway_id;     /**< 网关ID */
    rt_uint32_t node_id;        /**< 节点ID */
    rt_uint8_t work_channel;    /**< 工作信道 */
    rt_uint8_t timeslot;        /**< 时隙 */
    rt_uint8_t rssi;            /**< 信号强度 */
    join_status_t join_status;  /**< 入网状态 */
    rt_uint8_t ch_switch;       /**< 信道切换标志 */
} e35_module_state_t;

/**
 * @}
 */

/**
 * @defgroup E35_Exported_Functions E35模块导出函数
 * @{
 */

/**
 * @brief RF模块初始化函数
 * @return int 初始化结果
 * @retval 0 初始化成功
 * @retval -1 初始化失败
 * @note 该函数初始化RF通信模块的所有组件
 */
int rf_init(void);

/**
 * @brief RF模块反初始化函数
 * @return int 反初始化结果
 * @retval 0 反初始化成功
 * @note 该函数清理RF通信模块的所有资源
 */
int rf_deinit(void);

/**
 * @brief 上传按键数据
 * @param sequence 序列号
 * @param option 按键选项
 * @param battery 电池电量百分比
 * @return rt_err_t 上传结果
 * @retval RT_EOK 上传成功
 * @retval -RT_ERROR 上传失败
 * @retval -RT_EINVAL 参数无效
 * @note 该函数创建并发送按键上传请求帧
 */
rt_err_t press_upload(uint16_t sequence, uint8_t option, uint8_t battery);

/**
 * @brief 获取E35模块状态
 * @param state 状态结构体指针
 * @return rt_err_t 获取结果
 * @retval RT_EOK 获取成功
 * @retval -RT_EINVAL 参数无效
 * @note 该函数获取E35模块的当前状态信息
 */
rt_err_t e35_get_module_state(e35_module_state_t *state);

/**
 * @brief 设置入网状态
 * @param status 入网状态
 * @note 该函数设置E35模块的入网状态，供外部调用
 */
void e35_set_join_status(join_status_t status);

/**
 * @brief 获取入网状态
 * @return join_status_t 当前入网状态
 * @note 该函数获取E35模块的当前入网状态
 */
join_status_t e35_get_join_status(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __E35_MODULE_H__ */
