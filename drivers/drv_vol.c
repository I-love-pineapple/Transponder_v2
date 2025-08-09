/**
 * @file drv_vol.c
 * @brief 电池电压检测驱动实现文件
 * @details 实现电池电压检测的控制功能，支持电压读取、使能控制和状态管理
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

#include "drv_vol.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <drv_common.h>

#define DBG_TAG "drv.vol"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/**
 * @defgroup Voltage_Driver_Private_Variables 电压驱动私有变量
 * @{
 */

/** @brief ADC设备句柄 */
static rt_adc_device_t adc_dev = RT_NULL;

/** @brief 电压检测状态 */
static vol_status_t vol_detection_status = VOL_STATUS_DISABLED;

/** @brief 驱动初始化标志 */
static rt_bool_t vol_driver_initialized = RT_FALSE;

/**
 * @}
 */

/**
 * @defgroup Voltage_Driver_Private_Functions 电压驱动私有函数
 * @{
 */

/**
 * @brief 检查驱动是否已初始化
 * @return rt_bool_t 检查结果
 * @retval RT_TRUE 驱动已初始化
 * @retval RT_FALSE 驱动未初始化
 * @note 该函数用于检查驱动是否已正确初始化
 */
static rt_bool_t vol_driver_is_initialized(void)
{
    return vol_driver_initialized;
}

/**
 * @brief 设置电压检测使能引脚状态
 * @param level 引脚电平状态
 * @note 该函数控制电池电压检测电路的使能状态
 */
static void vol_set_enable_pin(rt_base_t level)
{
    rt_pin_write(VOL_ENABLE_PIN, level);
}

/**
 * @brief 将ADC原始值转换为电压值
 * @param raw_value ADC原始采样值
 * @return rt_uint32_t 转换后的电压值，单位：毫伏
 * @note 该函数根据参考电压和ADC位数计算实际电压值
 */
static rt_uint32_t vol_convert_raw_to_mv(rt_uint32_t raw_value)
{
    return (raw_value * VOL_REFER_VOLTAGE) / VOL_CONVERT_BITS;
}

/**
 * @}
 */

/**
 * @defgroup Voltage_Driver_Public_Functions 电压驱动公共函数
 * @{
 */

/**
 * @brief 电池电压驱动初始化函数
 * @return rt_err_t 初始化结果
 * @retval RT_EOK 初始化成功
 * @retval -RT_ERROR 初始化失败
 * @note 该函数初始化ADC设备和电压检测使能引脚
 */
rt_err_t drv_vol_init(void)
{
    LOG_D("Battery voltage driver initializing...");
    
    /* 检查是否已经初始化 */
    if (vol_driver_initialized)
    {
        LOG_W("Battery voltage driver already initialized");
        return RT_EOK;
    }
    
    /* 查找ADC设备 */
    adc_dev = (rt_adc_device_t)rt_device_find(VOL_ADC_DEV_NAME);
    if (adc_dev == RT_NULL)
    {
        LOG_E("Cannot find ADC device: %s", VOL_ADC_DEV_NAME);
        return -RT_ERROR;
    }
    
    /* 配置电压检测使能引脚 */
    rt_pin_mode(VOL_ENABLE_PIN, PIN_MODE_OUTPUT);
    vol_set_enable_pin(VOL_DISABLE_LEVEL);  /* 默认禁用电压检测 */
    
    /* 设置初始状态 */
    vol_detection_status = VOL_STATUS_DISABLED;
    vol_driver_initialized = RT_TRUE;
    
    LOG_I("Battery voltage driver initialized successfully");
    LOG_D("ADC device: %s, Channel: %d, Enable pin: PB8", VOL_ADC_DEV_NAME, VOL_ADC_CHANNEL);
    
    return RT_EOK;
}

/**
 * @brief 电池电压驱动反初始化函数
 * @return rt_err_t 反初始化结果
 * @retval RT_EOK 反初始化成功
 * @note 该函数释放ADC设备资源并禁用电压检测
 */
rt_err_t drv_vol_deinit(void)
{
    LOG_D("Battery voltage driver deinitializing...");
    
    /* 检查是否已初始化 */
    if (!vol_driver_initialized)
    {
        LOG_W("Battery voltage driver not initialized");
        return RT_EOK;
    }
    
    /* 禁用电压检测 */
    drv_vol_disable();
    
    /* 释放资源 */
    adc_dev = RT_NULL;
    vol_driver_initialized = RT_FALSE;
    vol_detection_status = VOL_STATUS_DISABLED;
    
    LOG_I("Battery voltage driver deinitialized successfully");
    
    return RT_EOK;
}

