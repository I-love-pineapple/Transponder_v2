/**
 * @file network_manager.c
 * @brief 入网管理器实现文件
 * @details 提供设备入网功能的统一管理接口
 * @author RT-Thread Team
 * @date 2024-12-27
 * @version 1.0.0
 */

#include "network_manager.h"
#include "e35_module.h"

#define DBG_TAG "net_mgr"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/**
 * @defgroup Network_Manager_Private_Variables 私有变量
 * @{
 */

/** @brief 入网信息 */
static network_info_t g_network_info = {0};

/** @brief 初始化标志 */
static rt_bool_t g_network_initialized = RT_FALSE;

/** @brief 入网互斥锁 */
static rt_mutex_t g_network_mutex = RT_NULL;

/** @brief 状态字符串表 */
static const char* g_status_strings[NET_STATUS_MAX] = {
    "NONE",
    "JOINING", 
    "JOINED",
    "FAILED",
    "TIMEOUT"
};

/** @brief 回调函数数组 */
#define MAX_NETWORK_CALLBACKS 4
static struct {
    network_event_callback_t callback;
    void *user_data;
    rt_bool_t used;
} g_callbacks[MAX_NETWORK_CALLBACKS] = {0};

/**
 * @}
 */

/**
 * @defgroup Network_Manager_Private_Functions 私有函数
 * @{
 */

/**
 * @brief 通知事件回调
 * @param event 网络事件
 * @param info 网络信息
 */
static void notify_event_callbacks(net_status_t event, const network_info_t *info)
{
    for (int i = 0; i < MAX_NETWORK_CALLBACKS; i++)
    {
        if (g_callbacks[i].used && g_callbacks[i].callback)
        {
            g_callbacks[i].callback(event, info, g_callbacks[i].user_data);
        }
    }
}

/**
 * @}
 */

/**
 * @defgroup Network_Manager_Public_Functions 公共函数
 * @{
 */

/**
 * @brief 初始化入网管理器
 */
rt_err_t network_manager_init(void)
{
    if (g_network_initialized)
    {
        LOG_W("入网管理器已经初始化");
        return RT_EOK;
    }
    
    LOG_I("初始化入网管理器...");
    
    /* 创建互斥锁 */
    g_network_mutex = rt_mutex_create("net_mutex", RT_IPC_FLAG_PRIO);
    if (g_network_mutex == RT_NULL)
    {
        LOG_E("创建入网互斥锁失败");
        return -RT_ERROR;
    }
    
    /* 初始化网络信息 */
    rt_memset(&g_network_info, 0, sizeof(g_network_info));
    g_network_info.status = NET_STATUS_NONE;
    g_network_info.node_id = 0x12345678; /* 默认节点ID */
    
    g_network_initialized = RT_TRUE;
    LOG_I("入网管理器初始化成功");
    
    return RT_EOK;
}

/**
 * @brief 反初始化入网管理器
 */
rt_err_t network_manager_deinit(void)
{
    if (!g_network_initialized)
    {
        LOG_W("入网管理器未初始化");
        return RT_EOK;
    }
    
    LOG_I("反初始化入网管理器...");
    
    /* 删除互斥锁 */
    if (g_network_mutex != RT_NULL)
    {
        rt_mutex_delete(g_network_mutex);
        g_network_mutex = RT_NULL;
    }
    
    g_network_initialized = RT_FALSE;
    LOG_I("入网管理器反初始化完成");
    
    return RT_EOK;
}

/**
 * @brief 开始入网流程
 */
rt_err_t network_start_join(void)
{
    rt_err_t result;
    
    if (!g_network_initialized)
    {
        LOG_E("入网管理器未初始化");
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_network_mutex, RT_WAITING_FOREVER);
    
    if (g_network_info.status == NET_STATUS_JOINING)
    {
        rt_mutex_release(g_network_mutex);
        LOG_W("已在入网中");
        return -RT_EBUSY;
    }
    
    LOG_I("开始入网流程");
    g_network_info.status = NET_STATUS_JOINING;
    g_network_info.retry_count = 0;
    
    rt_mutex_release(g_network_mutex);
    
    /* 启动E35模块的实际入网流程 */
    result = e35_start_join();
    if (result != RT_EOK)
    {
        LOG_E("启动E35入网失败");
        rt_mutex_take(g_network_mutex, RT_WAITING_FOREVER);
        g_network_info.status = NET_STATUS_FAILED;
        rt_mutex_release(g_network_mutex);
        return result;
    }
    
    /* 通知回调 */
    notify_event_callbacks(NET_STATUS_JOINING, &g_network_info);
    
    return RT_EOK;
}

/**
 * @brief 停止入网流程
 */
rt_err_t network_stop_join(void)
{
    if (!g_network_initialized)
    {
        LOG_E("入网管理器未初始化");
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_network_mutex, RT_WAITING_FOREVER);
    
    if (g_network_info.status == NET_STATUS_JOINING)
    {
        LOG_I("停止入网流程");
        g_network_info.status = NET_STATUS_NONE;
    }
    
    rt_mutex_release(g_network_mutex);
    
    return RT_EOK;
}

/**
 * @brief 离开网络
 */
rt_err_t network_leave(void)
{
    if (!g_network_initialized)
    {
        LOG_E("入网管理器未初始化");
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_network_mutex, RT_WAITING_FOREVER);
    
    LOG_I("离开网络");
    g_network_info.status = NET_STATUS_NONE;
    g_network_info.gateway_id = 0;
    g_network_info.work_channel = 0;
    g_network_info.timeslot = 0;
    
    rt_mutex_release(g_network_mutex);
    
    return RT_EOK;
}

/**
 * @brief 获取入网状态
 */
net_status_t network_get_status(void)
{
    if (!g_network_initialized)
    {
        return NET_STATUS_NONE;
    }
    
    rt_mutex_take(g_network_mutex, RT_WAITING_FOREVER);
    net_status_t status = g_network_info.status;
    rt_mutex_release(g_network_mutex);
    
    return status;
}

/**
 * @brief 获取入网信息
 */
rt_err_t network_get_info(network_info_t *info)
{
    if (!g_network_initialized || info == RT_NULL)
    {
        return -RT_EINVAL;
    }
    
    rt_mutex_take(g_network_mutex, RT_WAITING_FOREVER);
    rt_memcpy(info, &g_network_info, sizeof(network_info_t));
    rt_mutex_release(g_network_mutex);
    
    return RT_EOK;
}

/**
 * @brief 检查是否已入网
 */
rt_bool_t network_is_joined(void)
{
    return (network_get_status() == NET_STATUS_JOINED);
}

/**
 * @brief 注册入网事件回调函数
 */
rt_err_t network_register_callback(network_event_callback_t callback, void *user_data)
{
    if (!g_network_initialized || callback == RT_NULL)
    {
        return -RT_EINVAL;
    }
    
    for (int i = 0; i < MAX_NETWORK_CALLBACKS; i++)
    {
        if (!g_callbacks[i].used)
        {
            g_callbacks[i].callback = callback;
            g_callbacks[i].user_data = user_data;
            g_callbacks[i].used = RT_TRUE;
            LOG_D("注册入网事件回调 [%d]", i);
            return RT_EOK;
        }
    }
    
    LOG_E("入网事件回调已满");
    return -RT_EFULL;
}

/**
 * @brief 注销入网事件回调函数
 */
rt_err_t network_unregister_callback(network_event_callback_t callback)
{
    if (!g_network_initialized || callback == RT_NULL)
    {
        return -RT_EINVAL;
    }
    
    for (int i = 0; i < MAX_NETWORK_CALLBACKS; i++)
    {
        if (g_callbacks[i].used && g_callbacks[i].callback == callback)
        {
            g_callbacks[i].used = RT_FALSE;
            g_callbacks[i].callback = RT_NULL;
            g_callbacks[i].user_data = RT_NULL;
            LOG_D("注销入网事件回调 [%d]", i);
            return RT_EOK;
        }
    }
    
    LOG_W("未找到要注销的回调函数");
    return -RT_EINVAL;
}

/**
 * @brief 获取入网状态字符串
 */
const char* network_status_to_string(net_status_t status)
{
    if (status >= NET_STATUS_MAX)
    {
        return "UNKNOWN";
    }
    
    return g_status_strings[status];
}

/**
 * @brief 设置节点ID
 */
rt_err_t network_set_node_id(rt_uint32_t node_id)
{
    if (!g_network_initialized)
    {
        return -RT_EINVAL;
    }
    
    rt_mutex_take(g_network_mutex, RT_WAITING_FOREVER);
    g_network_info.node_id = node_id;
    rt_mutex_release(g_network_mutex);
    
    LOG_D("设置节点ID: 0x%08X", node_id);
    return RT_EOK;
}

/**
 * @brief 获取节点ID
 */
rt_uint32_t network_get_node_id(void)
{
    if (!g_network_initialized)
    {
        return 0;
    }
    
    rt_mutex_take(g_network_mutex, RT_WAITING_FOREVER);
    rt_uint32_t node_id = g_network_info.node_id;
    rt_mutex_release(g_network_mutex);
    
    return node_id;
}

/**
 * @brief 强制重新入网
 */
rt_err_t network_rejoin(void)
{
    if (!g_network_initialized)
    {
        return -RT_ERROR;
    }
    
    LOG_I("强制重新入网");
    
    /* 先离网 */
    network_leave();
    
    /* 延时后重新入网 */
    rt_thread_mdelay(100);
    
    return network_start_join();
}

/**
 * @brief 处理入网超时
 * @note 由E35模块超时回调调用，更新网络状态并通知上层
 */
void network_handle_join_timeout(void)
{
    if (!g_network_initialized)
    {
        return;
    }
    
    rt_mutex_take(g_network_mutex, RT_WAITING_FOREVER);
    
    if (g_network_info.status == NET_STATUS_JOINING)
    {
        LOG_W("入网超时，状态切换到超时状态");
        g_network_info.status = NET_STATUS_TIMEOUT;
        g_network_info.retry_count++;
    }
    
    rt_mutex_release(g_network_mutex);
    
    /* 通知回调函数 */
    notify_event_callbacks(NET_STATUS_TIMEOUT, &g_network_info);
}

/**
 * @}
 */

/* MSH命令 */
static int cmd_network(int argc, char *argv[])
{
    if (argc < 2)
    {
        rt_kprintf("用法: network <status|join|leave|rejoin|info>\n");
        return 0;
    }
    
    if (rt_strcmp(argv[1], "status") == 0)
    {
        net_status_t status = network_get_status();
        rt_kprintf("入网状态: %s\n", network_status_to_string(status));
    }
    else if (rt_strcmp(argv[1], "join") == 0)
    {
        rt_err_t result = network_start_join();
        rt_kprintf("开始入网: %s\n", result == RT_EOK ? "成功" : "失败");
    }
    else if (rt_strcmp(argv[1], "leave") == 0)
    {
        network_leave();
        rt_kprintf("离开网络\n");
    }
    else if (rt_strcmp(argv[1], "rejoin") == 0)
    {
        rt_err_t result = network_rejoin();
        rt_kprintf("重新入网: %s\n", result == RT_EOK ? "成功" : "失败");
    }
    else if (rt_strcmp(argv[1], "info") == 0)
    {
        network_info_t info;
        if (network_get_info(&info) == RT_EOK)
        {
            rt_kprintf("=== 入网信息 ===\n");
            rt_kprintf("状态: %s\n", network_status_to_string(info.status));
            rt_kprintf("网关ID: 0x%08X\n", info.gateway_id);
            rt_kprintf("节点ID: 0x%08X\n", info.node_id);
            rt_kprintf("工作信道: %d\n", info.work_channel);
            rt_kprintf("时隙: %d\n", info.timeslot);
            rt_kprintf("信号强度: %d\n", info.rssi);
            rt_kprintf("重试次数: %d\n", info.retry_count);
        }
        else
        {
            rt_kprintf("获取入网信息失败\n");
        }
    }
    else
    {
        rt_kprintf("未知命令: %s\n", argv[1]);
    }
    
    return 0;
}

MSH_CMD_EXPORT(cmd_network, Network management: status|join|leave|rejoin|info);
