/**
 * @file app_manager.c
 * @brief 应用管理器实现文件
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

#include "app_manager.h"
#include "system_state.h"
#include "config_manager.h"
#include "network_manager.h"
#include "key_manager.h"
#include "led_app.h"

#define DBG_TAG "app_mgr"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/**
 * @defgroup App_Manager_Private_Variables 私有变量
 * @{
 */

/** @brief 应用管理器控制结构体 */
static struct
{
    rt_bool_t initialized;              /**< 是否已初始化 */
    rt_bool_t started;                  /**< 是否已启动 */
    app_mode_t mode;                    /**< 运行模式 */
    rt_uint32_t start_time;             /**< 启动时间 */
    rt_uint32_t total_key_events;       /**< 总按键事件数 */
    rt_uint32_t total_network_joins;    /**< 总入网次数 */
    rt_uint32_t last_error_code;        /**< 最后错误码 */
} g_app_ctrl = {0};

/** @brief 运行模式字符串表 */
static const char* g_mode_strings[APP_MODE_MAX] = {
    "NORMAL",
    "LOW_POWER",
    "FACTORY_TEST",
    "DEBUG"
};

/**
 * @}
 */

/**
 * @defgroup App_Manager_Private_Functions 私有函数
 * @{
 */

/**
 * @brief 系统状态变化回调函数
 * @param old_state 旧状态
 * @param new_state 新状态
 * @param user_data 用户数据
 */
static void app_system_state_callback(sys_state_t old_state, sys_state_t new_state, void *user_data)
{
    LOG_D("系统状态变化: %s -> %s", 
          system_state_to_string(old_state), 
          system_state_to_string(new_state));

    /* 根据状态变化执行相应的操作 */
    switch (new_state)
    {
        case SYS_STATE_JOINED:
            if (old_state == SYS_STATE_JOINING)
            {
                g_app_ctrl.total_network_joins++;
                LOG_I("入网成功，总入网次数: %d", g_app_ctrl.total_network_joins);
            }
            break;

        case SYS_STATE_ERROR:
            LOG_E("系统进入错误状态，上一状态: %s", system_state_to_string(old_state));
            break;

        default:
            break;
    }
}

/**
 * @brief 网络事件回调函数
 * @param event 网络事件
 * @param info 网络信息
 * @param user_data 用户数据
 */
static void app_network_event_callback(net_status_t event, const network_info_t *info, void *user_data)
{
    LOG_D("网络事件: %s", network_status_to_string(event));

    switch (event)
    {
        case NET_STATUS_JOINED:
            system_state_post_event(SYS_EVENT_JOIN_SUCCESS, RT_NULL);
            break;

        case NET_STATUS_FAILED:
        case NET_STATUS_TIMEOUT:
            system_state_post_event(SYS_EVENT_JOIN_FAILED, RT_NULL);
            break;

        default:
            break;
    }
}

/**
 * @brief 按键事件回调函数
 * @param data 按键数据
 * @param user_data 用户数据
 */
static void app_key_event_callback(const key_data_t *data, void *user_data)
{
    g_app_ctrl.total_key_events++;
    
    LOG_D("按键事件: %s %s (总数: %d)", 
          key_id_to_string(data->key_id),
          key_event_to_string(data->event),
          g_app_ctrl.total_key_events);

    /* 处理按键1长按事件 - 用于入网控制 */
    if (data->key_id == KEY_ID_1 && data->event == KEY_EVENT_LONG_PRESS)
    {
        app_manager_handle_long_press(data->key_id);
        return;
    }

    /* 处理其他按键短按事件 - 用于数据上传 */
    if (data->event == KEY_EVENT_PRESS)
    {
        sys_state_t current_state = system_state_get();
        if (current_state == SYS_STATE_JOINED)
        {
            /* 开始按键上传 */
            if (key_manager_upload(data->key_id) == RT_EOK)
            {
                system_state_post_event(SYS_EVENT_KEY_UPLOAD_START, RT_NULL);
            }
        }
        else
        {
            LOG_W("当前状态不允许按键上传: %s", system_state_to_string(current_state));
        }
    }
}

/**
 * @brief 按键上传状态回调函数
 * @param info 上传信息
 * @param user_data 用户数据
 */
static void app_key_upload_callback(const key_upload_info_t *info, void *user_data)
{
    LOG_D("按键上传状态: %s", key_upload_status_to_string(info->status));

    switch (info->status)
    {
        case KEY_UPLOAD_SUCCESS:
            system_state_post_event(SYS_EVENT_KEY_UPLOAD_SUCCESS, RT_NULL);
            break;

        case KEY_UPLOAD_FAILED:
        case KEY_UPLOAD_TIMEOUT:
            system_state_post_event(SYS_EVENT_KEY_UPLOAD_FAILED, RT_NULL);
            break;

        default:
            break;
    }
}

