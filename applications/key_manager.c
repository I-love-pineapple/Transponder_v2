/**
 * @file key_manager.c
 * @brief 按键管理器实现文件
 * @details 提供按键事件处理和数据上传的统一管理接口
 * @author RT-Thread Team
 * @date 2024-12-27
 * @version 1.0.0
 */

#include "key_manager.h"
#include "drv_button.h"

#define DBG_TAG "key_mgr"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/**
 * @defgroup Key_Manager_Private_Variables 私有变量
 * @{
 */

/** @brief 按键上传信息 */
static key_upload_info_t g_upload_info = {0};

/** @brief 初始化标志 */
static rt_bool_t g_key_initialized = RT_FALSE;

/** @brief 按键互斥锁 */
static rt_mutex_t g_key_mutex = RT_NULL;

/** @brief 统计信息 */
static struct {
    rt_uint32_t total_uploads;
    rt_uint32_t success_uploads;
    rt_uint32_t failed_uploads;
} g_key_stats = {0};

/** @brief 事件字符串表 */
static const char* g_event_strings[KEY_EVENT_MAX] = {
    "NONE",
    "PRESS",
    "RELEASE", 
    "LONG_PRESS",
    "DOUBLE_CLICK"
};

/** @brief 按键ID字符串表 */
static const char* g_key_id_strings[KEY_ID_MAX] = {
    "UNKNOWN",
    "KEY1",
    "KEY2",
    "KEY3", 
    "KEY4",
    "KEY5",
    "KEY6"
};

/** @brief 上传状态字符串表 */
static const char* g_upload_status_strings[KEY_UPLOAD_MAX] = {
    "IDLE",
    "BUSY",
    "SUCCESS",
    "FAILED",
    "TIMEOUT"
};

/** @brief 事件回调函数数组 */
#define MAX_KEY_EVENT_CALLBACKS 4
static struct {
    key_event_callback_t callback;
    void *user_data;
    rt_bool_t used;
} g_event_callbacks[MAX_KEY_EVENT_CALLBACKS] = {0};

/** @brief 上传回调函数数组 */
#define MAX_KEY_UPLOAD_CALLBACKS 4
static struct {
    key_upload_callback_t callback;
    void *user_data;
    rt_bool_t used;
} g_upload_callbacks[MAX_KEY_UPLOAD_CALLBACKS] = {0};

/**
 * @}
 */

/**
 * @defgroup Key_Manager_Private_Functions 私有函数
 * @{
 */

/**
 * @brief 通知事件回调
 * @param data 按键数据
 */
static void notify_event_callbacks(const key_data_t *data)
{
    /* 复制回调函数列表（避免长时间持有锁） */
    struct {
        key_event_callback_t callback;
        void *user_data;
        rt_bool_t used;
    } callbacks_copy[MAX_KEY_EVENT_CALLBACKS];
    
    rt_mutex_take(g_key_mutex, RT_WAITING_FOREVER);
    rt_memcpy(callbacks_copy, g_event_callbacks, sizeof(callbacks_copy));
    rt_mutex_release(g_key_mutex);
    
    /* 调用回调函数（不持有锁） */
    for (int i = 0; i < MAX_KEY_EVENT_CALLBACKS; i++)
    {
        if (callbacks_copy[i].used && callbacks_copy[i].callback)
        {
            callbacks_copy[i].callback(data, callbacks_copy[i].user_data);
        }
    }
}

/**
 * @brief 通知上传回调
 * @param info 上传信息
 */
static void notify_upload_callbacks(const key_upload_info_t *info)
{
    /* 复制回调函数列表（避免长时间持有锁） */
    struct {
        key_upload_callback_t callback;
        void *user_data;
        rt_bool_t used;
    } callbacks_copy[MAX_KEY_UPLOAD_CALLBACKS];
    
    rt_mutex_take(g_key_mutex, RT_WAITING_FOREVER);
    rt_memcpy(callbacks_copy, g_upload_callbacks, sizeof(callbacks_copy));
    rt_mutex_release(g_key_mutex);
    
    /* 调用回调函数（不持有锁） */
    for (int i = 0; i < MAX_KEY_UPLOAD_CALLBACKS; i++)
    {
        if (callbacks_copy[i].used && callbacks_copy[i].callback)
        {
            callbacks_copy[i].callback(info, callbacks_copy[i].user_data);
        }
    }
}

/**
 * @brief 按键驱动回调函数
 * @param btn 按键实体
 */
static void key_driver_callback(void *btn)
{
    Button_t *button = (Button_t *)btn;
    rt_uint8_t button_event = Get_Button_Event(button);
    key_data_t key_data = {0};
    
    /* 转换按键名称为ID */
    if (rt_strcmp(button->Name, "key1") == 0)
        key_data.key_id = KEY_ID_1;
    else if (rt_strcmp(button->Name, "key2") == 0)
        key_data.key_id = KEY_ID_2;
    else if (rt_strcmp(button->Name, "key3") == 0)
        key_data.key_id = KEY_ID_3;
    else if (rt_strcmp(button->Name, "key4") == 0)
        key_data.key_id = KEY_ID_4;
    else if (rt_strcmp(button->Name, "key5") == 0)
        key_data.key_id = KEY_ID_5;
    else if (rt_strcmp(button->Name, "key6") == 0)
        key_data.key_id = KEY_ID_6;
    else
        return; /* 未知按键 */
    
    /* 转换按键事件 */
    switch (button_event)
    {
        case BUTTON_DOWM:
            key_data.event = KEY_EVENT_PRESS;
            break;
        case BUTTON_UP:
            key_data.event = KEY_EVENT_RELEASE;
            break;
        case BUTTON_LONG:
            key_data.event = KEY_EVENT_LONG_PRESS;
            break;
        case BUTTON_DOUBLE:
            key_data.event = KEY_EVENT_DOUBLE_CLICK;
            break;
        default:
            return; /* 未知事件 */
    }
    
    key_data.timestamp = rt_tick_get();
    key_data.battery_level = 85; /* TODO: 获取真实电池电量 */
    
    /* 处理按键事件 */
    key_manager_handle_event(key_data.key_id, key_data.event);
}

/**
 * @}
 */

/**
 * @defgroup Key_Manager_Public_Functions 公共函数
 * @{
 */

/**
 * @brief 初始化按键管理器
 */
rt_err_t key_manager_init(void)
{
    rt_err_t result;
    
    if (g_key_initialized)
    {
        LOG_W("按键管理器已经初始化");
        return RT_EOK;
    }
    
    LOG_I("初始化按键管理器...");
    
    /* 创建互斥锁 */
    g_key_mutex = rt_mutex_create("key_mutex", RT_IPC_FLAG_PRIO);
    if (g_key_mutex == RT_NULL)
    {
        LOG_E("创建按键互斥锁失败");
        return -RT_ERROR;
    }
    
    /* 初始化上传信息 */
    rt_memset(&g_upload_info, 0, sizeof(g_upload_info));
    g_upload_info.status = KEY_UPLOAD_IDLE;
    
    /* 注册按键驱动回调 */
    const char* key_names[] = {"key1", "key2", "key3", "key4", "key5", "key6"};
    for (int i = 0; i < 6; i++)
    {
        result = drv_button_attach_callback(key_names[i], BUTTON_ALL_RIGGER, key_driver_callback);
        if (result != RT_EOK)
        {
            LOG_E("注册%s回调失败", key_names[i]);
            rt_mutex_delete(g_key_mutex);
            return -RT_ERROR;
        }
    }
    
    g_key_initialized = RT_TRUE;
    LOG_I("按键管理器初始化成功");
    
    return RT_EOK;
}

/**
 * @brief 反初始化按键管理器
 */
rt_err_t key_manager_deinit(void)
{
    if (!g_key_initialized)
    {
        LOG_W("按键管理器未初始化");
        return RT_EOK;
    }
    
    LOG_I("反初始化按键管理器...");
    
    /* 删除互斥锁 */
    if (g_key_mutex != RT_NULL)
    {
        rt_mutex_delete(g_key_mutex);
        g_key_mutex = RT_NULL;
    }
    
    g_key_initialized = RT_FALSE;
    LOG_I("按键管理器反初始化完成");
    
    return RT_EOK;
}

/**
 * @brief 处理按键事件
 */
rt_err_t key_manager_handle_event(key_id_t key_id, key_event_t event)
{
    key_data_t key_data = {0};
    
    if (!g_key_initialized)
    {
        LOG_E("按键管理器未初始化");
        return -RT_ERROR;
    }
    
    if (key_id >= KEY_ID_MAX || event >= KEY_EVENT_MAX)
    {
        LOG_E("无效的按键参数: key_id=%d, event=%d", key_id, event);
        return -RT_EINVAL;
    }
    
    LOG_D("处理按键事件: %s %s", key_id_to_string(key_id), key_event_to_string(event));
    
    /* 构造按键数据 */
    key_data.key_id = key_id;
    key_data.event = event;
    key_data.timestamp = rt_tick_get();
    key_data.battery_level = 85; /* TODO: 获取真实电池电量 */
    
    /* 通知事件回调 */
    notify_event_callbacks(&key_data);
    
    return RT_EOK;
}

/**
 * @brief 上传按键数据
 */
rt_err_t key_manager_upload(key_id_t key_id)
{
    if (!g_key_initialized)
    {
        LOG_E("按键管理器未初始化");
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_key_mutex, RT_WAITING_FOREVER);
    
    if (g_upload_info.status == KEY_UPLOAD_BUSY)
    {
        rt_mutex_release(g_key_mutex);
        LOG_W("按键上传正在进行中");
        return -RT_EBUSY;
    }
    
    LOG_I("开始上传按键数据: %s", key_id_to_string(key_id));
    
    /* 更新上传信息 */
    g_upload_info.status = KEY_UPLOAD_BUSY;
    g_upload_info.sequence++;
    g_upload_info.key_id = key_id;
    g_upload_info.retry_count = 0;
    g_upload_info.start_time = rt_tick_get();
    g_upload_info.backoff_window = 8; /* 初始退避窗口 */
    
    /* 更新统计 */
    g_key_stats.total_uploads++;
    
    /* 复制上传信息（避免释放互斥量后访问全局变量） */
    key_upload_info_t upload_info_copy = g_upload_info;
    
    rt_mutex_release(g_key_mutex);
    
    /* 通知上传回调 */
    notify_upload_callbacks(&upload_info_copy);
    
    /* TODO: 实际的数据上传逻辑 */
    /* 这里应该调用E35模块的上传函数 */
    
    return RT_EOK;
}

/**
 * @brief 获取上传状态
 */
key_upload_status_t key_manager_get_upload_status(void)
{
    if (!g_key_initialized)
    {
        return KEY_UPLOAD_IDLE;
    }
    
    rt_mutex_take(g_key_mutex, RT_WAITING_FOREVER);
    key_upload_status_t status = g_upload_info.status;
    rt_mutex_release(g_key_mutex);
    
    return status;
}

/**
 * @brief 获取上传信息
 */
rt_err_t key_manager_get_upload_info(key_upload_info_t *info)
{
    if (!g_key_initialized || info == RT_NULL)
    {
        return -RT_EINVAL;
    }
    
    rt_mutex_take(g_key_mutex, RT_WAITING_FOREVER);
    rt_memcpy(info, &g_upload_info, sizeof(key_upload_info_t));
    rt_mutex_release(g_key_mutex);
    
    return RT_EOK;
}

/**
 * @brief 停止当前上传
 */
rt_err_t key_manager_stop_upload(void)
{
    if (!g_key_initialized)
    {
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_key_mutex, RT_WAITING_FOREVER);
    
    if (g_upload_info.status == KEY_UPLOAD_BUSY)
    {
        LOG_I("停止按键上传");
        g_upload_info.status = KEY_UPLOAD_IDLE;
    }
    
    rt_mutex_release(g_key_mutex);
    
    return RT_EOK;
}

/**
 * @brief 处理上传应答
 */
rt_err_t key_manager_handle_upload_ack(rt_uint16_t sequence, rt_uint8_t status)
{
    if (!g_key_initialized)
    {
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_key_mutex, RT_WAITING_FOREVER);
    
    if (g_upload_info.status != KEY_UPLOAD_BUSY || g_upload_info.sequence != sequence)
    {
        rt_mutex_release(g_key_mutex);
        LOG_W("收到不匹配的上传应答: seq=%d, expect=%d", sequence, g_upload_info.sequence);
        return -RT_EINVAL;
    }
    
    LOG_I("收到上传应答: seq=%d, status=%d", sequence, status);
    
    if (status == 0) /* 成功 */
    {
        g_upload_info.status = KEY_UPLOAD_SUCCESS;
        g_key_stats.success_uploads++;
    }
    else /* 失败 */
    {
        g_upload_info.status = KEY_UPLOAD_FAILED;
        g_key_stats.failed_uploads++;
    }
    
    rt_mutex_release(g_key_mutex);
    
    /* 通知上传回调 */
    notify_upload_callbacks(&g_upload_info);
    
    return RT_EOK;
}

/**
 * @brief 注册按键事件回调函数
 */
rt_err_t key_manager_register_event_callback(key_event_callback_t callback, void *user_data)
{
    if (!g_key_initialized || callback == RT_NULL)
    {
        return -RT_EINVAL;
    }
    
    rt_mutex_take(g_key_mutex, RT_WAITING_FOREVER);
    
    for (int i = 0; i < MAX_KEY_EVENT_CALLBACKS; i++)
    {
        if (!g_event_callbacks[i].used)
        {
            g_event_callbacks[i].callback = callback;
            g_event_callbacks[i].user_data = user_data;
            g_event_callbacks[i].used = RT_TRUE;
            rt_mutex_release(g_key_mutex);
            LOG_D("注册按键事件回调 [%d]", i);
            return RT_EOK;
        }
    }
    
    rt_mutex_release(g_key_mutex);
    LOG_E("按键事件回调已满");
    return -RT_EFULL;
}

/**
 * @brief 注册上传状态回调函数
 */
rt_err_t key_manager_register_upload_callback(key_upload_callback_t callback, void *user_data)
{
    if (!g_key_initialized || callback == RT_NULL)
    {
        return -RT_EINVAL;
    }
    
    rt_mutex_take(g_key_mutex, RT_WAITING_FOREVER);
    
    for (int i = 0; i < MAX_KEY_UPLOAD_CALLBACKS; i++)
    {
        if (!g_upload_callbacks[i].used)
        {
            g_upload_callbacks[i].callback = callback;
            g_upload_callbacks[i].user_data = user_data;
            g_upload_callbacks[i].used = RT_TRUE;
            rt_mutex_release(g_key_mutex);
            LOG_D("注册按键上传回调 [%d]", i);
            return RT_EOK;
        }
    }
    
    rt_mutex_release(g_key_mutex);
    LOG_E("按键上传回调已满");
    return -RT_EFULL;
}

/**
 * @brief 注销按键事件回调函数
 */
rt_err_t key_manager_unregister_event_callback(key_event_callback_t callback)
{
    if (!g_key_initialized || callback == RT_NULL)
    {
        return -RT_EINVAL;
    }
    
    rt_mutex_take(g_key_mutex, RT_WAITING_FOREVER);
    
    for (int i = 0; i < MAX_KEY_EVENT_CALLBACKS; i++)
    {
        if (g_event_callbacks[i].used && g_event_callbacks[i].callback == callback)
        {
            g_event_callbacks[i].used = RT_FALSE;
            g_event_callbacks[i].callback = RT_NULL;
            g_event_callbacks[i].user_data = RT_NULL;
            rt_mutex_release(g_key_mutex);
            LOG_D("注销按键事件回调 [%d]", i);
            return RT_EOK;
        }
    }
    
    rt_mutex_release(g_key_mutex);
    LOG_W("未找到要注销的事件回调函数");
    return -RT_EINVAL;
}

/**
 * @brief 注销上传状态回调函数
 */
rt_err_t key_manager_unregister_upload_callback(key_upload_callback_t callback)
{
    if (!g_key_initialized || callback == RT_NULL)
    {
        return -RT_EINVAL;
    }
    
    rt_mutex_take(g_key_mutex, RT_WAITING_FOREVER);
    
    for (int i = 0; i < MAX_KEY_UPLOAD_CALLBACKS; i++)
    {
        if (g_upload_callbacks[i].used && g_upload_callbacks[i].callback == callback)
        {
            g_upload_callbacks[i].used = RT_FALSE;
            g_upload_callbacks[i].callback = RT_NULL;
            g_upload_callbacks[i].user_data = RT_NULL;
            rt_mutex_release(g_key_mutex);
            LOG_D("注销按键上传回调 [%d]", i);
            return RT_EOK;
        }
    }
    
    rt_mutex_release(g_key_mutex);
    LOG_W("未找到要注销的上传回调函数");
    return -RT_EINVAL;
}

/**
 * @brief 获取按键事件字符串
 */
const char* key_event_to_string(key_event_t event)
{
    if (event >= KEY_EVENT_MAX)
    {
        return "UNKNOWN";
    }
    
    return g_event_strings[event];
}

/**
 * @brief 获取按键ID字符串
 */
const char* key_id_to_string(key_id_t key_id)
{
    if (key_id >= KEY_ID_MAX)
    {
        return "UNKNOWN";
    }
    
    return g_key_id_strings[key_id];
}

/**
 * @brief 获取上传状态字符串
 */
const char* key_upload_status_to_string(key_upload_status_t status)
{
    if (status >= KEY_UPLOAD_MAX)
    {
        return "UNKNOWN";
    }
    
    return g_upload_status_strings[status];
}

/**
 * @brief 获取统计信息
 */
rt_err_t key_manager_get_statistics(rt_uint32_t *total_uploads, rt_uint32_t *success_uploads, rt_uint32_t *failed_uploads)
{
    if (!g_key_initialized)
    {
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_key_mutex, RT_WAITING_FOREVER);
    
    if (total_uploads) *total_uploads = g_key_stats.total_uploads;
    if (success_uploads) *success_uploads = g_key_stats.success_uploads;
    if (failed_uploads) *failed_uploads = g_key_stats.failed_uploads;
    
    rt_mutex_release(g_key_mutex);
    
    return RT_EOK;
}

/**
 * @brief 重置统计信息
 */
rt_err_t key_manager_reset_statistics(void)
{
    if (!g_key_initialized)
    {
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_key_mutex, RT_WAITING_FOREVER);
    rt_memset(&g_key_stats, 0, sizeof(g_key_stats));
    rt_mutex_release(g_key_mutex);
    
    LOG_I("按键统计信息已重置");
    return RT_EOK;
}

/**
 * @}
 */

/* MSH命令 */
static int cmd_key(int argc, char *argv[])
{
    if (argc < 2)
    {
        rt_kprintf("用法: key <status|upload|stop|stats|reset>\n");
        return 0;
    }
    
    if (rt_strcmp(argv[1], "status") == 0)
    {
        key_upload_status_t status = key_manager_get_upload_status();
        rt_kprintf("上传状态: %s\n", key_upload_status_to_string(status));
    }
    else if (rt_strcmp(argv[1], "upload") == 0)
    {
        key_id_t key_id = KEY_ID_1;
        if (argc >= 3)
        {
            key_id = (key_id_t)atoi(argv[2]);
            if (key_id < KEY_ID_1 || key_id >= KEY_ID_MAX)
            {
                rt_kprintf("无效的按键ID: %d\n", key_id);
                return 0;
            }
        }
        
        rt_err_t result = key_manager_upload(key_id);
        rt_kprintf("上传按键 %s: %s\n", key_id_to_string(key_id), result == RT_EOK ? "成功" : "失败");
    }
    else if (rt_strcmp(argv[1], "stop") == 0)
    {
        key_manager_stop_upload();
        rt_kprintf("停止上传\n");
    }
    else if (rt_strcmp(argv[1], "stats") == 0)
    {
        rt_uint32_t total, success, failed;
        if (key_manager_get_statistics(&total, &success, &failed) == RT_EOK)
        {
            rt_kprintf("=== 按键统计 ===\n");
            rt_kprintf("总上传次数: %d\n", total);
            rt_kprintf("成功次数: %d\n", success);
            rt_kprintf("失败次数: %d\n", failed);
            if (total > 0)
            {
                rt_kprintf("成功率: %d%%\n", (success * 100) / total);
            }
        }
        else
        {
            rt_kprintf("获取统计信息失败\n");
        }
    }
    else if (rt_strcmp(argv[1], "reset") == 0)
    {
        key_manager_reset_statistics();
        rt_kprintf("统计信息已重置\n");
    }
    else
    {
        rt_kprintf("未知命令: %s\n", argv[1]);
    }
    
    return 0;
}

MSH_CMD_EXPORT(cmd_key, Key management: status|upload|stop|stats|reset);
