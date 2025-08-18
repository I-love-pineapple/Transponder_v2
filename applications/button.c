#include "drv_button.h"
#include <rtthread.h>
#include <rtdevice.h>

#ifdef PKG_USING_BUTTON

#define DBG_TAG "button"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/**
 * @brief 按键测试回调函数
 * @param btn 触发事件的按键实体指针
 * @note 该函数用于测试按键事件回调功能
 */
static void my_button_callback(void *btn)
{
    Button_t *button = (Button_t *)btn;
    rt_uint8_t event = Get_Button_Event(button);
    
    LOG_D("按键 [%s] 事件: ", button->Name);
    
    switch(event)
    {
        case BUTTON_DOWM:
            LOG_D("按下");
            break;
            
        case BUTTON_UP:
            LOG_D("松开");
            break;
            
        case BUTTON_DOUBLE:
            LOG_D("双击");
            break;
            
        case BUTTON_LONG:
            LOG_D("长按");
            break;
            
        case BUTTON_LONG_FREE:
            LOG_D("长按松开");
            break;
            
        case BUTTON_CONTINUOS:
            LOG_D("持续按下");
            break;
            
        case BUTTON_CONTINUOS_FREE:
            LOG_D("持续按下松开");
            break;
            
        default:
            LOG_D("未知事件(%d)", event);
            break;
    }
}

/**
 * @brief 按键驱动处理线程
 * @param parameter 线程参数
 * @note 该线程周期性调用按键处理函数，用于测试按键事件检测
 */
static void button_process_thread(void *parameter)
{
    while (1)
    {
        drv_button_process();
        rt_thread_mdelay(20); /* 20ms周期调用 */
    }
}

/**
 * @brief 按键驱动初始化函数
 */
rt_err_t my_button_init(void)
{
    rt_err_t result;
    rt_thread_t thread;

    
    /* 按键驱动初始化 */
    result = drv_button_init();
    if (result != RT_EOK)
    {
        LOG_E("按键驱动初始化失败");
        return -RT_ERROR;
    }
    LOG_D("按键驱动初始化成功");
    
    /* 创建按键处理线程 */
    thread = rt_thread_create("btn_proc", 
                              button_process_thread, 
                              RT_NULL,
                              1024, 
                              RT_THREAD_PRIORITY_MAX - 2, 
                              10);
    
    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
        LOG_D("按键处理线程创建成功");
    }
    else
    {
        LOG_E("按键处理线程创建失败");
        return -RT_ERROR;
    }
    
    /* 为所有按键绑定测试回调函数 */
    drv_button_attach_callback("key1", BUTTON_ALL_RIGGER, my_button_callback);
    drv_button_attach_callback("key2", BUTTON_ALL_RIGGER, my_button_callback);
    drv_button_attach_callback("key3", BUTTON_ALL_RIGGER, my_button_callback);
    drv_button_attach_callback("key4", BUTTON_ALL_RIGGER, my_button_callback);
    drv_button_attach_callback("key5", BUTTON_ALL_RIGGER, my_button_callback);
    drv_button_attach_callback("key6", BUTTON_ALL_RIGGER, my_button_callback);
    
    return RT_EOK;
}

#endif /* PKG_USING_BUTTON */
