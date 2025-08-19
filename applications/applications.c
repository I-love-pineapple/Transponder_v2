/**
 * @file applications.c
 * @brief 应用程序主逻辑实现文件
 * @details 实现系统状态管理、按键处理、入网流程和按键上传功能
 * @author RT-Thread Team
 * @date 2024-08-18
 * @version 2.0.0
 *
 * @copyright Copyright (c) 2024 RT-Thread Development Team
 *
 * @par 修改日志:
 * <table>
 * <tr><th>日期       <th>版本  <th>作者     <th>说明
 * <tr><td>2024-08-18 <td>2.0.0 <td>RT-Thread Team <td>重新设计应用程序架构
 * </table>
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "led_app.h"
#include "drv_button.h"
#include "protocol.h"
#include "applications.h"
#include "e35_module.h"

#define DBG_TAG "app"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/**
 * @defgroup App_Private_Macros 应用程序私有宏定义
 * @{
 */

/** @brief 应用程序线程优先级 */
#define APP_THREAD_PRIORITY         15

/** @brief 应用程序线程栈大小 */
#define APP_THREAD_STACK_SIZE       2048

/** @brief 应用程序线程时间片 */
#define APP_THREAD_TIMESLICE        10

/** @brief 初始退避窗口 */
#define INITIAL_BACKOFF_WINDOW      8

/** @brief 最大退避窗口 */
#define MAX_BACKOFF_WINDOW          64

/** @brief 按键上传重传间隔（毫秒） */
#define KEY_UPLOAD_RETRY_INTERVAL   100

/** @brief 按键上传最大重传次数 */
#define KEY_UPLOAD_MAX_RETRY        8

/** @brief 按键上传超时时间（毫秒） */
#define KEY_UPLOAD_TIMEOUT          3000

/** @brief 入网超时时间（毫秒） */
#define JOIN_TIMEOUT                30000

/**
 * @}
 */

/**
 * @defgroup App_Private_Types 应用程序私有类型定义
 * @{
 */

/**
 * @brief 按键上传状态结构体
 * @note 用于管理按键上传的状态信息
 */
typedef struct
{
    rt_bool_t is_uploading;         /**< 是否正在上传 */
    rt_uint16_t sequence;           /**< 序列号 */
    rt_uint8_t key_option;          /**< 按键选项 */
    rt_uint8_t retry_count;         /**< 重传次数 */
    rt_tick_t start_time;           /**< 开始时间 */
    rt_timer_t retry_timer;         /**< 重传定时器 */
    rt_uint32_t backoff_window;     /**< 退避窗口 */
    rt_uint8_t restart;             /**< 是否需要重启定时器 */
    rt_uint8_t timer_out;           /**< 定时器是否超时 */
} key_upload_state_t;

/**
 * @brief 应用程序控制结构体
 * @note 用于管理应用程序的全局状态和资源
 */
typedef struct
{
    sys_state_t current_state;      /**< 当前系统状态 */
    rt_mutex_t state_mutex;         /**< 状态互斥锁 */
    rt_sem_t join_sem;              /**< 入网信号量 */
    rt_sem_t key_upload_sem;        /**< 按键上传信号量 */
    rt_thread_t app_thread;         /**< 应用程序线程 */
    key_upload_state_t upload_state; /**< 按键上传状态 */
    rt_bool_t initialized;          /**< 初始化标志 */
} app_control_t;

/**
 * @}
 */

/**
 * @defgroup App_Private_Variables 应用程序私有变量
 * @{
 */

/** @brief 应用程序控制结构体实例 */
static app_control_t g_app_ctrl = {0};

/** @brief 外部函数声明 */
extern rt_err_t press_upload(uint16_t sequence, uint8_t option, uint8_t battery);
extern void e35_set_join_status(rt_uint8_t status);
extern rt_uint8_t e35_get_join_status(void);

/**
 * @defgroup App_Private_Functions 应用程序私有函数
 * @{
 */

