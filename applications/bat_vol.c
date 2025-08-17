/**
 * @file drv_vol_test.c
 * @brief 电池电压驱动测试文件
 * @details 提供电池电压驱动的测试函数，验证驱动功能的正确性
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

/**
 * @defgroup Voltage_Driver_Test_Functions 电压驱动测试函数
 * @{
 */

/**
 * @brief 电池电压驱动基本功能测试
 * @param argc 参数个数
 * @param argv 参数数组
 * @return int 测试结果
 * @retval 0 测试成功
 * @retval -1 测试失败
 * @note 该函数测试电池电压驱动的基本功能
 */
static int vol_test_basic(int argc, char *argv[])
{
    rt_err_t ret;
    vol_status_t status;
    
    rt_kprintf("=== Battery Voltage Driver Basic Test ===\n");
    
    /* 测试获取状态 */
    status = drv_vol_get_status();
    rt_kprintf("Initial status: %d\n", status);
    
    /* 测试使能电压检测 */
    rt_kprintf("Enabling voltage detection...\n");
    ret = drv_vol_enable();
    if (ret != RT_EOK)
    {
        rt_kprintf("Failed to enable voltage detection: %d\n", ret);
        return -1;
    }
    
    /* 检查状态 */
    status = drv_vol_get_status();
    rt_kprintf("Status after enable: %d\n", status);
    
    /* 测试禁用电压检测 */
    rt_kprintf("Disabling voltage detection...\n");
    ret = drv_vol_disable();
    if (ret != RT_EOK)
    {
        rt_kprintf("Failed to disable voltage detection: %d\n", ret);
        return -1;
    }
    
    /* 检查状态 */
    status = drv_vol_get_status();
    rt_kprintf("Status after disable: %d\n", status);
    
    rt_kprintf("Basic test completed successfully!\n");
    return 0;
}

/**
 * @brief 电池电压读取测试
 * @param argc 参数个数
 * @param argv 参数数组
 * @return int 测试结果
 * @retval 0 测试成功
 * @retval -1 测试失败
 * @note 该函数测试电池电压读取功能
 */
static int vol_test_read(int argc, char *argv[])
{
    rt_err_t ret;
    rt_uint32_t raw_value, voltage_mv;
    battery_voltage_t bat_vol;
    
    rt_kprintf("=== Battery Voltage Read Test ===\n");
    
    /* 使能电压检测 */
    ret = drv_vol_enable();
    if (ret != RT_EOK)
    {
        rt_kprintf("Failed to enable voltage detection: %d\n", ret);
        return -1;
    }
    
    /* 测试读取原始值 */
    rt_kprintf("Reading raw ADC value...\n");
    ret = drv_vol_read_raw(&raw_value);
    if (ret != RT_EOK)
    {
        rt_kprintf("Failed to read raw value: %d\n", ret);
        drv_vol_disable();
        return -1;
    }
    rt_kprintf("Raw ADC value: %d\n", raw_value);
    
    /* 测试读取电压值 */
    rt_kprintf("Reading voltage value...\n");
    ret = drv_vol_read_voltage(&voltage_mv);
    if (ret != RT_EOK)
    {
        rt_kprintf("Failed to read voltage: %d\n", ret);
        drv_vol_disable();
        return -1;
    }
    rt_kprintf("Voltage: %d.%02d V (%d mV)\n", 
               voltage_mv / 1000, (voltage_mv % 1000) / 10, voltage_mv);
    
    /* 测试读取完整信息 */
    rt_kprintf("Reading complete battery info...\n");
    ret = drv_vol_read_info(&bat_vol);
    if (ret != RT_EOK)
    {
        rt_kprintf("Failed to read battery info: %d\n", ret);
        drv_vol_disable();
        return -1;
    }
    
    rt_kprintf("Battery Info:\n");
    rt_kprintf("  Raw Value: %d\n", bat_vol.raw_value);
    rt_kprintf("  Voltage: %d.%02d V (%d mV)\n", 
               bat_vol.voltage_v, bat_vol.voltage_frac, bat_vol.voltage_mv);
    rt_kprintf("  Status: %d\n", bat_vol.status);
    
    /* 测试工具函数 */
    rt_kprintf("Testing utility functions...\n");
    rt_kprintf("  Voltage normal: %s\n", 
               drv_vol_is_voltage_normal(voltage_mv) ? "Yes" : "No");
    rt_kprintf("  Voltage low: %s\n", 
               drv_vol_is_voltage_low(voltage_mv) ? "Yes" : "No");
    rt_kprintf("  Voltage level: %s\n", 
               drv_vol_get_voltage_level_string(voltage_mv));
    
    /* 禁用电压检测 */
    drv_vol_disable();
    
    rt_kprintf("Read test completed successfully!\n");
    return 0;
}

/**
 * @brief 电池电压连续监测测试
 * @param argc 参数个数
 * @param argv 参数数组
 * @return int 测试结果
 * @retval 0 测试成功
 * @retval -1 测试失败
 * @note 该函数进行连续的电池电压监测测试
 */
