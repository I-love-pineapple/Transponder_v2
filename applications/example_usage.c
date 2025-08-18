/**
 * @file example_usage.c
 * @brief LED应用程序使用示例
 * @details 展示如何使用LED应用程序的各种功能
 * @author RT-Thread Team
 * @date 2024-08-18
 * @version 2.0.0
 * 
 * @copyright Copyright (c) 2024 RT-Thread Development Team
 * 
 * @par 修改日志:
 * <table>
 * <tr><th>日期       <th>版本  <th>作者     <th>说明
 * <tr><td>2024-08-18 <td>2.0.0 <td>RT-Thread Team <td>创建使用示例
 * </table>
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "led_app.h"
#include "applications.h"
#include "e35_module.h"

#define DBG_TAG "example"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/**
 * @brief 系统状态查询示例
 * @note 该函数演示如何查询系统状态
 */
static void system_status_example(void)
{
    LOG_I("=== 系统状态查询示例 ===");
    
    /* 获取当前系统状态 */
    sys_state_t current_state = app_get_system_state();
    
    switch (current_state)
    {
        case SYS_STATE_NOT_JOINED:
            LOG_I("当前系统状态: 未入网");
            break;
        case SYS_STATE_JOINING:
            LOG_I("当前系统状态: 入网中");
            break;
        case SYS_STATE_JOINED:
            LOG_I("当前系统状态: 已入网");
            break;
        case SYS_STATE_KEY_UPLOADING:
            LOG_I("当前系统状态: 上传按键中");
            break;
        case SYS_STATE_KEY_UPLOAD_DONE:
            LOG_I("当前系统状态: 按键上传完成");
            break;
        case SYS_STATE_SLEEP:
            LOG_I("当前系统状态: 休眠");
            break;
        default:
            LOG_W("当前系统状态: 未知(%d)", current_state);
            break;
    }
    
    /* 获取E35模块状态 */
    e35_module_state_t e35_state;
    rt_err_t result = e35_get_module_state(&e35_state);
    if (result == RT_EOK)
    {
        LOG_I("E35模块状态:");
        LOG_I("  初始化状态: %s", e35_state.initialized ? "已初始化" : "未初始化");
        LOG_I("  透传模式: %s", e35_state.in_trans_mode ? "是" : "否");
        LOG_I("  网关ID: 0x%08X", e35_state.gateway_id);
        LOG_I("  节点ID: 0x%08X", e35_state.node_id);
        LOG_I("  工作信道: %d", e35_state.work_channel);
        LOG_I("  时隙: %d", e35_state.timeslot);
        LOG_I("  RSSI: %d", e35_state.rssi);
        LOG_I("  入网状态: %d", e35_state.join_status);
    }
    else
    {
        LOG_E("获取E35模块状态失败");
    }
    
    LOG_I("系统状态查询示例完成");
}

/**
 * @brief MSH命令：系统状态查询示例
 * @note 该命令可在MSH中执行系统状态查询示例
 */
static void status_example(void)
{
    system_status_example();
}
MSH_CMD_EXPORT(status_example, 系统状态查询示例);