/**
 * @brief 获取电池电量百分比
 * @return rt_uint8_t 电池电量百分比（0-100）
 * @note 该函数获取当前电池电量百分比，用于上传数据
 */
static rt_uint8_t app_get_battery_level(void)
{
    /* TODO: 实现真实的电池电量检测 */
    return 85; /* 暂时返回固定值 */
}

/**
 * @brief 设置系统状态
 * @param new_state 新的系统状态
 * @note 该函数线程安全地设置系统状态并更新LED显示
 */
static void app_set_system_state(sys_state_t new_state)
{
    if (new_state >= SYS_STATE_MAX)
    {
        LOG_E("无效的系统状态: %d", new_state);
        return;
    }

    rt_mutex_take(g_app_ctrl.state_mutex, RT_WAITING_FOREVER);

    if (g_app_ctrl.current_state != new_state)
    {
        LOG_I("系统状态切换: %d -> %d", g_app_ctrl.current_state, new_state);
        g_app_ctrl.current_state = new_state;

        /* 更新LED显示状态 */
        switch (new_state)
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
                LOG_W("未处理的系统状态: %d", new_state);
                break;
        }
    }

    rt_mutex_release(g_app_ctrl.state_mutex);
}

/**
 * @brief 按键上传重传定时器回调函数
 * @param parameter 定时器参数
 * @note 该函数在重传定时器超时时被调用，执行按键数据重传
 */
static void app_key_upload_retry_timeout(void *parameter)
{
    key_upload_state_t *upload_state = &g_app_ctrl.upload_state;
    rt_err_t err;

    if (!upload_state->is_uploading)
    {
        rt_timer_stop(upload_state->retry_timer);
        LOG_W("按键上传重传定时器触发，但当前未在上传状态");
        return;
    }

    upload_state->timer_out = RT_TRUE;
}

/**
 * @brief 开始按键上传流程
 * @param key_option 按键选项
 * @return rt_err_t 操作结果
 * @retval RT_EOK 操作成功
 * @retval -RT_ERROR 操作失败
 * @note 该函数启动按键上传流程，包括重传机制
 */
static rt_err_t app_start_key_upload(rt_uint8_t key_option)
{
    key_upload_state_t *upload_state = &g_app_ctrl.upload_state;

    /* 检查当前状态 */
    rt_mutex_take(g_app_ctrl.state_mutex, RT_WAITING_FOREVER);
    if ((g_app_ctrl.current_state != SYS_STATE_JOINED) &&
        (g_app_ctrl.current_state != SYS_STATE_KEY_UPLOADING))
    {
        rt_mutex_release(g_app_ctrl.state_mutex);
        LOG_W("当前状态不允许按键上传，状态: %d", g_app_ctrl.current_state);
        return -RT_ERROR;
    }
    rt_mutex_release(g_app_ctrl.state_mutex);

    /* 检查是否已在上传中 */
    if (upload_state->is_uploading)
    {
        LOG_W("按键上传已在进行中，忽略新的上传请求");
        return -RT_EBUSY;
    }

    /* 初始化上传状态 */
    upload_state->is_uploading = RT_TRUE;
    upload_state->sequence++;
    upload_state->key_option = key_option;
    upload_state->retry_count = 0;
    upload_state->start_time = rt_tick_get();
    upload_state->backoff_window = INITIAL_BACKOFF_WINDOW;

    /* 切换到上传状态 */
    app_set_system_state(SYS_STATE_KEY_UPLOADING);

    LOG_I("开始按键上传，选项:%d，序列号:%d", key_option, upload_state->sequence);

    /* 发送第一次上传 */
    press_upload(upload_state->sequence, upload_state->key_option, app_get_battery_level());

    e35_module_state_t tmp_e35_state;
    e35_get_module_state(&tmp_e35_state);
    /* 窗口*时隙 */
    rt_uint32_t random_delay = get_random(40, upload_state->backoff_window * tmp_e35_state.timeslot);
    LOG_D("下次随机退避延迟: %dms", random_delay);
    rt_timer_control(upload_state->retry_timer, RT_TIMER_CTRL_SET_TIME, &random_delay);
    /* 启动重传定时器 */
    rt_timer_start(upload_state->retry_timer);

    return RT_EOK;
}