/**
 * @brief 初始化所有子模块
 * @return rt_err_t 初始化结果
 */
static rt_err_t init_all_modules(void)
{
    rt_err_t result;

    /* 初始化配置管理器 */
    result = config_manager_init();
    if (result != RT_EOK)
    {
        LOG_E("配置管理器初始化失败: %d", result);
        return result;
    }

    /* 加载配置 */
    result = config_load();
    if (result != RT_EOK)
    {
        LOG_W("配置加载失败，使用默认配置: %d", result);
    }

    /* 初始化系统状态管理器 */
    result = system_state_init();
    if (result != RT_EOK)
    {
        LOG_E("系统状态管理器初始化失败: %d", result);
        config_manager_deinit();
        return result;
    }

    /* 初始化网络管理器 */
    result = network_manager_init();
    if (result != RT_EOK)
    {
        LOG_E("网络管理器初始化失败: %d", result);
        system_state_deinit();
        config_manager_deinit();
        return result;
    }

    /* 初始化按键管理器 */
    result = key_manager_init();
    if (result != RT_EOK)
    {
        LOG_E("按键管理器初始化失败: %d", result);
        network_manager_deinit();
        system_state_deinit();
        config_manager_deinit();
        return result;
    }

    /* 初始化LED应用 */
    result = led_app_init();
    if (result != 0)
    {
        LOG_E("LED应用初始化失败: %d", result);
        key_manager_deinit();
        network_manager_deinit();
        system_state_deinit();
        config_manager_deinit();
        return -RT_ERROR;
    }

    return RT_EOK;
}

/**
 * @brief 注册所有回调函数
 * @return rt_err_t 注册结果
 */
static rt_err_t register_all_callbacks(void)
{
    rt_err_t result;

    /* 注册系统状态变化回调 */
    result = system_state_register_callback(app_system_state_callback, RT_NULL);
    if (result != RT_EOK)
    {
        LOG_E("注册系统状态回调失败: %d", result);
        return result;
    }

    /* 注册网络事件回调 */
    result = network_register_callback(app_network_event_callback, RT_NULL);
    if (result != RT_EOK)
    {
        LOG_E("注册网络事件回调失败: %d", result);
        return result;
    }

    /* 注册按键事件回调 */
    result = key_manager_register_event_callback(app_key_event_callback, RT_NULL);
    if (result != RT_EOK)
    {
        LOG_E("注册按键事件回调失败: %d", result);
        return result;
    }

    /* 注册按键上传状态回调 */
    result = key_manager_register_upload_callback(app_key_upload_callback, RT_NULL);
    if (result != RT_EOK)
    {
        LOG_E("注册按键上传回调失败: %d", result);
        return result;
    }

    return RT_EOK;
}

/**
 * @brief 反初始化所有子模块
 */
static void deinit_all_modules(void)
{
    key_manager_deinit();
    network_manager_deinit();
    system_state_deinit();
    config_manager_deinit();
}

/**
 * @}
 */

/**
 * @defgroup App_Manager_Public_Functions 公共函数
 * @{
 */

/**
 * @brief 初始化应用管理器
 */
rt_err_t app_manager_init(void)
{
    rt_err_t result;

    if (g_app_ctrl.initialized)
    {
        LOG_W("应用管理器已经初始化");
        return RT_EOK;
    }

    LOG_I("初始化应用管理器...");

    /* 初始化控制结构体 */
    rt_memset(&g_app_ctrl, 0, sizeof(g_app_ctrl));
    g_app_ctrl.mode = APP_MODE_NORMAL;

    /* 初始化所有子模块 */
    result = init_all_modules();
    if (result != RT_EOK)
    {
        LOG_E("子模块初始化失败: %d", result);
        return result;
    }

    /* 注册回调函数 */
    result = register_all_callbacks();
    if (result != RT_EOK)
    {
        LOG_E("回调函数注册失败: %d", result);
        deinit_all_modules();
        return result;
    }

    g_app_ctrl.initialized = RT_TRUE;
    LOG_I("应用管理器初始化成功");

    return RT_EOK;
}

/**
 * @brief 反初始化应用管理器
 */
rt_err_t app_manager_deinit(void)
{
    if (!g_app_ctrl.initialized)
    {
        LOG_W("应用管理器未初始化");
        return RT_EOK;
    }

    LOG_I("反初始化应用管理器...");

    /* 停止运行 */
    if (g_app_ctrl.started)
    {
        app_manager_stop();
    }

    /* 反初始化所有子模块 */
    deinit_all_modules();

    g_app_ctrl.initialized = RT_FALSE;
    LOG_I("应用管理器反初始化完成");

    return RT_EOK;
}