/**
 * @brief 使能电池电压检测
 * @return rt_err_t 操作结果
 * @retval RT_EOK 使能成功
 * @retval -RT_ERROR 使能失败
 * @note 该函数使能电池电压检测电路
 */
rt_err_t drv_vol_enable(void)
{
    rt_err_t ret = RT_EOK;
    
    /* 检查驱动是否已初始化 */
    if (!vol_driver_is_initialized())
    {
        LOG_E("Battery voltage driver not initialized");
        return -RT_ERROR;
    }
    
    /* 检查是否已经使能 */
    if (vol_detection_status == VOL_STATUS_ENABLED)
    {
        LOG_D("Battery voltage detection already enabled");
        return RT_EOK;
    }
    
    /* 使能ADC通道 */
    ret = rt_adc_enable(adc_dev, VOL_ADC_CHANNEL);
    if (ret != RT_EOK)
    {
        LOG_E("Failed to enable ADC channel %d", VOL_ADC_CHANNEL);
        vol_detection_status = VOL_STATUS_ERROR;
        return -RT_ERROR;
    }
    
    /* 使能电压检测电路 */
    vol_set_enable_pin(VOL_ENABLE_LEVEL);
    
    /* 更新状态 */
    vol_detection_status = VOL_STATUS_ENABLED;
    
    LOG_D("Battery voltage detection enabled");
    
    return RT_EOK;
}

/**
 * @brief 禁用电池电压检测
 * @return rt_err_t 操作结果
 * @retval RT_EOK 禁用成功
 * @note 该函数禁用电池电压检测电路以节省功耗
 */
rt_err_t drv_vol_disable(void)
{
    /* 检查驱动是否已初始化 */
    if (!vol_driver_is_initialized())
    {
        LOG_E("Battery voltage driver not initialized");
        return -RT_ERROR;
    }
    
    /* 检查是否已经禁用 */
    if (vol_detection_status == VOL_STATUS_DISABLED)
    {
        LOG_D("Battery voltage detection already disabled");
        return RT_EOK;
    }
    
    /* 禁用电压检测电路 */
    vol_set_enable_pin(VOL_DISABLE_LEVEL);
    
    /* 禁用ADC通道 */
    rt_adc_disable(adc_dev, VOL_ADC_CHANNEL);
    
    /* 更新状态 */
    vol_detection_status = VOL_STATUS_DISABLED;
    
    LOG_D("Battery voltage detection disabled");
    
    return RT_EOK;
}

/**
 * @brief 读取电池电压原始ADC值
 * @param raw_value 用于存储ADC原始值的指针
 * @return rt_err_t 读取结果
 * @retval RT_EOK 读取成功
 * @retval -RT_EINVAL 参数无效
 * @retval -RT_ERROR 读取失败
 * @note 该函数读取ADC原始采样值 (0-4095)
 */
rt_err_t drv_vol_read_raw(rt_uint32_t *raw_value)
{
    /* 参数检查 */
    if (raw_value == RT_NULL)
    {
        LOG_E("Raw value pointer is NULL");
        return -RT_EINVAL;
    }
    
    /* 检查驱动是否已初始化 */
    if (!vol_driver_is_initialized())
    {
        LOG_E("Battery voltage driver not initialized");
        return -RT_ERROR;
    }
    
    /* 检查电压检测是否使能 */
    if (vol_detection_status != VOL_STATUS_ENABLED)
    {
        LOG_E("Battery voltage detection not enabled");
        return -RT_ERROR;
    }
    
    /* 读取ADC原始值 */
    *raw_value = rt_adc_read(adc_dev, VOL_ADC_CHANNEL);
    
    LOG_D("ADC raw value: %d", *raw_value);
    
    return RT_EOK;
}

/**
 * @brief 读取电池电压值
 * @param voltage_mv 用于存储电压值的指针，单位：毫伏
 * @return rt_err_t 读取结果
 * @retval RT_EOK 读取成功
 * @retval -RT_EINVAL 参数无效
 * @retval -RT_ERROR 读取失败
 * @note 该函数读取并转换电池电压值，单位为毫伏
 */
rt_err_t drv_vol_read_voltage(rt_uint32_t *voltage_mv)
{
    rt_uint32_t raw_value;
    rt_err_t ret;
    
    /* 参数检查 */
    if (voltage_mv == RT_NULL)
    {
        LOG_E("Voltage pointer is NULL");
        return -RT_EINVAL;
    }
    
    /* 读取ADC原始值 */
    ret = drv_vol_read_raw(&raw_value);
    if (ret != RT_EOK)
    {
        return ret;
    }
    
    /* 转换为电压值 */
    *voltage_mv = vol_convert_raw_to_mv(raw_value);
    
    LOG_D("Battery voltage: %d.%02d V", *voltage_mv / 1000, (*voltage_mv % 1000) / 10);

    return RT_EOK;
}

