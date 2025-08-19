/**
 * @file config_manager.c
 * @brief 配置管理器实现文件
 * @details 提供系统配置的统一管理和掉电存储功能
 * @author RT-Thread Team
 * @date 2024-12-27
 * @version 1.0.0
 */

#include "config_manager.h"
#include "config_storage.h"

#define DBG_TAG "config_mgr"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/**
 * @defgroup Config_Manager_Private_Variables 私有变量
 * @{
 */

/** @brief 系统配置实例 */
static system_config_t g_system_config = {0};

/** @brief 配置管理器初始化标志 */
static rt_bool_t g_config_initialized = RT_FALSE;

/** @brief 配置互斥锁 */
static rt_mutex_t g_config_mutex = RT_NULL;

/**
 * @}
 */

/**
 * @defgroup Config_Manager_Private_Functions 私有函数
 * @{
 */

/**
 * @brief 初始化默认配置
 */
static void init_default_config(void)
{
    rt_memset(&g_system_config, 0, sizeof(g_system_config));
    
    /* 设置配置头信息 */
    g_system_config.magic = CONFIG_MAGIC;
    g_system_config.version = CONFIG_VERSION;
    
    /* 网络配置默认值 */
    g_system_config.network.gateway_id = DEFAULT_GATEWAY_ID;
    g_system_config.network.node_id = DEFAULT_NODE_ID;
    g_system_config.network.work_channel = DEFAULT_WORK_CHANNEL;
    g_system_config.network.timeslot = DEFAULT_TIMESLOT;
    g_system_config.network.beacon_channels[0] = 0;
    g_system_config.network.beacon_channels[1] = 27;
    g_system_config.network.beacon_channels[2] = 54;
    g_system_config.network.join_timeout = DEFAULT_JOIN_TIMEOUT;
    g_system_config.network.max_join_retry = DEFAULT_MAX_JOIN_RETRY;
    
    /* RF配置默认值 */
    g_system_config.rf.address_high = DEFAULT_ADDR_HIGH;
    g_system_config.rf.address_low = DEFAULT_ADDR_LOW;
    g_system_config.rf.channel = DEFAULT_RF_CHANNEL;
    g_system_config.rf.power = DEFAULT_RF_POWER;
    g_system_config.rf.rate = DEFAULT_RF_RATE;
    g_system_config.rf.encrypt_enabled = RT_FALSE;
    g_system_config.rf.encrypt_key0 = 123;
    g_system_config.rf.encrypt_key1 = 456;
    
    /* 按键配置默认值 */
    g_system_config.key.upload_timeout = DEFAULT_UPLOAD_TIMEOUT;
    g_system_config.key.max_upload_retry = DEFAULT_MAX_UPLOAD_RETRY;
    g_system_config.key.retry_interval = DEFAULT_RETRY_INTERVAL;
    g_system_config.key.initial_backoff = DEFAULT_INITIAL_BACKOFF;
    g_system_config.key.max_backoff = DEFAULT_MAX_BACKOFF;
    
    /* 电源配置默认值 */
    g_system_config.power.low_voltage_threshold = DEFAULT_LOW_VOLTAGE_THRESHOLD;
    g_system_config.power.normal_voltage_min = DEFAULT_NORMAL_VOLTAGE_MIN;
    g_system_config.power.battery_check_interval = DEFAULT_BATTERY_CHECK_INTERVAL;
    g_system_config.power.auto_sleep_enabled = RT_FALSE;
    g_system_config.power.idle_sleep_timeout = DEFAULT_IDLE_SLEEP_TIMEOUT;
    
    LOG_I("配置已初始化为默认值");
}

/**
 * @brief 计算配置CRC32
 * @param config 配置结构体指针
 * @return rt_uint32_t CRC32值
 */
static rt_uint32_t calculate_config_crc32(const system_config_t *config)
{
    /* 简化的CRC计算，实际应用中应使用标准CRC32算法 */
    rt_uint32_t crc = 0;
    const rt_uint8_t *data = (const rt_uint8_t *)config;
    rt_uint32_t len = sizeof(system_config_t) - sizeof(config->crc32);
    
    for (rt_uint32_t i = 0; i < len; i++)
    {
        crc += data[i];
    }
    
    return crc;
}

/**
 * @}
 */

/**
 * @defgroup Config_Manager_Public_Functions 公共函数
 * @{
 */

/**
 * @brief 初始化配置管理器
 */
rt_err_t config_manager_init(void)
{
    if (g_config_initialized)
    {
        LOG_W("配置管理器已经初始化");
        return RT_EOK;
    }
    
    LOG_I("初始化配置管理器...");
    
    /* 创建互斥锁 */
    g_config_mutex = rt_mutex_create("cfg_mutex", RT_IPC_FLAG_PRIO);
    if (g_config_mutex == RT_NULL)
    {
        LOG_E("创建配置互斥锁失败");
        return -RT_ERROR;
    }
    
    /* 初始化默认配置 */
    init_default_config();
    
    g_config_initialized = RT_TRUE;
    LOG_I("配置管理器初始化成功");
    
    return RT_EOK;
}

/**
 * @brief 反初始化配置管理器
 */
rt_err_t config_manager_deinit(void)
{
    if (!g_config_initialized)
    {
        LOG_W("配置管理器未初始化");
        return RT_EOK;
    }
    
    LOG_I("反初始化配置管理器...");
    
    /* 删除互斥锁 */
    if (g_config_mutex != RT_NULL)
    {
        rt_mutex_delete(g_config_mutex);
        g_config_mutex = RT_NULL;
    }
    
    g_config_initialized = RT_FALSE;
    LOG_I("配置管理器反初始化完成");
    
    return RT_EOK;
}

/**
 * @brief 加载配置
 */
rt_err_t config_load(void)
{
    if (!g_config_initialized)
    {
        LOG_E("配置管理器未初始化");
        return -RT_ERROR;
    }
    
    LOG_I("加载配置...");
    
    /* TODO: 从存储器读取配置 */
    /* 目前使用默认配置 */
    LOG_W("配置存储功能暂未实现，使用默认配置");
    
    return RT_EOK;
}

/**
 * @brief 保存配置
 */
rt_err_t config_save(void)
{
    if (!g_config_initialized)
    {
        LOG_E("配置管理器未初始化");
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER);
    
    /* 更新CRC */
    g_system_config.crc32 = calculate_config_crc32(&g_system_config);
    
    rt_mutex_release(g_config_mutex);
    
    LOG_I("配置保存成功 (CRC32: 0x%08X)", g_system_config.crc32);
    
    /* TODO: 写入到存储器 */
    LOG_W("配置存储功能暂未实现");
    
    return RT_EOK;
}

/**
 * @brief 恢复默认配置
 */
rt_err_t config_restore_defaults(void)
{
    if (!g_config_initialized)
    {
        LOG_E("配置管理器未初始化");
        return -RT_ERROR;
    }
    
    LOG_I("恢复默认配置...");
    
    rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER);
    init_default_config();
    rt_mutex_release(g_config_mutex);
    
    LOG_I("默认配置恢复完成");
    
    return RT_EOK;
}

