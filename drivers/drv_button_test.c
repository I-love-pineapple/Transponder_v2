/**
 * @file drv_button_test.c
 * @brief 按键驱动测试文件
 * @details 提供按键驱动的测试函数和示例代码
 * @author RT-Thread Team
 * @date 2024-08-01
 * @version 1.0.0
 * 
 * @copyright Copyright (c) 2024 RT-Thread Development Team
 * 
 * @par 修改日志:
 * <table>
 * <tr><th>日期       <th>版本  <th>作者     <th>说明
 * <tr><td>2024-08-01 <td>1.0.0 <td>RT-Thread Team <td>首次创建
 * </table>
 */

#include "drv_button.h"
#include <rtthread.h>
#include <rtdevice.h>

#ifdef PKG_USING_BUTTON

/**
 * @defgroup Button_Test_Functions 按键测试函数
 * @{
 */

/**
 * @brief 按键测试回调函数
 * @param btn 触发事件的按键实体指针
 * @note 该函数用于测试按键事件回调功能
 */
static void test_button_callback(void *btn)
{
    Button_t *button = (Button_t *)btn;
    rt_uint8_t event = Get_Button_Event(button);
    
    rt_kprintf("[TEST] Button [%s] event: ", button->Name);
    
    switch(event)
    {
        case BUTTON_DOWM:
            rt_kprintf("PRESSED\n");
            break;
            
        case BUTTON_UP:
            rt_kprintf("RELEASED\n");
            break;
            
        case BUTTON_DOUBLE:
            rt_kprintf("DOUBLE CLICKED\n");
            break;
            
        case BUTTON_LONG:
            rt_kprintf("LONG PRESSED\n");
            break;
            
        case BUTTON_LONG_FREE:
            rt_kprintf("LONG PRESS RELEASED\n");
            break;
            
        case BUTTON_CONTINUOS:
            rt_kprintf("CONTINUOUS PRESSED\n");
            break;
            
        case BUTTON_CONTINUOS_FREE:
            rt_kprintf("CONTINUOUS PRESS RELEASED\n");
            break;
            
        default:
            rt_kprintf("UNKNOWN EVENT (%d)\n", event);
            break;
    }
}

/**
 * @brief 按键驱动基本功能测试
 * @return rt_err_t 测试结果
 * @retval RT_EOK 测试通过
 * @retval -RT_ERROR 测试失败
 * @note 该函数测试按键驱动的基本初始化和配置功能
 */
static rt_err_t button_basic_test(void)
{
    rt_err_t result;
    
    rt_kprintf("[TEST] Starting button basic test...\n");
    
    /* 测试按键驱动初始化 */
    result = drv_button_init();
    if (result != RT_EOK)
    {
        rt_kprintf("[TEST] Button driver init failed!\n");
        return -RT_ERROR;
    }
    rt_kprintf("[TEST] Button driver init: PASS\n");
    
    /* 测试获取按键句柄 */
    Button_t *btn1 = drv_button_get_handle("key1");
    Button_t *btn2 = drv_button_get_handle("key2");
    Button_t *btn_invalid = drv_button_get_handle("invalid_key");
    
    if (btn1 == RT_NULL || btn2 == RT_NULL)
    {
        rt_kprintf("[TEST] Get button handle failed!\n");
        return -RT_ERROR;
    }
    
    if (btn_invalid != RT_NULL)
    {
        rt_kprintf("[TEST] Invalid button handle should be NULL!\n");
        return -RT_ERROR;
    }
    rt_kprintf("[TEST] Get button handle: PASS\n");
    
    /* 测试绑定回调函数 */
    result = drv_button_attach_callback("key1", BUTTON_ALL_RIGGER, test_button_callback);
    if (result != RT_EOK)
    {
        rt_kprintf("[TEST] Attach callback failed!\n");
        return -RT_ERROR;
    }
    rt_kprintf("[TEST] Attach callback: PASS\n");
    
    /* 测试获取按键状态 */
    rt_uint8_t state = drv_button_get_state("key1");
    rt_kprintf("[TEST] Key1 current state: %d\n", state);
    
    /* 测试获取按键事件 */
    rt_uint8_t event = drv_button_get_event("key1");
    rt_kprintf("[TEST] Key1 current event: %d\n", event);
    
    rt_kprintf("[TEST] Button basic test: PASS\n");
    return RT_EOK;
}

/**
 * @brief 按键驱动处理线程
 * @param parameter 线程参数
 * @note 该线程周期性调用按键处理函数，用于测试按键事件检测
 */
static void button_process_thread(void *parameter)
{
    rt_kprintf("[TEST] Button process thread started\n");
    
    while (1)
    {
        drv_button_process();
        rt_thread_mdelay(20); /* 20ms周期调用 */
    }
}

/**
 * @brief 按键驱动完整测试函数
 * @return rt_err_t 测试结果
 * @retval RT_EOK 测试通过
 * @retval -RT_ERROR 测试失败
 * @note 该函数执行完整的按键驱动测试，包括初始化、事件检测等
 */
rt_err_t drv_button_full_test(void)
{
    rt_err_t result;
    rt_thread_t thread;
    
    rt_kprintf("\n=== Button Driver Full Test ===\n");
    
    /* 执行基本功能测试 */
    result = button_basic_test();
    if (result != RT_EOK)
    {
        rt_kprintf("[TEST] Basic test failed!\n");
        return -RT_ERROR;
    }
    
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
        rt_kprintf("[TEST] Button process thread created\n");
    }
    else
    {
        rt_kprintf("[TEST] Failed to create button process thread!\n");
        return -RT_ERROR;
    }
    
    /* 为所有按键绑定测试回调函数 */
    drv_button_attach_callback("key1", BUTTON_ALL_RIGGER, test_button_callback);
    drv_button_attach_callback("key2", BUTTON_ALL_RIGGER, test_button_callback);
    drv_button_attach_callback("key3", BUTTON_ALL_RIGGER, test_button_callback);
    drv_button_attach_callback("key4", BUTTON_ALL_RIGGER, test_button_callback);
    drv_button_attach_callback("key5", BUTTON_ALL_RIGGER, test_button_callback);
    drv_button_attach_callback("key6", BUTTON_ALL_RIGGER, test_button_callback);
    
    rt_kprintf("[TEST] All buttons configured with test callbacks\n");
    rt_kprintf("[TEST] Press any button to test functionality\n");
    rt_kprintf("[TEST] Button GPIO pins:\n");
    rt_kprintf("  Key1: PC4\n");
    rt_kprintf("  Key2: PB14\n");
    rt_kprintf("  Key3: PA0\n");
    rt_kprintf("  Key4: PA8\n");
    rt_kprintf("  Key5: PB7\n");
    rt_kprintf("  Key6: PA15\n");
    rt_kprintf("=== Test Ready ===\n\n");
    
    return RT_EOK;
}

/**
 * @brief 按键驱动简单测试函数
 * @note 该函数提供一个简单的按键测试接口，可通过MSH命令调用
 */
static void button_test(void)
{
    drv_button_full_test();
}

/* 导出MSH命令 */
MSH_CMD_EXPORT(button_test, button driver test);

/**
 * @}
 */

#endif /* PKG_USING_BUTTON */
