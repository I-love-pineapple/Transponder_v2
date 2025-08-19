/**
 * @file system_state.c
 * @brief 系统状态管理器实现文件
 * @details 实现系统状态机的统一管理功能
 * @author RT-Thread Team
 * @date 2024-12-27
 * @version 1.0.0
 * 
 * @copyright Copyright (c) 2024 RT-Thread Development Team
 * 
 * @par 修改日志:
 * <table>
 * <tr><th>日期       <th>版本  <th>作者     <th>说明
 * <tr><td>2024-12-27 <td>1.0.0 <td>RT-Thread Team <td>创建系统状态管理器
 * </table>
 */

#include "system_state.h"
#include "led_app.h"

#define DBG_TAG "sys_state"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/**
 * @defgroup System_State_Private_Macros 私有宏定义
 * @{
 */

/** @brief 最大回调函数数量 */
#define MAX_STATE_CALLBACKS     8

/**
 * @}
 */

/**
 * @defgroup System_State_Private_Types 私有类型定义
 * @{
 */

/**
 * @brief 状态回调信息结构体
 */
typedef struct
{
    state_change_callback_t callback;   /**< 回调函数 */
    void *user_data;                    /**< 用户数据 */
    rt_bool_t used;                     /**< 是否已使用 */
} state_callback_info_t;

/**
 * @brief 系统状态管理器控制结构体
 */
typedef struct
{
    sys_state_t current_state;          /**< 当前状态 */
    rt_mutex_t state_mutex;             /**< 状态互斥锁 */
    rt_mailbox_t event_mailbox;         /**< 事件邮箱 */
    rt_thread_t state_thread;           /**< 状态处理线程 */
    rt_bool_t initialized;              /**< 初始化标志 */
    state_callback_info_t callbacks[MAX_STATE_CALLBACKS]; /**< 回调函数数组 */
} system_state_ctrl_t;

/**
 * @brief 事件消息结构体
 */
typedef struct
{
    sys_event_t event;                  /**< 事件类型 */
    void *data;                         /**< 事件数据 */
} event_message_t;

/**
 * @}
 */

/**
 * @defgroup System_State_Private_Variables 私有变量
 * @{
 */

/** @brief 系统状态管理器控制结构体实例 */
static system_state_ctrl_t g_state_ctrl = {0};

/** @brief 状态转换表 */
static const rt_bool_t g_state_transition_table[SYS_STATE_MAX][SYS_STATE_MAX] = {
    /* FROM\TO    INIT  NOT_JOINED  JOINING  JOINED  KEY_UP  KEY_DONE  SLEEP  ERROR */
    /* INIT     */ {0,   1,          0,       0,      0,      0,        0,     1},
    /* NOT_JOIN */ {0,   1,          1,       0,      0,      0,        1,     1},
    /* JOINING  */ {0,   1,          1,       1,      0,      0,        0,     1},
    /* JOINED   */ {0,   1,          0,       1,      1,      0,        1,     1},
    /* KEY_UP   */ {0,   0,          0,       1,      1,      1,        0,     1},
    /* KEY_DONE */ {0,   0,          0,       1,      0,      1,        0,     1},
    /* SLEEP    */ {0,   1,          0,       0,      0,      0,        1,     1},
    /* ERROR    */ {1,   1,          0,       0,      0,      0,        0,     1}
};

/** @brief 状态字符串表 */
static const char* g_state_strings[SYS_STATE_MAX] = {
    "INIT",
    "NOT_JOINED",
    "JOINING",
    "JOINED",
    "KEY_UPLOADING",
    "KEY_UPLOAD_DONE",
    "SLEEP",
    "ERROR"
};

/** @brief 事件字符串表 */
static const char* g_event_strings[SYS_EVENT_MAX] = {
    "INIT_COMPLETE",
    "JOIN_REQUEST",
    "JOIN_SUCCESS",
    "JOIN_FAILED",
    "LEAVE_NETWORK",
    "KEY_PRESS",
    "KEY_UPLOAD_START",
    "KEY_UPLOAD_SUCCESS",
    "KEY_UPLOAD_FAILED",
    "ENTER_SLEEP",
    "WAKEUP",
    "ERROR"
};

/**
 * @}
 */

/**
 * @defgroup System_State_Private_Functions 私有函数
 * @{
 */

/**
 * @brief 更新LED状态
 * @param state 系统状态
 */
static void update_led_status(sys_state_t state)
{
    switch (state)
    {
        case SYS_STATE_NOT_JOINED:
            led_app_switch(LED_STATUS_NOT_JOINED);
            break;
        case SYS_STATE_JOINING:
            led_app_switch(LED_STATUS_JOINING);
            break;
        case SYS_STATE_JOINED:
            led_app_switch(LED_STATUS_JOINED);
            break;
        case SYS_STATE_KEY_UPLOADING:
            led_app_switch(LED_STATUS_KEY_UPLOADING);
            break;
        case SYS_STATE_KEY_UPLOAD_DONE:
            led_app_switch(LED_STATUS_KEY_UPLOAD_DONE);
            break;
        case SYS_STATE_SLEEP:
            led_app_switch(LED_STATUS_SLEEP);
            break;
        default:
            LOG_W("未处理的LED状态: %d", state);
            break;
    }
}

/**
 * @brief 通知状态变化回调函数
 * @param old_state 旧状态
 * @param new_state 新状态
 */
static void notify_state_change_callbacks(sys_state_t old_state, sys_state_t new_state)
{
    for (int i = 0; i < MAX_STATE_CALLBACKS; i++)
    {
        if (g_state_ctrl.callbacks[i].used && g_state_ctrl.callbacks[i].callback)
        {
            g_state_ctrl.callbacks[i].callback(old_state, new_state, g_state_ctrl.callbacks[i].user_data);
        }
    }
}

/**
 * @brief 处理状态转换
 * @param new_state 新状态
 * @return rt_err_t 处理结果
 */
static rt_err_t handle_state_transition(sys_state_t new_state)
{
    sys_state_t old_state;

    if (new_state >= SYS_STATE_MAX)
    {
        LOG_E("无效的系统状态: %d", new_state);
        return -RT_EINVAL;
    }

    rt_mutex_take(g_state_ctrl.state_mutex, RT_WAITING_FOREVER);

    old_state = g_state_ctrl.current_state;

    /* 检查状态转换是否合法 */
    if (!g_state_transition_table[old_state][new_state])
    {
        rt_mutex_release(g_state_ctrl.state_mutex);
        LOG_E("非法的状态转换: %s -> %s", 
              g_state_strings[old_state], g_state_strings[new_state]);
        return -RT_ERROR;
    }

    /* 状态相同，无需转换 */
    if (old_state == new_state)
    {
        rt_mutex_release(g_state_ctrl.state_mutex);
        return RT_EOK;
    }

    /* 执行状态转换 */
    g_state_ctrl.current_state = new_state;
    
    rt_mutex_release(g_state_ctrl.state_mutex);

    LOG_I("系统状态转换: %s -> %s", g_state_strings[old_state], g_state_strings[new_state]);

    /* 更新LED状态 */
    update_led_status(new_state);

    /* 通知回调函数 */
    notify_state_change_callbacks(old_state, new_state);

    return RT_EOK;
}

/**
 * @brief 处理系统事件
 * @param event 系统事件
 * @param data 事件数据
 * @return rt_err_t 处理结果
 */
static rt_err_t handle_system_event(sys_event_t event, void *data)
{
    sys_state_t current_state, target_state;

    /* 获取当前状态（线程安全） */
    rt_mutex_take(g_state_ctrl.state_mutex, RT_WAITING_FOREVER);
    current_state = g_state_ctrl.current_state;
    rt_mutex_release(g_state_ctrl.state_mutex);
    
    target_state = current_state;

    LOG_D("处理系统事件: %s (当前状态: %s)", 
          g_event_strings[event], g_state_strings[current_state]);

    /* 根据当前状态和事件确定目标状态 */
    switch (current_state)
    {
        case SYS_STATE_INIT:
            if (event == SYS_EVENT_INIT_COMPLETE)
                target_state = SYS_STATE_NOT_JOINED;
            else if (event == SYS_EVENT_ERROR)
                target_state = SYS_STATE_ERROR;
            break;

        case SYS_STATE_NOT_JOINED:
            if (event == SYS_EVENT_JOIN_REQUEST)
                target_state = SYS_STATE_JOINING;
            else if (event == SYS_EVENT_ENTER_SLEEP)
                target_state = SYS_STATE_SLEEP;
            else if (event == SYS_EVENT_ERROR)
                target_state = SYS_STATE_ERROR;
            break;

        case SYS_STATE_JOINING:
            if (event == SYS_EVENT_JOIN_SUCCESS)
                target_state = SYS_STATE_JOINED;
            else if (event == SYS_EVENT_JOIN_FAILED)
                target_state = SYS_STATE_NOT_JOINED;
            else if (event == SYS_EVENT_LEAVE_NETWORK)
                target_state = SYS_STATE_NOT_JOINED;
            else if (event == SYS_EVENT_ERROR)
                target_state = SYS_STATE_ERROR;
            break;

        case SYS_STATE_JOINED:
            if (event == SYS_EVENT_KEY_UPLOAD_START)
                target_state = SYS_STATE_KEY_UPLOADING;
            else if (event == SYS_EVENT_LEAVE_NETWORK)
                target_state = SYS_STATE_NOT_JOINED;
            else if (event == SYS_EVENT_ENTER_SLEEP)
                target_state = SYS_STATE_SLEEP;
            else if (event == SYS_EVENT_ERROR)
                target_state = SYS_STATE_ERROR;
            break;

        case SYS_STATE_KEY_UPLOADING:
            if (event == SYS_EVENT_KEY_UPLOAD_SUCCESS)
                target_state = SYS_STATE_KEY_UPLOAD_DONE;
            else if (event == SYS_EVENT_KEY_UPLOAD_FAILED)
                target_state = SYS_STATE_JOINED;
            else if (event == SYS_EVENT_ERROR)
                target_state = SYS_STATE_ERROR;
            break;

        case SYS_STATE_KEY_UPLOAD_DONE:
            /* 自动回到已入网状态 */
            target_state = SYS_STATE_JOINED;
            break;

        case SYS_STATE_SLEEP:
            if (event == SYS_EVENT_WAKEUP)
                target_state = SYS_STATE_NOT_JOINED;
            else if (event == SYS_EVENT_ERROR)
                target_state = SYS_STATE_ERROR;
            break;

        case SYS_STATE_ERROR:
            if (event == SYS_EVENT_INIT_COMPLETE)
                target_state = SYS_STATE_NOT_JOINED;
            break;

        default:
            LOG_W("未处理的状态: %d", current_state);
            break;
    }

    /* 执行状态转换 */
    if (target_state != current_state)
    {
        return handle_state_transition(target_state);
    }

    return RT_EOK;
}

/**
 * @brief 状态处理线程入口函数
 * @param parameter 线程参数
 */
static void state_thread_entry(void *parameter)
{
    rt_err_t result;
    rt_ubase_t msg_ptr;
    event_message_t *msg;

    LOG_I("系统状态处理线程启动");

    while (1)
    {
        /* 等待事件 */
        result = rt_mb_recv(g_state_ctrl.event_mailbox, &msg_ptr, RT_WAITING_FOREVER);
        if (result != RT_EOK)
        {
            LOG_E("接收事件失败: %d", result);
            continue;
        }

        /* 获取消息指针 */
        msg = (event_message_t *)msg_ptr;
        if (msg == RT_NULL)
        {
            LOG_E("接收到空消息");
            continue;
        }

        /* 处理事件 */
        handle_system_event(msg->event, msg->data);
        
        /* 释放消息内存 */
        rt_free(msg);
    }
}

/**
 * @}
 */

/**
 * @defgroup System_State_Public_Functions 公共函数
 * @{
 */

/**
 * @brief 初始化系统状态管理器
 */
rt_err_t system_state_init(void)
{
    rt_err_t result;

    if (g_state_ctrl.initialized)
    {
        LOG_W("系统状态管理器已经初始化");
        return RT_EOK;
    }

    LOG_I("初始化系统状态管理器...");

    /* 初始化控制结构体 */
    rt_memset(&g_state_ctrl, 0, sizeof(g_state_ctrl));
    g_state_ctrl.current_state = SYS_STATE_INIT;

    /* 创建状态互斥锁 */
    g_state_ctrl.state_mutex = rt_mutex_create("state_mutex", RT_IPC_FLAG_PRIO);
    if (g_state_ctrl.state_mutex == RT_NULL)
    {
        LOG_E("创建状态互斥锁失败");
        return -RT_ERROR;
    }

    /* 创建事件邮箱 */
    g_state_ctrl.event_mailbox = rt_mb_create("state_mb", 32, RT_IPC_FLAG_PRIO);
    if (g_state_ctrl.event_mailbox == RT_NULL)
    {
        LOG_E("创建事件邮箱失败");
        rt_mutex_delete(g_state_ctrl.state_mutex);
        return -RT_ERROR;
    }

    /* 创建状态处理线程 */
    g_state_ctrl.state_thread = rt_thread_create("state_mgr",
                                                  state_thread_entry,
                                                  RT_NULL,
                                                  1024,
                                                  15,
                                                  10);
    if (g_state_ctrl.state_thread == RT_NULL)
    {
        LOG_E("创建状态处理线程失败");
        rt_mb_delete(g_state_ctrl.event_mailbox);
        rt_mutex_delete(g_state_ctrl.state_mutex);
        return -RT_ERROR;
    }

    /* 启动状态处理线程 */
    rt_thread_startup(g_state_ctrl.state_thread);

    g_state_ctrl.initialized = RT_TRUE;
    LOG_I("系统状态管理器初始化成功");

    return RT_EOK;
}

/**
 * @brief 反初始化系统状态管理器
 */
rt_err_t system_state_deinit(void)
{
    if (!g_state_ctrl.initialized)
    {
        LOG_W("系统状态管理器未初始化");
        return RT_EOK;
    }

    LOG_I("反初始化系统状态管理器...");

    /* 删除线程 */
    if (g_state_ctrl.state_thread != RT_NULL)
    {
        rt_thread_delete(g_state_ctrl.state_thread);
        g_state_ctrl.state_thread = RT_NULL;
    }

    /* 清理邮箱中未处理的消息（防止内存泄漏） */
    if (g_state_ctrl.event_mailbox != RT_NULL)
    {
        rt_ubase_t msg_ptr;
        event_message_t *msg;
        
        /* 清理所有未处理的消息 */
        while (rt_mb_recv(g_state_ctrl.event_mailbox, &msg_ptr, 0) == RT_EOK)
        {
            msg = (event_message_t *)msg_ptr;
            if (msg != RT_NULL)
            {
                rt_free(msg);
                LOG_D("清理未处理的事件消息");
            }
        }
        
        rt_mb_delete(g_state_ctrl.event_mailbox);
        g_state_ctrl.event_mailbox = RT_NULL;
    }

    /* 删除互斥锁 */
    if (g_state_ctrl.state_mutex != RT_NULL)
    {
        rt_mutex_delete(g_state_ctrl.state_mutex);
        g_state_ctrl.state_mutex = RT_NULL;
    }

    g_state_ctrl.initialized = RT_FALSE;
    LOG_I("系统状态管理器反初始化完成");

    return RT_EOK;
}

/**
 * @brief 获取当前系统状态
 */
sys_state_t system_state_get(void)
{
    sys_state_t state;

    if (!g_state_ctrl.initialized)
    {
        return SYS_STATE_INIT;
    }

    rt_mutex_take(g_state_ctrl.state_mutex, RT_WAITING_FOREVER);
    state = g_state_ctrl.current_state;
    rt_mutex_release(g_state_ctrl.state_mutex);

    return state;
}

/**
 * @brief 设置系统状态
 */
rt_err_t system_state_set(sys_state_t new_state)
{
    if (!g_state_ctrl.initialized)
    {
        LOG_E("系统状态管理器未初始化");
        return -RT_ERROR;
    }

    return handle_state_transition(new_state);
}

/**
 * @brief 发送系统事件
 */
rt_err_t system_state_post_event(sys_event_t event, void *data)
{
    event_message_t *msg;
    rt_err_t result;

    if (!g_state_ctrl.initialized)
    {
        LOG_E("系统状态管理器未初始化");
        return -RT_ERROR;
    }

    if (event >= SYS_EVENT_MAX)
    {
        LOG_E("无效的系统事件: %d", event);
        return -RT_EINVAL;
    }

    /* 分配消息内存 */
    msg = (event_message_t *)rt_malloc(sizeof(event_message_t));
    if (msg == RT_NULL)
    {
        LOG_E("分配消息内存失败");
        return -RT_ENOMEM;
    }

    msg->event = event;
    msg->data = data;

    result = rt_mb_send(g_state_ctrl.event_mailbox, (rt_ubase_t)msg);
    if (result != RT_EOK)
    {
        LOG_E("发送系统事件失败: %d", result);
        rt_free(msg);
        return result;
    }

    LOG_D("发送系统事件: %s", g_event_strings[event]);
    return RT_EOK;
}

/**
 * @brief 注册状态变化回调函数
 */
rt_err_t system_state_register_callback(state_change_callback_t callback, void *user_data)
{
    if (!g_state_ctrl.initialized || callback == RT_NULL)
    {
        return -RT_EINVAL;
    }

    for (int i = 0; i < MAX_STATE_CALLBACKS; i++)
    {
        if (!g_state_ctrl.callbacks[i].used)
        {
            g_state_ctrl.callbacks[i].callback = callback;
            g_state_ctrl.callbacks[i].user_data = user_data;
            g_state_ctrl.callbacks[i].used = RT_TRUE;
            LOG_D("注册状态变化回调函数 [%d]", i);
            return RT_EOK;
        }
    }

    LOG_E("状态变化回调函数已满");
    return -RT_EFULL;
}

/**
 * @brief 注销状态变化回调函数
 */
rt_err_t system_state_unregister_callback(state_change_callback_t callback)
{
    if (!g_state_ctrl.initialized || callback == RT_NULL)
    {
        return -RT_EINVAL;
    }

    for (int i = 0; i < MAX_STATE_CALLBACKS; i++)
    {
        if (g_state_ctrl.callbacks[i].used && g_state_ctrl.callbacks[i].callback == callback)
        {
            g_state_ctrl.callbacks[i].used = RT_FALSE;
            g_state_ctrl.callbacks[i].callback = RT_NULL;
            g_state_ctrl.callbacks[i].user_data = RT_NULL;
            LOG_D("注销状态变化回调函数 [%d]", i);
            return RT_EOK;
        }
    }

    LOG_W("未找到要注销的回调函数");
    return -RT_EINVAL;
}

/**
 * @brief 检查状态转换是否合法
 */
rt_bool_t system_state_is_transition_valid(sys_state_t from, sys_state_t to)
{
    if (from >= SYS_STATE_MAX || to >= SYS_STATE_MAX)
    {
        return RT_FALSE;
    }

    return g_state_transition_table[from][to];
}

/**
 * @brief 获取状态字符串
 */
const char* system_state_to_string(sys_state_t state)
{
    if (state >= SYS_STATE_MAX)
    {
        return "UNKNOWN";
    }

    return g_state_strings[state];
}

/**
 * @brief 获取事件字符串
 */
const char* system_event_to_string(sys_event_t event)
{
    if (event >= SYS_EVENT_MAX)
    {
        return "UNKNOWN";
    }

    return g_event_strings[event];
}

/**
 * @}
 */