static int vol_test_monitor(int argc, char *argv[])
{
    rt_err_t ret;
    battery_voltage_t bat_vol;
    int count = 10;  /* 默认监测10次 */
    int interval = 1000;  /* 默认间隔1秒 */
    
    rt_kprintf("=== Battery Voltage Monitor Test ===\n");
    
    /* 解析参数 */
    if (argc >= 2)
    {
        count = atoi(argv[1]);
        if (count <= 0) count = 10;
    }
    if (argc >= 3)
    {
        interval = atoi(argv[2]);
        if (interval <= 0) interval = 1000;
    }
    
    rt_kprintf("Monitor count: %d, interval: %d ms\n", count, interval);
    
    /* 使能电压检测 */
    ret = drv_vol_enable();
    if (ret != RT_EOK)
    {
        rt_kprintf("Failed to enable voltage detection: %d\n", ret);
        return -1;
    }
    
    rt_kprintf("Starting voltage monitoring...\n");
    rt_kprintf("Time(s)\tRaw\tVoltage(V)\tLevel\n");
    rt_kprintf("-------\t---\t----------\t-----\n");
    
    for (int i = 0; i < count; i++)
    {
        ret = drv_vol_read_info(&bat_vol);
        if (ret == RT_EOK)
        {
            rt_kprintf("%d\t%d\t%d.%02d\t\t%s\n", 
                       i + 1,
                       bat_vol.raw_value,
                       bat_vol.voltage_v, bat_vol.voltage_frac,
                       drv_vol_get_voltage_level_string(bat_vol.voltage_mv));
        }
        else
        {
            rt_kprintf("%d\tError reading voltage: %d\n", i + 1, ret);
        }
        
        rt_thread_mdelay(interval);
    }
    
    /* 禁用电压检测 */
    drv_vol_disable();
    
    rt_kprintf("Monitor test completed!\n");
    return 0;
}

/**
 * @brief 兼容性测试 - 与原始测试函数对比
 * @param argc 参数个数
 * @param argv 参数数组
 * @return int 测试结果
 * @retval 0 测试成功
 * @retval -1 测试失败
 * @note 该函数验证新驱动与原始测试代码的兼容性
 */
static int vol_test_compatibility(int argc, char *argv[])
{
    rt_err_t ret;
    rt_uint32_t new_raw, new_voltage;
    battery_voltage_t bat_vol;
    
    /* 原始测试代码的逻辑 */
    rt_adc_device_t adc_dev;
    rt_uint32_t orig_raw, orig_voltage;
    
    rt_kprintf("=== Compatibility Test with Original Code ===\n");
    
    /* 使用新驱动读取 */
    ret = drv_vol_enable();
    if (ret != RT_EOK)
    {
        rt_kprintf("Failed to enable new driver: %d\n", ret);
        return -1;
    }
    
    ret = drv_vol_read_info(&bat_vol);
    if (ret != RT_EOK)
    {
        rt_kprintf("Failed to read with new driver: %d\n", ret);
        drv_vol_disable();
        return -1;
    }
    
    new_raw = bat_vol.raw_value;
    new_voltage = bat_vol.voltage_mv;
    
    /* 使用原始代码逻辑读取 */
    adc_dev = (rt_adc_device_t)rt_device_find("adc1");
    if (adc_dev != RT_NULL)
    {
        rt_adc_enable(adc_dev, 1);
        orig_raw = rt_adc_read(adc_dev, 1);
        orig_voltage = orig_raw * 3300 / 4096;
        rt_adc_disable(adc_dev, 1);
    }
    else
    {
        rt_kprintf("Failed to find ADC device for original test\n");
        drv_vol_disable();
        return -1;
    }
    
    /* 比较结果 */
    rt_kprintf("Comparison Results:\n");
    rt_kprintf("  New Driver - Raw: %d, Voltage: %d mV\n", new_raw, new_voltage);
    rt_kprintf("  Original   - Raw: %d, Voltage: %d mV\n", orig_raw, orig_voltage);
    rt_kprintf("  Raw Diff: %d\n", (int)(new_raw - orig_raw));
    rt_kprintf("  Voltage Diff: %d mV\n", (int)(new_voltage - orig_voltage));
    
    /* 检查差异是否在合理范围内 */
    if (abs((int)(new_raw - orig_raw)) <= 5 && abs((int)(new_voltage - orig_voltage)) <= 20)
    {
        rt_kprintf("Compatibility test PASSED!\n");
    }
    else
    {
        rt_kprintf("Compatibility test FAILED - significant difference detected!\n");
    }
    
    drv_vol_disable();
    return 0;
}

/**
 * @}
 */

/* 导出测试命令到 msh */
MSH_CMD_EXPORT(vol_test_basic, Battery voltage driver basic test);
MSH_CMD_EXPORT(vol_test_read, Battery voltage read test);
MSH_CMD_EXPORT(vol_test_monitor, Battery voltage monitor test [count] [interval_ms]);
MSH_CMD_EXPORT(vol_test_compatibility, Compatibility test with original code);