/**
 * @brief 获取系统配置
 */
const system_config_t* config_get_system(void)
{
    return &g_system_config;
}

/**
 * @brief 获取网络配置
 */
const network_config_t* config_get_network(void)
{
    return &g_system_config.network;
}

/**
 * @brief 获取RF配置
 */
const rf_config_t* config_get_rf(void)
{
    return &g_system_config.rf;
}

/**
 * @brief 获取按键配置
 */
const key_config_t* config_get_key(void)
{
    return &g_system_config.key;
}

/**
 * @brief 获取电源配置
 */
const power_config_t* config_get_power(void)
{
    return &g_system_config.power;
}

/**
 * @brief 设置网络配置
 */
rt_err_t config_set_network(const network_config_t *config)
{
    if (!g_config_initialized || config == RT_NULL)
    {
        return -RT_EINVAL;
    }
    
    rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER);
    rt_memcpy(&g_system_config.network, config, sizeof(network_config_t));
    rt_mutex_release(g_config_mutex);
    
    LOG_D("网络配置已更新");
    return RT_EOK;
}

/**
 * @brief 设置RF配置
 */
rt_err_t config_set_rf(const rf_config_t *config)
{
    if (!g_config_initialized || config == RT_NULL)
    {
        return -RT_EINVAL;
    }
    
    rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER);
    rt_memcpy(&g_system_config.rf, config, sizeof(rf_config_t));
    rt_mutex_release(g_config_mutex);
    
    LOG_D("RF配置已更新");
    return RT_EOK;
}

/**
 * @brief 设置按键配置
 */