/**
 * @brief 处理按键上传应答
 * @note 该函数在收到服务器应答后被调用，停止重传并更新状态
 */
static void app_handle_key_upload_ack(void)
{
    key_upload_state_t *upload_state = &g_app_ctrl.upload_state;

    if (!upload_state->is_uploading)
    {
        LOG_W("收到按键上传应答，但当前未在上传状态");
        return;
    }

    /* 停止重传定时器 */
    rt_timer_stop(upload_state->retry_timer);

    /* 更新状态 */
    upload_state->is_uploading = RT_FALSE;

    LOG_I("按键上传成功，序列号:%d，重传次数:%d",
          upload_state->sequence, upload_state->retry_count);

    /* 切换到上传完成状态 */
    app_set_system_state(SYS_STATE_KEY_UPLOAD_DONE);

    /* 延时后恢复到已入网状态 */
    rt_thread_mdelay(1000);
    app_set_system_state(SYS_STATE_JOINED);
}

/**
 * @brief 按键事件回调函数
 * @param btn 触发事件的按键实体指针
 * @note 该函数处理所有按键事件，包括入网和按键上传
 */
static void app_button_callback(void *btn)
{
    Button_t *button = (Button_t *)btn;
    rt_uint8_t event = Get_Button_Event(button);

    LOG_D("按键 [%s] 事件: %d", button->Name, event);

    /* 处理KEY1长按松开事件 - 触发入网 */
    if (rt_strcmp(button->Name, "key1") == 0 && event == BUTTON_LONG)
    {
        LOG_I("检测到KEY1长按松开，触发入网流程");
        rt_sem_release(g_app_ctrl.join_sem);
        return;
    }

    /* 处理任意按键短按事件 - 触发按键上传 */
    if (event == BUTTON_DOWM)
    {
        rt_uint8_t key_option = 0;

        /* 根据按键名称确定选项值 */
        if (rt_strcmp(button->Name, "key1") == 0)
            key_option = 1;
        else if (rt_strcmp(button->Name, "key2") == 0)
            key_option = 2;
        else if (rt_strcmp(button->Name, "key3") == 0)
            key_option = 3;
        else if (rt_strcmp(button->Name, "key4") == 0)
            key_option = 4;
        else if (rt_strcmp(button->Name, "key5") == 0)
            key_option = 5;
        else if (rt_strcmp(button->Name, "key6") == 0)
            key_option = 6;
        else
        {
            LOG_W("未知按键: %s", button->Name);
            return;
        }

        LOG_I("检测到按键 [%s] 短按，选项:%d", button->Name, key_option);

        /* 启动按键上传 */
        app_start_key_upload(key_option);
    }
}

/**
 * @brief 应用程序主线程入口函数
 * @param parameter 线程参数
 * @note 该函数是应用程序的主线程，负责处理入网流程和状态监控
 */