/**
 * @brief 读取完整的电池电压信息
 * @param bat_vol 用于存储电池电压信息的结构体指针
 * @return rt_err_t 读取结果
 * @retval RT_EOK 读取成功
 * @retval -RT_EINVAL 参数无效
 * @retval -RT_ERROR 读取失败
 * @note 该函数读取完整的电池电压信息，包括原始值、电压值和状态
 */
rt_err_t drv_vol_read_info(battery_voltage_t *bat_vol)
{
    rt_uint32_t raw_value;
    rt_err_t ret;

    /* 参数检查 */
    if (bat_vol == RT_NULL)
    {
        LOG_E("Battery voltage info pointer is NULL");
        return -RT_EINVAL;
    }

    /* 初始化结构体 */
    rt_memset(bat_vol, 0, sizeof(battery_voltage_t));

    /* 读取ADC原始值 */
    ret = drv_vol_read_raw(&raw_value);
    if (ret != RT_EOK)
    {
        bat_vol->status = VOL_STATUS_ERROR;
        return ret;
    }

    /* 填充电压信息 */
    bat_vol->raw_value = raw_value;
    bat_vol->voltage_mv = vol_convert_raw_to_mv(raw_value);
    bat_vol->voltage_v = bat_vol->voltage_mv / 1000;
    bat_vol->voltage_frac = (bat_vol->voltage_mv % 1000) / 10;
    bat_vol->status = vol_detection_status;

    LOG_D("Battery info: %d.%02d V (Raw: %d, Status: %d)",
          bat_vol->voltage_v, bat_vol->voltage_frac, bat_vol->raw_value, bat_vol->status);

    return RT_EOK;
}

/**
 * @brief 获取电池电压检测状态
 * @return vol_status_t 检测状态
 * @retval VOL_STATUS_ENABLED 检测已使能
 * @retval VOL_STATUS_DISABLED 检测已禁用
 * @retval VOL_STATUS_ERROR 检测错误
 * @note 该函数获取当前电池电压检测的状态
 */
vol_status_t drv_vol_get_status(void)
{
    return vol_detection_status;
}

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
 */
rt_bool_t drv_vol_is_voltage_normal(rt_uint32_t voltage_mv)
{
    return (voltage_mv >= VOL_BATTERY_MIN && voltage_mv <= VOL_BATTERY_MAX);
}

/**
 * @brief 判断电池电压是否为低电压
 * @param voltage_mv 电压值，单位：毫伏
 * @return rt_bool_t 判断结果
 * @retval RT_TRUE 低电压
 * @retval RT_FALSE 非低电压
 * @note 该函数根据低电压阈值判断电池是否需要充电
 */
rt_bool_t drv_vol_is_voltage_low(rt_uint32_t voltage_mv)
{
    return (voltage_mv < VOL_BATTERY_LOW && voltage_mv >= VOL_BATTERY_MIN);
}

/**
 * @brief 获取电池电压等级描述字符串
 * @param voltage_mv 电压值，单位：毫伏
 * @return const char* 电压等级描述
 * @note 该函数返回电池电压等级的文字描述
 */
const char* drv_vol_get_voltage_level_string(rt_uint32_t voltage_mv)
{
    if (voltage_mv < VOL_BATTERY_MIN)
    {
        return "Critical Low";
    }
    else if (voltage_mv < VOL_BATTERY_LOW)
    {
        return "Low";
    }
    else if (voltage_mv < VOL_BATTERY_NORMAL)
    {
        return "Medium";
    }
    else if (voltage_mv <= VOL_BATTERY_MAX)
    {
        return "Normal";
    }
    else
    {
        return "Over Voltage";
    }
}

/**
 * @}
 */

/**
 * @defgroup Voltage_Driver_Auto_Init 电压驱动自动初始化
 * @{
 */

/**
 * @brief 电池电压驱动自动初始化函数
 * @return int 初始化结果
 * @retval 0 初始化成功
 * @retval -1 初始化失败
 * @note 该函数在系统启动时自动调用，初始化电池电压驱动
 */
static int drv_vol_auto_init(void)
{
    rt_err_t result;

    result = drv_vol_init();
    if (result != RT_EOK)
    {
        LOG_E("Battery voltage driver auto initialization failed");
        return -1;
    }

    LOG_I("Battery voltage driver auto initialization completed");
    return 0;
}
INIT_DEVICE_EXPORT(drv_vol_auto_init);

/**
 * @}
 */