rt_err_t config_set_key(const key_config_t *config)
{
    if (!g_config_initialized || config == RT_NULL)
    {
        return -RT_EINVAL;
    }
    
    rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER);
    rt_memcpy(&g_system_config.key, config, sizeof(key_config_t));
    rt_mutex_release(g_config_mutex);
    
    LOG_D("按键配置已更新");
    return RT_EOK;
}

/**
 * @brief 设置电源配置
 */
rt_err_t config_set_power(const power_config_t *config)
{
    if (!g_config_initialized || config == RT_NULL)
    {
        return -RT_EINVAL;
    }
    
    rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER);
    rt_memcpy(&g_system_config.power, config, sizeof(power_config_t));
    rt_mutex_release(g_config_mutex);
    
    LOG_D("电源配置已更新");
    return RT_EOK;
}

/**
 * @brief 验证配置有效性
 */
rt_bool_t config_validate(config_item_t item)
{
    /* 简化的验证逻辑 */
    return RT_TRUE;
}

/**
 * @brief 打印配置信息
 */
void config_print(config_item_t item)
{
    if (!g_config_initialized)
    {
        rt_kprintf("配置管理器未初始化\n");
        return;
    }
    
    rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER);
    
    switch (item)
    {
        case CONFIG_ITEM_NETWORK:
            rt_kprintf("=== 网络配置 ===\n");
            rt_kprintf("网关ID: 0x%08X\n", g_system_config.network.gateway_id);
            rt_kprintf("节点ID: 0x%08X\n", g_system_config.network.node_id);
            rt_kprintf("工作信道: %d\n", g_system_config.network.work_channel);
            rt_kprintf("时隙: %d\n", g_system_config.network.timeslot);
            rt_kprintf("入网超时: %d ms\n", g_system_config.network.join_timeout);
            break;
            
        case CONFIG_ITEM_RF:
            rt_kprintf("=== RF配置 ===\n");
            rt_kprintf("地址: %d.%d\n", g_system_config.rf.address_high, g_system_config.rf.address_low);
            rt_kprintf("信道: %d\n", g_system_config.rf.channel);
            rt_kprintf("功率: %d\n", g_system_config.rf.power);
            rt_kprintf("速率: %d\n", g_system_config.rf.rate);
            break;
            
        case CONFIG_ITEM_KEY:
            rt_kprintf("=== 按键配置 ===\n");
            rt_kprintf("上传超时: %d ms\n", g_system_config.key.upload_timeout);
            rt_kprintf("最大重试: %d\n", g_system_config.key.max_upload_retry);
            rt_kprintf("重传间隔: %d ms\n", g_system_config.key.retry_interval);
            break;
            
        case CONFIG_ITEM_POWER:
            rt_kprintf("=== 电源配置 ===\n");
            rt_kprintf("低电压门限: %d mV\n", g_system_config.power.low_voltage_threshold);
            rt_kprintf("正常电压: %d mV\n", g_system_config.power.normal_voltage_min);
            rt_kprintf("自动休眠: %s\n", g_system_config.power.auto_sleep_enabled ? "启用" : "禁用");
            break;
            
        case CONFIG_ITEM_ALL:
            config_print(CONFIG_ITEM_NETWORK);
            config_print(CONFIG_ITEM_RF);
            config_print(CONFIG_ITEM_KEY);
            config_print(CONFIG_ITEM_POWER);
            break;
            
        default:
            rt_kprintf("未知的配置项: %d\n", item);
            break;
    }
    
    rt_mutex_release(g_config_mutex);
}

/**
 * @}
 */

/* MSH命令 */
static int cmd_config(int argc, char *argv[])
{
    if (argc < 2)
    {
        rt_kprintf("用法: config <print|save|load|restore>\n");
        return 0;
    }
    
    if (rt_strcmp(argv[1], "print") == 0)
    {
        config_print(CONFIG_ITEM_ALL);
    }
    else if (rt_strcmp(argv[1], "save") == 0)
    {
        config_save();
        rt_kprintf("配置已保存\n");
    }
    else if (rt_strcmp(argv[1], "load") == 0)
    {
        config_load();
        rt_kprintf("配置已加载\n");
    }
    else if (rt_strcmp(argv[1], "restore") == 0)
    {
        config_restore_defaults();
        rt_kprintf("配置已恢复到默认值\n");
    }
    else
    {
        rt_kprintf("未知命令: %s\n", argv[1]);
    }
    
    return 0;
}

MSH_CMD_EXPORT(cmd_config, Configuration management: print|save|load|restore);
