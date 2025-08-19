/**
 * @file config_manager.h
 * @brief 配置管理器头文件
 * @details 提供系统配置的统一管理和掉电存储功能
 * @author RT-Thread Team
 * @date 2024-12-27
 * @version 1.0.0
 * 
 * @copyright Copyright (c) 2024 RT-Thread Development Team
 * 
 * @par 修改日志:
 * <table>
 * <tr><th>日期       <th>版本  <th>作者     <th>说明
 * <tr><td>2024-12-27 <td>1.0.0 <td>RT-Thread Team <td>创建配置管理器
 * </table>
 */

#ifndef __CONFIG_MANAGER_H__
#define __CONFIG_MANAGER_H__

#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup Config_Types 配置类型定义
 * @{
 */

/**
 * @brief 网络配置结构体
 */
typedef struct
{
    rt_uint32_t gateway_id;             /**< 网关ID */
    rt_uint32_t node_id;                /**< 节点ID */
    rt_uint8_t work_channel;            /**< 工作信道 */
    rt_uint8_t timeslot;                /**< 时隙 */
    rt_uint8_t beacon_channels[3];      /**< 信标信道 */
    rt_uint32_t join_timeout;           /**< 入网超时时间(ms) */
    rt_uint8_t max_join_retry;          /**< 最大入网重试次数 */
} network_config_t;

/**
 * @brief RF配置结构体
 */
typedef struct
{
    rt_uint16_t address_high;           /**< 地址高位 */
    rt_uint16_t address_low;            /**< 地址低位 */
    rt_uint8_t channel;                 /**< 信道 */
    rt_uint8_t power;                   /**< 发射功率 */
    rt_uint8_t rate;                    /**< 空中速率 */
    rt_bool_t encrypt_enabled;          /**< 是否启用加密 */
    rt_uint16_t encrypt_key0;           /**< 加密密钥0 */
    rt_uint16_t encrypt_key1;           /**< 加密密钥1 */
} rf_config_t;

/**
 * @brief 按键配置结构体
 */
typedef struct
{
    rt_uint32_t upload_timeout;         /**< 按键上传超时时间(ms) */
    rt_uint8_t max_upload_retry;        /**< 最大上传重试次数 */
    rt_uint32_t retry_interval;         /**< 重传间隔(ms) */
    rt_uint32_t initial_backoff;        /**< 初始退避窗口 */
    rt_uint32_t max_backoff;            /**< 最大退避窗口 */
} key_config_t;

/**
 * @brief 电源配置结构体
 */
typedef struct
{
    rt_uint32_t low_voltage_threshold;  /**< 低电压门限(mV) */
    rt_uint32_t normal_voltage_min;     /**< 正常电压最小值(mV) */
    rt_uint32_t battery_check_interval; /**< 电池检查间隔(ms) */
    rt_bool_t auto_sleep_enabled;       /**< 是否启用自动休眠 */
    rt_uint32_t idle_sleep_timeout;     /**< 空闲休眠超时(ms) */
} power_config_t;

/**
 * @brief 系统配置结构体
 */
typedef struct
{
    rt_uint32_t magic;                  /**< 配置魔数 */
    rt_uint32_t version;                /**< 配置版本 */
    rt_uint32_t crc32;                  /**< CRC32校验和 */
    network_config_t network;           /**< 网络配置 */
    rf_config_t rf;                     /**< RF配置 */
    key_config_t key;                   /**< 按键配置 */
    power_config_t power;               /**< 电源配置 */
    rt_uint8_t reserved[64];            /**< 保留字段 */
} system_config_t;

/**
 * @brief 配置项枚举
 */
typedef enum
{
    CONFIG_ITEM_NETWORK = 0,            /**< 网络配置 */
    CONFIG_ITEM_RF,                     /**< RF配置 */
    CONFIG_ITEM_KEY,                    /**< 按键配置 */
    CONFIG_ITEM_POWER,                  /**< 电源配置 */
    CONFIG_ITEM_ALL,                    /**< 所有配置 */
    CONFIG_ITEM_MAX
} config_item_t;

/**
 * @}
 */

/**
 * @defgroup Config_Constants 配置常量定义
 * @{
 */

/** @brief 配置魔数 */
#define CONFIG_MAGIC                    0x54524430  /* "TRD0" */

/** @brief 配置版本 */
#define CONFIG_VERSION                  0x00010000  /* v1.0.0 */

/** @brief 默认配置值 */
#define DEFAULT_NODE_ID                 0x12345678
#define DEFAULT_GATEWAY_ID              0x00000000
#define DEFAULT_WORK_CHANNEL            0
#define DEFAULT_TIMESLOT                1
#define DEFAULT_JOIN_TIMEOUT            30000
#define DEFAULT_MAX_JOIN_RETRY          3

#define DEFAULT_ADDR_HIGH               255
#define DEFAULT_ADDR_LOW                255
#define DEFAULT_RF_CHANNEL              0
#define DEFAULT_RF_POWER                0
#define DEFAULT_RF_RATE                 2           /* 250K */

#define DEFAULT_UPLOAD_TIMEOUT          3000
#define DEFAULT_MAX_UPLOAD_RETRY        8
#define DEFAULT_RETRY_INTERVAL          100
#define DEFAULT_INITIAL_BACKOFF         8
#define DEFAULT_MAX_BACKOFF             64

#define DEFAULT_LOW_VOLTAGE_THRESHOLD   2800
#define DEFAULT_NORMAL_VOLTAGE_MIN      3000
#define DEFAULT_BATTERY_CHECK_INTERVAL  60000
#define DEFAULT_IDLE_SLEEP_TIMEOUT      300000      /* 5分钟 */

/**
 * @}
 */

/**
 * @defgroup Config_Functions 配置管理函数
 * @{
 */

/**
 * @brief 初始化配置管理器
 * @return rt_err_t 初始化结果
 * @retval RT_EOK 初始化成功
 * @retval -RT_ERROR 初始化失败
 */
rt_err_t config_manager_init(void);

/**
 * @brief 反初始化配置管理器
 * @return rt_err_t 反初始化结果
 * @retval RT_EOK 反初始化成功
 */
rt_err_t config_manager_deinit(void);

/**
 * @brief 加载配置
 * @return rt_err_t 加载结果
 * @retval RT_EOK 加载成功
 * @retval -RT_ERROR 加载失败，使用默认配置
 */
rt_err_t config_load(void);

/**
 * @brief 保存配置
 * @return rt_err_t 保存结果
 * @retval RT_EOK 保存成功
 * @retval -RT_ERROR 保存失败
 */
rt_err_t config_save(void);

/**
 * @brief 恢复默认配置
 * @return rt_err_t 恢复结果
 * @retval RT_EOK 恢复成功
 */
rt_err_t config_restore_defaults(void);

/**
 * @brief 获取系统配置
 * @return const system_config_t* 配置指针
 * @note 返回只读配置，请勿直接修改
 */
const system_config_t* config_get_system(void);

/**
 * @brief 获取网络配置
 * @return const network_config_t* 网络配置指针
 */
const network_config_t* config_get_network(void);

/**
 * @brief 获取RF配置
 * @return const rf_config_t* RF配置指针
 */
const rf_config_t* config_get_rf(void);

/**
 * @brief 获取按键配置
 * @return const key_config_t* 按键配置指针
 */
const key_config_t* config_get_key(void);

/**
 * @brief 获取电源配置
 * @return const power_config_t* 电源配置指针
 */
const power_config_t* config_get_power(void);

/**
 * @brief 设置网络配置
 * @param config 网络配置
 * @return rt_err_t 设置结果
 * @retval RT_EOK 设置成功
 * @retval -RT_EINVAL 参数无效
 */
rt_err_t config_set_network(const network_config_t *config);

/**
 * @brief 设置RF配置
 * @param config RF配置
 * @return rt_err_t 设置结果
 * @retval RT_EOK 设置成功
 * @retval -RT_EINVAL 参数无效
 */
rt_err_t config_set_rf(const rf_config_t *config);

/**
 * @brief 设置按键配置
 * @param config 按键配置
 * @return rt_err_t 设置结果
 * @retval RT_EOK 设置成功
 * @retval -RT_EINVAL 参数无效
 */
rt_err_t config_set_key(const key_config_t *config);

/**
 * @brief 设置电源配置
 * @param config 电源配置
 * @return rt_err_t 设置结果
 * @retval RT_EOK 设置成功
 * @retval -RT_EINVAL 参数无效
 */
rt_err_t config_set_power(const power_config_t *config);

/**
 * @brief 验证配置有效性
 * @param item 配置项
 * @return rt_bool_t 是否有效
 * @retval RT_TRUE 配置有效
 * @retval RT_FALSE 配置无效
 */
rt_bool_t config_validate(config_item_t item);

/**
 * @brief 打印配置信息
 * @param item 配置项
 */
void config_print(config_item_t item);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __CONFIG_MANAGER_H__ */