/**
 * @brief 启动应用管理器
 */
rt_err_t app_manager_start(void)
{
    if (!g_app_ctrl.initialized)
    {
        LOG_E("应用管理器未初始化");
        return -RT_ERROR;
    }

    if (g_app_ctrl.started)
    {
        LOG_W("应用管理器已经启动");
        return RT_EOK;
    }

    LOG_I("启动应用管理器...");

    /* 记录启动时间 */
    g_app_ctrl.start_time = rt_tick_get();

    /* 发送初始化完成事件 */
    system_state_post_event(SYS_EVENT_INIT_COMPLETE, RT_NULL);

    g_app_ctrl.started = RT_TRUE;
    LOG_I("应用管理器启动成功");

    return RT_EOK;
}

/**
 * @brief 停止应用管理器
 */
rt_err_t app_manager_stop(void)
{
    if (!g_app_ctrl.started)
    {
        LOG_W("应用管理器未启动");
        return RT_EOK;
    }

    LOG_I("停止应用管理器...");

    /* 停止所有活动 */
    network_stop_join();
    key_manager_stop_upload();

    g_app_ctrl.started = RT_FALSE;
    LOG_I("应用管理器停止完成");

    return RT_EOK;
}

/**
 * @brief 设置运行模式
 */
rt_err_t app_manager_set_mode(app_mode_t mode)
{
    if (mode >= APP_MODE_MAX)
    {
        LOG_E("无效的运行模式: %d", mode);
        return -RT_EINVAL;
    }

    if (g_app_ctrl.mode != mode)
    {
        LOG_I("运行模式切换: %s -> %s", 
              g_mode_strings[g_app_ctrl.mode], 
              g_mode_strings[mode]);
        g_app_ctrl.mode = mode;
    }

    return RT_EOK;
}

/**
 * @brief 获取运行模式
 */
app_mode_t app_manager_get_mode(void)
{
    return g_app_ctrl.mode;
}

/**
 * @brief 获取应用管理器信息
 */
rt_err_t app_manager_get_info(app_manager_info_t *info)
{
    if (info == RT_NULL)
    {
        return -RT_EINVAL;
    }

    info->initialized = g_app_ctrl.initialized;
    info->mode = g_app_ctrl.mode;
    info->total_key_events = g_app_ctrl.total_key_events;
    info->total_network_joins = g_app_ctrl.total_network_joins;
    info->last_error_code = g_app_ctrl.last_error_code;
    info->current_state = system_state_get();

    if (g_app_ctrl.started && g_app_ctrl.start_time > 0)
    {
        info->uptime = (rt_tick_get() - g_app_ctrl.start_time) / RT_TICK_PER_SECOND;
    }
    else
    {
        info->uptime = 0;
    }

    return RT_EOK;
}

/**
 * @brief 处理按键长按事件（用于入网控制）
 */
rt_err_t app_manager_handle_long_press(key_id_t key_id)
{
    sys_state_t current_state;

    if (!g_app_ctrl.initialized || !g_app_ctrl.started)
    {
        LOG_E("应用管理器未就绪");
        return -RT_ERROR;
    }

    if (key_id != KEY_ID_1)
    {
        LOG_W("只有KEY1长按支持入网控制");
        return -RT_EINVAL;
    }

    current_state = system_state_get();

    LOG_I("处理KEY1长按事件，当前状态: %s", system_state_to_string(current_state));

    switch (current_state)
    {
        case SYS_STATE_NOT_JOINED:
            /* 开始入网 */
            LOG_I("开始入网流程");
            if (network_start_join() == RT_EOK)
            {
                system_state_post_event(SYS_EVENT_JOIN_REQUEST, RT_NULL);
            }
            break;

        case SYS_STATE_JOINED:
            /* 离网 */
            LOG_I("离开网络");
            network_leave();
            system_state_post_event(SYS_EVENT_LEAVE_NETWORK, RT_NULL);
            break;

        case SYS_STATE_JOINING:
            LOG_W("正在入网中，忽略长按事件");
            break;

        case SYS_STATE_KEY_UPLOADING:
        case SYS_STATE_KEY_UPLOAD_DONE:
            LOG_W("正在处理按键上传，忽略长按事件");
            break;

        case SYS_STATE_SLEEP:
            LOG_I("从休眠状态唤醒");
            system_state_post_event(SYS_EVENT_WAKEUP, RT_NULL);
            break;

        default:
            LOG_W("当前状态不支持入网控制: %s", system_state_to_string(current_state));
            break;
    }

    return RT_EOK;
}

