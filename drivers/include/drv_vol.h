/**
 * @file drv_vol.h
 * @brief 电池电压检测驱动头文件
 * @details 定义电池电压检测驱动的接口函数、宏定义和数据结构
 * @author RT-Thread Team
 * @date 2024-08-09
 * @version 1.0.0
 * 
 * @copyright Copyright (c) 2024 RT-Thread Development Team
 * 
 * @par 修改日志:
 * <table>
 * <tr><th>日期       <th>版本  <th>作者     <th>说明
 * <tr><td>2024-08-09 <td>1.0.0 <td>RT-Thread Team <td>首次创建
 * </table>
 */

#ifndef __DRV_VOL_H__
#define __DRV_VOL_H__

#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup Voltage_Driver_Exported_Types 电压驱动导出类型定义
 * @{
 */

/**
 * @brief 电压检测状态枚举定义
 * @note 用于表示电压检测的状态
 */
typedef enum
{
    VOL_STATUS_DISABLED = 0,    /**< 电压检测禁用 */
    VOL_STATUS_ENABLED,         /**< 电压检测使能 */
    VOL_STATUS_ERROR            /**< 电压检测错误 */
} vol_status_t;

/**
 * @brief 电池电压信息结构体定义
 * @note 用于存储电池电压的详细信息
 */
typedef struct
{
    rt_uint32_t raw_value;      /**< ADC原始采样值 (0-4095) */
    rt_uint32_t voltage_mv;     /**< 电压值，单位：毫伏 (mV) */
    rt_uint32_t voltage_v;      /**< 电压值整数部分，单位：伏 (V) */
    rt_uint32_t voltage_frac;   /**< 电压值小数部分 (百分之一伏) */
    vol_status_t status;        /**< 检测状态 */
} battery_voltage_t;

/**
 * @}
 */

/**
 * @defgroup Voltage_Driver_Exported_Macros 电压驱动导出宏定义
 * @{
 */

/** @brief ADC设备相关宏定义 */
#define VOL_ADC_DEV_NAME        "adc1"          /**< ADC设备名称 */
#define VOL_ADC_CHANNEL         1               /**< ADC通道号 */
#define VOL_REFER_VOLTAGE       3300            /**< 参考电压 3.3V，单位：mV */
#define VOL_CONVERT_BITS        (1 << 12)       /**< ADC转换位数 12位 */

/** @brief 电压检测引脚定义 */
#define VOL_ENABLE_PIN          GET_PIN(B, 8)   /**< 电池电压检测使能引脚 PB8 */

/** @brief 电压检测控制宏定义 */
#define VOL_ENABLE_LEVEL        PIN_HIGH        /**< 电压检测使能电平 */
#define VOL_DISABLE_LEVEL       PIN_LOW         /**< 电压检测禁用电平 */

/** @brief 电压阈值定义 (单位：mV) */
#define VOL_BATTERY_MIN         2000            /**< 电池最低电压阈值 2.0V */
#define VOL_BATTERY_MAX         4200            /**< 电池最高电压阈值 4.2V */
#define VOL_BATTERY_LOW         3000            /**< 电池低电压警告阈值 3.0V */
#define VOL_BATTERY_NORMAL      3600            /**< 电池正常电压阈值 3.6V */

/**
 * @}
 */

/**
 * @defgroup Voltage_Driver_Exported_Functions 电压驱动导出函数
 * @{
 */

/**
 * @brief 电池电压驱动初始化函数
 * @return rt_err_t 初始化结果
 * @retval RT_EOK 初始化成功
 * @retval -RT_ERROR 初始化失败
 * @note 该函数初始化ADC设备和电压检测使能引脚
 * 
 * @par 示例:
 * @code
 * rt_err_t result = drv_vol_init();
 * if (result == RT_EOK)
 * {
 *     rt_kprintf("Battery voltage driver initialized successfully\n");
 * }
 * @endcode
 */
rt_err_t drv_vol_init(void);

/**
 * @brief 电池电压驱动反初始化函数
 * @return rt_err_t 反初始化结果
 * @retval RT_EOK 反初始化成功
 * @note 该函数释放ADC设备资源并禁用电压检测
 * 
 * @par 示例:
 * @code
 * rt_err_t result = drv_vol_deinit();
 * if (result == RT_EOK)
 * {
 *     rt_kprintf("Battery voltage driver deinitialized successfully\n");
 * }
 * @endcode
 */
rt_err_t drv_vol_deinit(void);

/**
 * @brief 使能电池电压检测
 * @return rt_err_t 操作结果
 * @retval RT_EOK 使能成功
 * @retval -RT_ERROR 使能失败
 * @note 该函数使能电池电压检测电路
 * 
 * @par 示例:
 * @code
 * rt_err_t result = drv_vol_enable();
 * if (result == RT_EOK)
 * {
 *     rt_kprintf("Battery voltage detection enabled\n");
 * }
 * @endcode
 */
rt_err_t drv_vol_enable(void);

/**
 * @brief 禁用电池电压检测
 * @return rt_err_t 操作结果
 * @retval RT_EOK 禁用成功
 * @note 该函数禁用电池电压检测电路以节省功耗
 * 
 * @par 示例:
 * @code
 * drv_vol_disable();
 * @endcode
 */
rt_err_t drv_vol_disable(void);

/**
 * @brief 读取电池电压原始ADC值
 * @param raw_value 用于存储ADC原始值的指针
 * @return rt_err_t 读取结果
 * @retval RT_EOK 读取成功
 * @retval -RT_EINVAL 参数无效
 * @retval -RT_ERROR 读取失败
 * @note 该函数读取ADC原始采样值 (0-4095)
 * 
 * @par 示例:
 * @code
 * rt_uint32_t raw_value;
 * rt_err_t result = drv_vol_read_raw(&raw_value);
 * if (result == RT_EOK)
 * {
 *     rt_kprintf("ADC raw value: %d\n", raw_value);
 * }
 * @endcode
 */
rt_err_t drv_vol_read_raw(rt_uint32_t *raw_value);

/**
 * @brief 读取电池电压值
 * @param voltage_mv 用于存储电压值的指针，单位：毫伏
 * @return rt_err_t 读取结果
 * @retval RT_EOK 读取成功
 * @retval -RT_EINVAL 参数无效
 * @retval -RT_ERROR 读取失败
 * @note 该函数读取并转换电池电压值，单位为毫伏
 * 
 * @par 示例:
 * @code
 * rt_uint32_t voltage_mv;
 * rt_err_t result = drv_vol_read_voltage(&voltage_mv);
 * if (result == RT_EOK)
 * {
 *     rt_kprintf("Battery voltage: %d.%02d V\n", voltage_mv / 1000, (voltage_mv % 1000) / 10);
 * }
 * @endcode
 */
rt_err_t drv_vol_read_voltage(rt_uint32_t *voltage_mv);

/**
 * @brief 读取完整的电池电压信息
 * @param bat_vol 用于存储电池电压信息的结构体指针
 * @return rt_err_t 读取结果
 * @retval RT_EOK 读取成功
 * @retval -RT_EINVAL 参数无效
 * @retval -RT_ERROR 读取失败
 * @note 该函数读取完整的电池电压信息，包括原始值、电压值和状态
 * 
 * @par 示例:
 * @code
 * battery_voltage_t bat_vol;
 * rt_err_t result = drv_vol_read_info(&bat_vol);
 * if (result == RT_EOK)
 * {
 *     rt_kprintf("Battery: %d.%02d V (Raw: %d)\n", 
 *                bat_vol.voltage_v, bat_vol.voltage_frac, bat_vol.raw_value);
 * }
 * @endcode
 */
rt_err_t drv_vol_read_info(battery_voltage_t *bat_vol);

/**
 * @brief 获取电池电压检测状态
 * @return vol_status_t 检测状态
 * @retval VOL_STATUS_ENABLED 检测已使能
 * @retval VOL_STATUS_DISABLED 检测已禁用
 * @retval VOL_STATUS_ERROR 检测错误
 * @note 该函数获取当前电池电压检测的状态
 * 
 * @par 示例:
 * @code
 * vol_status_t status = drv_vol_get_status();
 * if (status == VOL_STATUS_ENABLED)
 * {
 *     rt_kprintf("Voltage detection is enabled\n");
 * }
 * @endcode
 */
vol_status_t drv_vol_get_status(void);

/**
 * @}
 */

/**
 * @defgroup Voltage_Driver_Utility_Functions 电压驱动工具函数
 * @{
 */

/**
 * @brief 判断电池电压是否在正常范围内
 * @param voltage_mv 电压值，单位：毫伏
 * @return rt_bool_t 判断结果
 * @retval RT_TRUE 电压正常
 * @retval RT_FALSE 电压异常
 * @note 该函数根据预设阈值判断电池电压是否正常
 *
 * @par 示例:
 * @code
 * rt_uint32_t voltage_mv = 3700;
 * if (drv_vol_is_voltage_normal(voltage_mv))
 * {
 *     rt_kprintf("Battery voltage is normal\n");
 * }
 * @endcode
 */
rt_bool_t drv_vol_is_voltage_normal(rt_uint32_t voltage_mv);

/**
 * @brief 判断电池电压是否为低电压
 * @param voltage_mv 电压值，单位：毫伏
 * @return rt_bool_t 判断结果
 * @retval RT_TRUE 低电压
 * @retval RT_FALSE 非低电压
 * @note 该函数根据低电压阈值判断电池是否需要充电
 *
 * @par 示例:
 * @code
 * rt_uint32_t voltage_mv = 2800;
 * if (drv_vol_is_voltage_low(voltage_mv))
 * {
 *     rt_kprintf("Battery voltage is low, please charge\n");
 * }
 * @endcode
 */
rt_bool_t drv_vol_is_voltage_low(rt_uint32_t voltage_mv);

/**
 * @brief 获取电池电压等级描述字符串
 * @param voltage_mv 电压值，单位：毫伏
 * @return const char* 电压等级描述
 * @note 该函数返回电池电压等级的文字描述
 *
 * @par 示例:
 * @code
 * rt_uint32_t voltage_mv = 3700;
 * const char* level = drv_vol_get_voltage_level_string(voltage_mv);
 * rt_kprintf("Battery level: %s\n", level);
 * @endcode
 */
const char* drv_vol_get_voltage_level_string(rt_uint32_t voltage_mv);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __DRV_VOL_H__ */