static void app_main_thread_entry(void *parameter)
{
    rt_err_t result;
    rt_tick_t start_time;
    rt_err_t err;

    LOG_I("应用程序主线程启动");

    /* 初始化为未入网状态 */
    app_set_system_state(SYS_STATE_NOT_JOINED);

    while(1)
    {
        rt_thread_mdelay(10);
        switch (g_app_ctrl.current_state)
        {
            case SYS_STATE_NOT_JOINED:
            {
                /* 等待入网信号量 */
                result = rt_sem_take(g_app_ctrl.join_sem, RT_WAITING_FOREVER);
                if (result != RT_EOK)
                {
                    LOG_E("等待入网信号量失败: %d", result);
                    continue;
                }

                LOG_I("收到入网请求，开始入网流程");

                /* 切换到入网中状态 */
                app_set_system_state(SYS_STATE_JOINING);

                /* 设置E35模块入网状态 */
                e35_set_join_status(1); /* JOIN_STATUS_JOINING */

                /* 等待入网完成或超时 */
                start_time = rt_tick_get();
                break;
            }

            case SYS_STATE_JOINING:
            {
                /* 检查入网状态 */
                rt_uint8_t current_status = e35_get_join_status();

                if (current_status == 2) /* JOIN_STATUS_JOINED */
                {
                    LOG_I("入网成功");
                    app_set_system_state(SYS_STATE_JOINED);
                    break;
                }

                /* 检查超时 */
                rt_tick_t elapsed = rt_tick_get() - start_time;
                if (elapsed * 1000 / RT_TICK_PER_SECOND >= JOIN_TIMEOUT)
                {
                    LOG_W("入网超时，返回未入网状态");
                    e35_set_join_status(0); /* JOIN_STATUS_NONE */
                    app_set_system_state(SYS_STATE_NOT_JOINED);
                    break;
                }
                break;
            }

            case SYS_STATE_JOINED:
            {
                
                break;
            }

            case SYS_STATE_KEY_UPLOADING:
            {
                if (g_app_ctrl.upload_state.timer_out)
                {
                    key_upload_state_t *upload_state = &g_app_ctrl.upload_state;

                    upload_state->timer_out = RT_FALSE;

                    /* 检查是否超过最大重传次数 */
                    if (upload_state->retry_count >= KEY_UPLOAD_MAX_RETRY)
                    {
                        LOG_E("按键上传重传次数超限，上传失败");
                        upload_state->is_uploading = RT_FALSE;
                        app_set_system_state(SYS_STATE_JOINED);
                        break;
                    }

                    /* 检查是否超时 */
                    rt_tick_t elapsed = rt_tick_get() - upload_state->start_time;
                    if (elapsed * 1000 / RT_TICK_PER_SECOND >= KEY_UPLOAD_TIMEOUT)
                    {
                        LOG_E("按键上传超时，上传失败");
                        upload_state->is_uploading = RT_FALSE;
                        app_set_system_state(SYS_STATE_JOINED);
                        break;
                    }

                    /* 执行重传 */
                    upload_state->retry_count++;
                    upload_state->backoff_window = upload_state->backoff_window * 2;
                    if (upload_state->backoff_window > MAX_BACKOFF_WINDOW)
                    {
                        upload_state->backoff_window = MAX_BACKOFF_WINDOW;
                    }
                    LOG_D("按键上传重传，第%d次，序列号:%d", upload_state->retry_count, upload_state->sequence);

                    press_upload(upload_state->sequence, upload_state->key_option, app_get_battery_level());
                    e35_module_state_t tmp_e35_state;
                    e35_get_module_state(&tmp_e35_state);
                    /* 窗口*时隙 */
                    rt_uint32_t random_delay = get_random(40, upload_state->backoff_window * tmp_e35_state.timeslot);
                    LOG_D("下次随机退避延迟: %dms", random_delay);
                    err = rt_timer_control(upload_state->retry_timer, RT_TIMER_CTRL_SET_TIME, &random_delay);
                    if (err != RT_EOK)
                    {
                        LOG_E("设置重传定时器超时失败，错误码: %d", err);
                        return;
                    }
                    /* 重新启动重传定时器 */
                    err = rt_timer_start(upload_state->retry_timer);
                    if (err != RT_EOK)
                    {
                        LOG_E("启动重传定时器失败，错误码: %d", err);
                    }
                }
                break;
            }

            case SYS_STATE_KEY_UPLOAD_DONE:
            {
                break;
            }

            case SYS_STATE_SLEEP:
            {
                break;
            }

            default:
                break;
        }
    }
}

/**
 * @}
 */

/**
 * @defgroup App_Public_Functions 应用程序公共函数
 * @{
 */

