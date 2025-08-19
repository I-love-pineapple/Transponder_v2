/**
 * @file applications.c
 * @brief 兼容性适配文件
 * @details 为保持与旧代码的兼容性，提供必要的接口函数
 * @author RT-Thread Team
 * @date 2024-12-27
 * @version 1.0.0
 */

#include <rtthread.h>
#include "key_manager.h"

#define DBG_TAG "app_compat"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/**
 * @brief 处理PRESS_ACK帧应答
 * @note 兼容性函数，桥接到新的按键管理器
 */
void app_handle_press_ack(void)
{
    LOG_D("收到PRESS_ACK应答");
    
    /* 获取当前上传信息 */
    key_upload_info_t info;
    if (key_manager_get_upload_info(&info) == RT_EOK)
    {
        /* 处理上传应答 */
        key_manager_handle_upload_ack(info.sequence, 0); /* 0表示成功 */
    }
    else
    {
        LOG_W("获取按键上传信息失败");
    }
}