/**
 * @brief 重启系统
 */
rt_err_t app_manager_reboot(rt_uint32_t delay_ms)
{
    LOG_I("系统将在 %d ms 后重启", delay_ms);

    if (delay_ms > 0)
    {
        rt_thread_mdelay(delay_ms);
    }

    /* 保存配置 */
    config_save();

    /* 重启系统 */
    rt_hw_cpu_reset();

    return RT_EOK;
}

/**
 * @brief 进入低功耗模式
 */
rt_err_t app_manager_enter_low_power(void)
{
    LOG_I("进入低功耗模式");

    /* 设置低功耗模式 */
    app_manager_set_mode(APP_MODE_LOW_POWER);

    /* 发送进入休眠事件 */
    system_state_post_event(SYS_EVENT_ENTER_SLEEP, RT_NULL);

    return RT_EOK;
}

/**
 * @brief 退出低功耗模式
 */
rt_err_t app_manager_exit_low_power(void)
{
    LOG_I("退出低功耗模式");

    /* 恢复正常模式 */
    app_manager_set_mode(APP_MODE_NORMAL);

    /* 发送唤醒事件 */
    system_state_post_event(SYS_EVENT_WAKEUP, RT_NULL);

    return RT_EOK;
}

/**
 * @brief 获取运行模式字符串
 */
const char* app_mode_to_string(app_mode_t mode)
{
    if (mode >= APP_MODE_MAX)
    {
        return "UNKNOWN";
    }

    return g_mode_strings[mode];
}

/**
 * @brief 打印系统信息
 */
void app_manager_print_info(void)
{
    app_manager_info_t info;
    network_info_t net_info;
    key_upload_info_t key_info;

    if (app_manager_get_info(&info) != RT_EOK)
    {
        rt_kprintf("获取系统信息失败\n");
        return;
    }

    rt_kprintf("=== 系统信息 ===\n");
    rt_kprintf("初始化状态: %s\n", info.initialized ? "是" : "否");
    rt_kprintf("运行模式: %s\n", app_mode_to_string(info.mode));
    rt_kprintf("运行时间: %d 秒\n", info.uptime);
    rt_kprintf("当前状态: %s\n", system_state_to_string(info.current_state));
    rt_kprintf("按键事件总数: %d\n", info.total_key_events);
    rt_kprintf("入网总次数: %d\n", info.total_network_joins);
    rt_kprintf("最后错误码: %d\n", info.last_error_code);

    if (network_get_info(&net_info) == RT_EOK)
    {
        rt_kprintf("\n=== 网络信息 ===\n");
        rt_kprintf("入网状态: %s\n", network_status_to_string(net_info.status));
        rt_kprintf("网关ID: 0x%08X\n", net_info.gateway_id);
        rt_kprintf("节点ID: 0x%08X\n", net_info.node_id);
        rt_kprintf("工作信道: %d\n", net_info.work_channel);
        rt_kprintf("时隙: %d\n", net_info.timeslot);
        rt_kprintf("信号强度: %d\n", net_info.rssi);
        rt_kprintf("重试次数: %d\n", net_info.retry_count);
    }

    if (key_manager_get_upload_info(&key_info) == RT_EOK)
    {
        rt_kprintf("\n=== 按键上传信息 ===\n");
        rt_kprintf("上传状态: %s\n", key_upload_status_to_string(key_info.status));
        rt_kprintf("序列号: %d\n", key_info.sequence);
        rt_kprintf("按键ID: %s\n", key_id_to_string(key_info.key_id));
        rt_kprintf("重试次数: %d\n", key_info.retry_count);
    }
}

/**
 * @brief 执行自检
 */
rt_err_t app_manager_self_test(void)
{
    LOG_I("执行系统自检...");

    /* 检查初始化状态 */
    if (!g_app_ctrl.initialized)
    {
        LOG_E("应用管理器未初始化");
        return -RT_ERROR;
    }

    /* 检查各个模块 */
    // 这里可以添加各个模块的自检代码

    LOG_I("系统自检完成");
    return RT_EOK;
}

/**
 * @}
 */

/* 导出MSH命令 */
static int app_info(int argc, char *argv[])
{
    app_manager_print_info();
    return 0;
}

static int app_reboot(int argc, char *argv[])
{
    rt_uint32_t delay = 1000;
    
    if (argc >= 2)
    {
        delay = atoi(argv[1]);
    }
    
    app_manager_reboot(delay);
    return 0;
}

MSH_CMD_EXPORT(app_info, Print application information);
MSH_CMD_EXPORT(app_reboot, Reboot system [delay_ms]);