/**
 * @brief 应用程序初始化函数
 * @return rt_err_t 初始化结果
 * @retval RT_EOK 初始化成功
 * @retval -RT_ERROR 初始化失败
 * @note 该函数初始化应用程序的所有资源和线程
 */
rt_err_t app_init(void)
{
    rt_err_t result;

    if (g_app_ctrl.initialized)
    {
        LOG_W("应用程序已经初始化");
        return RT_EOK;
    }

    LOG_I("初始化应用程序...");

    /* 初始化控制结构体 */
    rt_memset(&g_app_ctrl, 0, sizeof(g_app_ctrl));
    g_app_ctrl.current_state = SYS_STATE_NOT_JOINED;

    /* 创建状态互斥锁 */
    g_app_ctrl.state_mutex = rt_mutex_create("app_state", RT_IPC_FLAG_PRIO);
    if (g_app_ctrl.state_mutex == RT_NULL)
    {
        LOG_E("创建状态互斥锁失败");
        return -RT_ERROR;
    }

    /* 创建入网信号量 */
    g_app_ctrl.join_sem = rt_sem_create("app_join", 0, RT_IPC_FLAG_PRIO);
    if (g_app_ctrl.join_sem == RT_NULL)
    {
        LOG_E("创建入网信号量失败");
        rt_mutex_delete(g_app_ctrl.state_mutex);
        return -RT_ERROR;
    }

    /* 创建按键上传信号量 */
    g_app_ctrl.key_upload_sem = rt_sem_create("app_upload", 0, RT_IPC_FLAG_PRIO);
    if (g_app_ctrl.key_upload_sem == RT_NULL)
    {
        LOG_E("创建按键上传信号量失败");
        rt_sem_delete(g_app_ctrl.join_sem);
        rt_mutex_delete(g_app_ctrl.state_mutex);
        return -RT_ERROR;
    }

    /* 创建按键上传重传定时器 */
    g_app_ctrl.upload_state.retry_timer = rt_timer_create("upload_retry",
                                                          app_key_upload_retry_timeout,
                                                          RT_NULL,
                                                          KEY_UPLOAD_RETRY_INTERVAL,
                                                          RT_TIMER_FLAG_ONE_SHOT);
    if (g_app_ctrl.upload_state.retry_timer == RT_NULL)
    {
        LOG_E("创建重传定时器失败");
        rt_sem_delete(g_app_ctrl.key_upload_sem);
        rt_sem_delete(g_app_ctrl.join_sem);
        rt_mutex_delete(g_app_ctrl.state_mutex);
        return -RT_ERROR;
    }

    /* 创建应用程序主线程 */
    g_app_ctrl.app_thread = rt_thread_create("app_main",
                                             app_main_thread_entry,
                                             RT_NULL,
                                             APP_THREAD_STACK_SIZE,
                                             APP_THREAD_PRIORITY,
                                             APP_THREAD_TIMESLICE);
    if (g_app_ctrl.app_thread == RT_NULL)
    {
        LOG_E("创建应用程序主线程失败");
        rt_timer_delete(g_app_ctrl.upload_state.retry_timer);
        rt_sem_delete(g_app_ctrl.key_upload_sem);
        rt_sem_delete(g_app_ctrl.join_sem);
        rt_mutex_delete(g_app_ctrl.state_mutex);
        return -RT_ERROR;
    }

    /* 启动主线程 */
    rt_thread_startup(g_app_ctrl.app_thread);

    g_app_ctrl.initialized = RT_TRUE;
    LOG_I("应用程序初始化成功");

    return RT_EOK;
}

/**
 * @brief 注册按键回调函数
 * @return rt_err_t 注册结果
 * @retval RT_EOK 注册成功
 * @retval -RT_ERROR 注册失败
 * @note 该函数为所有按键注册统一的回调函数
 */
