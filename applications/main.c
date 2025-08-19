/**
 * @file main_new.c
 * @brief 重构后的系统主函数实现文件
 * @details 使用新的模块化架构的系统启动入口
 * @author RT-Thread Team
 * @date 2024-12-27
 * @version 2.0.0
 *
 * @copyright Copyright (c) 2024-2025, RT-Thread Development Team
 * @license Apache-2.0
 *
 * @par 修改日志:
 * <table>
 * <tr><th>日期       <th>版本  <th>作者        <th>说明
 * <tr><td>2024-12-27 <td>2.0.0 <td>RT-Thread   <td>重构版本，模块化架构
 * </table>
 */

#include <rtthread.h>
#include "app_manager.h"

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/**
 * @defgroup Main_External_Functions 外部函数声明
 * @brief 底层驱动初始化所需的外部函数接口
 * @{
 */

/** @brief 按键驱动初始化函数 */
extern rt_err_t my_button_init(void);

/** @brief RF通信模块初始化函数 */
extern int rf_init(void);

/** @brief 随机数生成器初始化函数 */
extern void MX_RNG_Init(void);

/**
 * @}
 */

/**
 * @brief 底层硬件初始化
 * @return rt_err_t 初始化结果
 * @retval RT_EOK 初始化成功
 * @retval -RT_ERROR 初始化失败
 * @note 初始化底层硬件驱动，为应用层提供服务
 */
static rt_err_t hardware_init(void)
{
    rt_err_t result;

    LOG_I("初始化底层硬件...");

    /* 初始化随机数生成器 */
    MX_RNG_Init();
    LOG_D("随机数生成器初始化完成");

    /* 初始化按键驱动 */
    result = my_button_init();
    if (result != RT_EOK)
    {
        LOG_E("按键驱动初始化失败: %d", result);
        return result;
    }
    LOG_D("按键驱动初始化完成");

    /* 初始化RF通信模块 */
    result = rf_init();
    if (result != 0)
    {
        LOG_E("RF通信模块初始化失败: %d", result);
        return -RT_ERROR;
    }
    LOG_D("RF通信模块初始化完成");

    LOG_I("底层硬件初始化完成");
    return RT_EOK;
}

/**
 * @brief 重构后的系统主函数
 * @return int 程序退出码
 * @retval RT_EOK 程序正常退出
 * @retval -RT_ERROR 初始化失败
 * @note 使用新的模块化架构，代码更简洁、逻辑更清晰
 * @par 新架构特点:
 *      - 模块高度解耦，职责清晰
 *      - 统一的配置管理
 *      - 完善的状态机管理
 *      - 事件驱动的设计模式
 *      - 预留配置掉电存储接口
 */
int main(void)
{
    rt_err_t result;

    LOG_I("=== 系统启动 (重构版本 v2.0.0) ===");

    /* 1. 初始化底层硬件 */
    result = hardware_init();
    if (result != RT_EOK)
    {
        LOG_E("底层硬件初始化失败");
        return -RT_ERROR;
    }

    /* 2. 初始化应用管理器 (自动初始化所有子模块) */
    result = app_manager_init();
    if (result != RT_EOK)
    {
        LOG_E("应用管理器初始化失败");
        return -RT_ERROR;
    }

    /* 3. 启动应用管理器 */
    result = app_manager_start();
    if (result != RT_EOK)
    {
        LOG_E("应用管理器启动失败");
        app_manager_deinit();
        return -RT_ERROR;
    }

    LOG_I("=== 系统启动完成，进入主循环 ===");
    LOG_I("新架构特性:");
    LOG_I("  - 模块化设计，高度解耦");
    LOG_I("  - 统一配置管理，支持掉电存储");
    LOG_I("  - 完善的状态机管理");
    LOG_I("  - 事件驱动架构");
    LOG_I("  - 简化的维护和扩展");

    /* 4. 主循环 - 现在只需要简单的监控 */
    while (1)
    {
        rt_thread_mdelay(5000);
        
        /* 可以在这里添加系统监控逻辑 */
        /* 例如：检查内存使用、看门狗、健康检查等 */
        
        /* 打印系统运行状态 (调试用) */
        // if (DBG_LVL >= DBG_INFO)
        // {
        //     app_manager_info_t info;
        //     if (app_manager_get_info(&info) == RT_EOK)
        //     {
        //         LOG_D("系统运行状态: %s, 运行时间: %d秒, 按键事件: %d, 入网次数: %d",
        //               system_state_to_string(info.current_state),
        //               info.uptime, info.total_key_events, info.total_network_joins);
        //     }
        // }
    }

    /* 正常情况下不会执行到这里 */
    LOG_I("系统主循环退出，清理资源...");
    app_manager_deinit();
    
    return RT_EOK;
}

/* 
 * 使用说明:
 * 
 * 1. 要使用新架构，请将此文件重命名为 main.c，将原 main.c 备份
 * 2. 新架构的优势：
 *    - main函数从97行减少到80行左右，逻辑更清晰
 *    - 所有业务逻辑由应用管理器统一管理
 *    - 模块间通过标准接口通信，耦合度极低
 *    - 配置集中管理，支持运行时修改和掉电保存
 *    - 状态机驱动，事件响应更及时
 * 
 * 3. 迁移建议：
 *    - 逐步替换：可以先保留原架构，并行测试新架构
 *    - 配置迁移：将现有硬编码配置移到配置管理器
 *    - 功能测试：重点测试按键处理和入网逻辑
 *    - 性能对比：比较新旧架构的内存和CPU使用
 */