rt_err_t app_register_button_callbacks(void)
{
    rt_err_t result;

    LOG_I("注册按键回调函数...");

    /* 为所有按键注册回调函数 */
    result = drv_button_attach_callback("key1", BUTTON_ALL_RIGGER, app_button_callback);
    if (result != RT_EOK)
    {
        LOG_E("注册key1回调失败");
        return -RT_ERROR;
    }

    result = drv_button_attach_callback("key2", BUTTON_ALL_RIGGER, app_button_callback);
    if (result != RT_EOK)
    {
        LOG_E("注册key2回调失败");
        return -RT_ERROR;
    }

    result = drv_button_attach_callback("key3", BUTTON_ALL_RIGGER, app_button_callback);
    if (result != RT_EOK)
    {
        LOG_E("注册key3回调失败");
        return -RT_ERROR;
    }

    result = drv_button_attach_callback("key4", BUTTON_ALL_RIGGER, app_button_callback);
    if (result != RT_EOK)
    {
        LOG_E("注册key4回调失败");
        return -RT_ERROR;
    }

    result = drv_button_attach_callback("key5", BUTTON_ALL_RIGGER, app_button_callback);
    if (result != RT_EOK)
    {
        LOG_E("注册key5回调失败");
        return -RT_ERROR;
    }

    result = drv_button_attach_callback("key6", BUTTON_ALL_RIGGER, app_button_callback);
    if (result != RT_EOK)
    {
        LOG_E("注册key6回调失败");
        return -RT_ERROR;
    }

    LOG_I("按键回调函数注册成功");
    return RT_EOK;
}

/**
 * @brief 处理PRESS_ACK帧应答
 * @note 该函数应在e35-2g4t.c中收到PRESS_ACK帧时被调用
 */
void app_handle_press_ack(void)
{
    LOG_D("收到PRESS_ACK应答");
    app_handle_key_upload_ack();
}

/**
 * @brief 获取当前系统状态
 * @return sys_state_t 当前系统状态
 * @note 该函数线程安全地获取当前系统状态
 */
sys_state_t app_get_system_state(void)
{
    sys_state_t state;

    rt_mutex_take(g_app_ctrl.state_mutex, RT_WAITING_FOREVER);
    state = g_app_ctrl.current_state;
    rt_mutex_release(g_app_ctrl.state_mutex);

    return state;
}

/**
 * @brief 应用程序反初始化函数
 * @return rt_err_t 反初始化结果
 * @retval RT_EOK 反初始化成功
 * @note 该函数清理应用程序的所有资源
 */
rt_err_t app_deinit(void)
{
    if (!g_app_ctrl.initialized)
    {
        LOG_W("应用程序未初始化");
        return RT_EOK;
    }

    LOG_I("反初始化应用程序...");

    /* 停止并删除主线程 */
    if (g_app_ctrl.app_thread != RT_NULL)
    {
        rt_thread_delete(g_app_ctrl.app_thread);
        g_app_ctrl.app_thread = RT_NULL;
    }

    /* 停止并删除重传定时器 */
    if (g_app_ctrl.upload_state.retry_timer != RT_NULL)
    {
        rt_timer_stop(g_app_ctrl.upload_state.retry_timer);
        rt_timer_delete(g_app_ctrl.upload_state.retry_timer);
        g_app_ctrl.upload_state.retry_timer = RT_NULL;
    }

    /* 删除信号量 */
    if (g_app_ctrl.key_upload_sem != RT_NULL)
    {
        rt_sem_delete(g_app_ctrl.key_upload_sem);
        g_app_ctrl.key_upload_sem = RT_NULL;
    }

    if (g_app_ctrl.join_sem != RT_NULL)
    {
        rt_sem_delete(g_app_ctrl.join_sem);
        g_app_ctrl.join_sem = RT_NULL;
    }

    /* 删除互斥锁 */
    if (g_app_ctrl.state_mutex != RT_NULL)
    {
        rt_mutex_delete(g_app_ctrl.state_mutex);
        g_app_ctrl.state_mutex = RT_NULL;
    }

    g_app_ctrl.initialized = RT_FALSE;
    LOG_I("应用程序反初始化完成");

    return RT_EOK;
}

/**
 * @}
 */
