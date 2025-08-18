/*
 * E35-2G4T模块使用示例
 * 演示如何使用E35-2G4T AT驱动进行无线通信
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "e35_at.h"
#include "at.h"
#include "protocol.h"
#include "e35_module.h"

#define DBG_TAG "e35.2g4t"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>


/**
 * @defgroup E35_Private_Variables E35模块私有变量
 * @{
 */

/** @brief E35设备实例 */
static struct e35_device *e35_device = RT_NULL;

/** @brief UART设备句柄 */
static rt_device_t uart_device = RT_NULL;

/** @brief LPUART信号量 */
static rt_sem_t lpuart_sem = RT_NULL;

/** @brief AT客户端句柄 */
at_client_t lpuart_cli = RT_NULL;

/** @brief E35模块状态管理结构体 */
static e35_module_state_t g_e35_state = {0};

/** @brief 外部函数声明 */
extern void app_handle_press_ack(void);

/**
 * @}
 */

/**
 * @defgroup E35_Private_Constants E35模块私有常量
 * @{
 */

/** @brief 信标信道表 */
const rt_uint8_t beacon_ch_table[3] = {0, 27, 54};

/** @brief 默认节点ID */
#define DEFAULT_NODE_ID         0x12345678

/** @brief 默认地址配置 */
#define DEFAULT_ADDR_HIGH       255
#define DEFAULT_ADDR_LOW        255

/** @brief 默认射频参数 */
#define DEFAULT_RF_RATE         E35_RATE_250K
#define DEFAULT_RF_POWER        0
#define DEFAULT_RF_CHANNEL      0

/** @brief 跳频定时器周期（毫秒） */
#define FHSS_TIMER_PERIOD       4000

/**
 * @}
 */

/**
 * @defgroup E35_Private_Functions E35模块私有函数
 * @{
 */

/**
 * @brief 初始化E35模块配置
 * @return rt_err_t 初始化结果
 * @retval RT_EOK 初始化成功
 * @retval -RT_ERROR 初始化失败
 * @note 该函数配置E35模块的基本参数
 */
static rt_err_t e35_module_config_init(void)
{
    rt_err_t result = RT_EOK;

    LOG_I("配置E35模块参数...");

    /* 设置模块地址 */
    result = e35_device_set_address(e35_device, DEFAULT_ADDR_HIGH, DEFAULT_ADDR_LOW);
    if (result != RT_EOK)
    {
        LOG_E("设置模块地址失败");
        return result;
    }
    g_e35_state.config.address_high = DEFAULT_ADDR_HIGH;
    g_e35_state.config.address_low = DEFAULT_ADDR_LOW;

    /* 设置射频参数 */
    result = e35_device_set_rf_params(e35_device, DEFAULT_RF_RATE, DEFAULT_RF_POWER, DEFAULT_RF_CHANNEL);
    if (result != RT_EOK)
    {
        LOG_E("设置射频参数失败");
        return result;
    }
    g_e35_state.config.rate = DEFAULT_RF_RATE;
    g_e35_state.config.power = DEFAULT_RF_POWER;
    g_e35_state.config.channel = DEFAULT_RF_CHANNEL;

    /* 设置为透明传输模式 */
    result = e35_device_set_trans_mode(e35_device, E35_TRANS_TRANSPARENT);
    if (result != RT_EOK)
    {
        LOG_E("设置透明传输模式失败");
        return result;
    }

    /* 打开DRSSI功能 */
    result = e35_device_set_drssi(e35_device, 1);
    if (result != RT_EOK)
    {
        LOG_E("设置DRSSI功能失败");
        return result;
    }

    /* 可选：开启数据加密 */
    result = e35_device_set_encrypt(e35_device, 0, 123, 456);
    if (result != RT_EOK)
    {
        LOG_W("设置加密失败，继续使用非加密模式");
        g_e35_state.config.encrypt_enabled = RT_FALSE;
    }
    else
    {
        g_e35_state.config.encrypt_enabled = RT_FALSE;
        g_e35_state.config.encrypt_key0 = 123;
        g_e35_state.config.encrypt_key1 = 456;
    }

    /* 切换到传输模式 */
    result = e35_device_enter_trans_mode(e35_device);
    if (result != RT_EOK)
    {
        LOG_E("进入传输模式失败");
        return result;
    }
    g_e35_state.in_trans_mode = RT_TRUE;

    LOG_I("E35模块配置完成");
    return RT_EOK;
}

/**
 * @brief 初始化E35模块
 * @return rt_err_t 初始化结果
 * @retval RT_EOK 初始化成功
 * @retval -RT_ERROR 初始化失败
 * @note 该函数创建并初始化E35设备实例
 */
static rt_err_t e35_module_init(void)
{
    rt_err_t result = RT_EOK;

    if (g_e35_state.initialized)
    {
        LOG_W("E35模块已经初始化");
        return RT_EOK;
    }

    LOG_I("初始化E35模块...");

    /* 初始化状态结构体 */
    rt_memset(&g_e35_state, 0, sizeof(g_e35_state));
    g_e35_state.node_id = DEFAULT_NODE_ID;
    g_e35_state.join_status = JOIN_STATUS_NONE;

    /* 创建E35设备实例 */
    e35_device = e35_create("lpuart1");
    if (e35_device == RT_NULL)
    {
        LOG_E("创建E35设备实例失败");
        return -RT_ERROR;
    }

    /* 初始化E35设备 */
    result = e35_device_init(e35_device);
    if (result != RT_EOK)
    {
        LOG_E("初始化E35设备失败");
        return result;
    }

    /* 配置E35模块参数 */
    result = e35_module_config_init();
    if (result != RT_EOK)
    {
        LOG_E("配置E35模块失败");
        return result;
    }

    g_e35_state.initialized = RT_TRUE;
    LOG_I("E35模块初始化成功");

    return RT_EOK;
}

/**
 * @brief 发送无线数据
 * @param data 数据指针
 * @param len 数据长度
 * @return rt_err_t 发送结果
 * @retval RT_EOK 发送成功
 * @retval -RT_ERROR 发送失败
 * @retval -RT_EINVAL 参数无效
 * @note 该函数通过E35模块发送无线数据
 */
static rt_err_t e35_send_data(const char *data, rt_size_t len)
{
    rt_size_t sent;

    /* 参数验证 */
    if (data == RT_NULL || len == 0)
    {
        LOG_E("发送数据参数无效");
        return -RT_EINVAL;
    }

    if (uart_device == RT_NULL)
    {
        LOG_E("UART设备未初始化");
        return -RT_ERROR;
    }

    if (!g_e35_state.initialized || !g_e35_state.in_trans_mode)
    {
        LOG_E("E35模块未就绪");
        return -RT_ERROR;
    }

    /* 发送数据 */
    sent = rt_device_write(uart_device, 0, data, len);

    if (sent != len)
    {
        LOG_E("数据发送不完整，已发送 %d/%d 字节", sent, len);
        return -RT_ERROR;
    }

    LOG_D("发送 %d 字节数据成功", sent);
    return RT_EOK;
}

/**
 * @brief 跳频定时器超时回调函数
 * @param parameter 定时器参数
 * @note 该函数在跳频定时器超时时被调用，设置信道切换标志
 */
static void fhss_timeout(void *parameter)
{
    if (g_e35_state.join_status == JOIN_STATUS_JOINING)
    {
        g_e35_state.ch_switch = 1;
        LOG_D("跳频定时器触发，设置信道切换标志");
    }
}

/**
 * @brief 切换工作信道
 * @param channel 目标信道
 * @return rt_err_t 切换结果
 * @retval RT_EOK 切换成功
 * @retval -RT_ERROR 切换失败
 * @note 该函数切换E35模块的工作信道
 */
static rt_err_t e35_switch_channel(rt_uint8_t channel)
{
    rt_err_t result;

    if (!g_e35_state.initialized)
    {
        LOG_E("E35模块未初始化");
        return -RT_ERROR;
    }

    /* 退出透传模式 */
    lpuart_cli->at_mode = 0;
    g_e35_state.in_trans_mode = RT_FALSE;

    /* 设置新信道 */
    result = e35_device_set_rf_params(e35_device, g_e35_state.config.rate,
                                      g_e35_state.config.power, channel);
    if (result != RT_EOK)
    {
        LOG_E("设置信道 %d 失败", channel);
        return result;
    }

    /* 重新进入透传模式 */
    result = e35_device_enter_trans_mode(e35_device);
    if (result != RT_EOK)
    {
        LOG_E("重新进入透传模式失败");
        return result;
    }

    lpuart_cli->at_mode = 1;
    g_e35_state.in_trans_mode = RT_TRUE;
    g_e35_state.work_channel = channel;

    LOG_D("切换到信道 %d 成功", channel);
    return RT_EOK;
}

/**
 * @brief 处理信道切换逻辑
 * @note 该函数处理入网过程中的信道切换
 */
static void e35_handle_channel_switch(void)
{
    static rt_uint32_t ch_idx = 0;
    rt_uint8_t new_channel;
    char rx_buf[256];

    if (!g_e35_state.ch_switch || g_e35_state.join_status != JOIN_STATUS_JOINING)
    {
        return;
    }

    /* 切换到下一个信道 */
    ch_idx++;
    g_e35_state.ch_switch = 0;
    new_channel = beacon_ch_table[ch_idx % 3];

    /* 清空接收缓冲区 */
    rt_device_read(lpuart_cli->device, 0, rx_buf, 256);

    /* 切换信道 */
    if (e35_switch_channel(new_channel) == RT_EOK)
    {
        LOG_D("入网中，切换到信道: %d", new_channel);
    }
    else
    {
        LOG_E("信道切换失败");
    }
}

/**
 * @brief E35无线通信处理线程
 * @param parameter 线程参数
 * @note 该线程负责处理E35模块的无线通信和协议解析
 */
static void e35_comm_thread(void *parameter)
{
    rt_err_t result;
    volatile rt_uint16_t len = 0;
    char rx_buf[256];
    protocol_frame_t frame;
    protocol_error_t err;
    rt_timer_t timer_fhss;

    LOG_I("E35无线通信线程启动");

    /* 创建跳频定时器 */
    timer_fhss = rt_timer_create("fhss", fhss_timeout,
                                 RT_NULL, FHSS_TIMER_PERIOD,
                                 RT_TIMER_FLAG_PERIODIC);

    if (timer_fhss == RT_NULL)
    {
        LOG_E("创建跳频定时器失败");
        return;
    }

    /* 启动跳频定时器 */
    rt_timer_start(timer_fhss);

    /* 等待系统初始化完成 */
    rt_thread_mdelay(200);

    LOG_I("E35无线通信处理开始");

    while (1)
    {
        /* 处理信道切换 */
        e35_handle_channel_switch();

        /* 等待接收数据 */
        result = rt_sem_take(lpuart_sem, 10);
        if (result != RT_EOK)
        {
            continue;
        }

        /* 短暂延时确保数据接收完整 */
        rt_thread_mdelay(3);

        /* 读取接收数据 */
        len = rt_device_read(lpuart_cli->device, 0, rx_buf, 256);
        if (len == 0)
        {
            continue;
        }

        /* 提取RSSI（最后一个字节） */
        len--;
        g_e35_state.rssi = rx_buf[len];

        LOG_D("接收数据长度:%d, RSSI:%d", len, g_e35_state.rssi);
        LOG_RAW("数据内容: ");
        for (int i = 0; i < (len + 1); i++)
        {
            LOG_RAW("%02X ", rx_buf[i]);
        }
        LOG_RAW("\n");

        /* 解析协议帧 */
        err = protocol_parse_frame(rx_buf, len, &frame);
        if (err != PROTOCOL_OK)
        {
            LOG_E("协议帧解析失败，错误码: %d", err);
            continue;
        }

        // LOG_D("收到帧类型: 0x%02X", frame.header.frame_type);

        /* 根据帧类型进行处理 */
        switch (frame.header.frame_type)
        {
            /* 入网广播帧处理 */
            case FRAME_JOIN_BEACON:
            {
                join_beacon_payload_t payload;
                LOG_D("收到JOIN_BEACON帧");

                /* 检查当前状态 */
                if (g_e35_state.join_status != JOIN_STATUS_JOINING)
                {
                    LOG_D("当前非入网状态，忽略广播帧");
                    continue;
                }

                /* 提取载荷数据 */
                err = protocol_extract_join_beacon_payload(&frame, &payload);
                if (err != PROTOCOL_OK)
                {
                    LOG_E("提取JOIN_BEACON载荷失败，错误码: %d", err);
                    continue;
                }

                /* 检查信号强度 */
                if (g_e35_state.rssi < 40)
                {
                    LOG_D("信号强度过低(%d)，忽略广播帧", g_e35_state.rssi);
                    continue;
                }

                /* 保存网关信息 */
                g_e35_state.gateway_id = payload.gateway_id;
                g_e35_state.work_channel = payload.work_channel;

                LOG_I("收到有效广播帧，网关ID:0x%08X，工作信道:%d",
                      g_e35_state.gateway_id, g_e35_state.work_channel);

                /* 创建并发送入网请求帧 */
                rt_uint16_t req_len = 256;
                err = protocol_create_join_req_frame(g_e35_state.gateway_id,
                                                     g_e35_state.node_id,
                                                     g_e35_state.rssi,
                                                     rx_buf, &req_len);
                if (err != PROTOCOL_OK)
                {
                    LOG_E("创建JOIN_REQ帧失败，错误码: %d", err);
                    continue;
                }

                /* 发送入网请求 */
                if (e35_send_data(rx_buf, req_len) == RT_EOK)
                {
                    LOG_I("发送入网请求成功");
                    /* 重启跳频定时器 */
                    rt_timer_stop(timer_fhss);
                    rt_timer_start(timer_fhss);
                }
                else
                {
                    LOG_E("发送入网请求失败");
                }

                break;
            }
                
            /* 入网应答帧处理 */
            case FRAME_JOIN_RESP:
            {
                join_resp_payload_t payload;
                LOG_D("收到JOIN_RESP帧");

                /* 检查当前状态 */
                if (g_e35_state.join_status != JOIN_STATUS_JOINING)
                {
                    LOG_D("当前非入网状态，忽略应答帧");
                    continue;
                }

                /* 提取载荷数据 */
                err = protocol_extract_join_resp_payload(&frame, &payload);
                if (err != PROTOCOL_OK)
                {
                    LOG_E("提取JOIN_RESP载荷失败，错误码: %d", err);
                    continue;
                }

                /* 验证网关ID */
                if (g_e35_state.gateway_id != payload.gateway_id)
                {
                    LOG_D("网关ID不匹配，期望:0x%08X，收到:0x%08X",
                          g_e35_state.gateway_id, payload.gateway_id);
                    continue;
                }

                /* 验证节点ID */
                if (g_e35_state.node_id != payload.node_id)
                {
                    LOG_D("节点ID不匹配，期望:0x%08X，收到:0x%08X",
                          g_e35_state.node_id, payload.node_id);
                    continue;
                }

                LOG_I("入网应答验证通过 - 网关ID:0x%08X，节点ID:0x%08X，工作信道:%d，时隙:%d",
                      payload.gateway_id, payload.node_id,
                      payload.work_channel, payload.timeslot);

                /* 保存入网参数 */
                g_e35_state.work_channel = payload.work_channel;
                g_e35_state.timeslot = payload.timeslot;

                /* 创建并发送入网确认帧 */
                rt_uint16_t ack_len = 256;
                err = protocol_create_join_ack_frame(g_e35_state.gateway_id,
                                                     g_e35_state.node_id,
                                                     0, rx_buf, &ack_len);
                if (err != PROTOCOL_OK)
                {
                    LOG_E("创建JOIN_ACK帧失败，错误码: %d", err);
                    continue;
                }

                /* 发送入网确认 */
                if (e35_send_data(rx_buf, ack_len) != RT_EOK)
                {
                    LOG_E("发送入网确认失败");
                    continue;
                }

                /* 停止跳频定时器 */
                rt_timer_stop(timer_fhss);

                /* 短暂延时 */
                rt_thread_mdelay(100);

                /* 切换到工作信道 */
                if (e35_switch_channel(g_e35_state.work_channel) != RT_EOK)
                {
                    LOG_E("切换到工作信道失败");
                    continue;
                }

                /* 更新入网状态 */
                g_e35_state.join_status = JOIN_STATUS_JOINED;

                LOG_I("入网成功！工作信道:%d，时隙:%d",
                      g_e35_state.work_channel, g_e35_state.timeslot);

                break;
            }

            /* 按键上传确认帧处理 */
            case FRAME_PRESS_ACK:
            {
                LOG_D("收到PRESS_ACK帧");

                /* 检查当前状态 */
                if (g_e35_state.join_status != JOIN_STATUS_JOINED)
                {
                    LOG_D("当前非已入网状态，忽略PRESS_ACK帧");
                    continue;
                }

                /* 通知应用程序处理按键上传应答 */
                app_handle_press_ack();

                LOG_I("按键上传应答处理完成");
                break;
            }

            /* 重置命令帧处理 */
            case FRAME_RESET_CMD:
            {
                LOG_I("收到RESET_CMD帧，执行系统重置");
                /* TODO: 实现系统重置逻辑 */
                break;
            }

            /* 未知帧类型 */
            default:
            {
                LOG_W("收到未知帧类型: 0x%02X", frame.header.frame_type);
                break;
            }
        }
    }

    /* 清理资源 */
    if (timer_fhss != RT_NULL)
    {
        rt_timer_stop(timer_fhss);
        rt_timer_delete(timer_fhss);
    }

    LOG_I("E35无线通信线程退出");
}

/**
 * @}
 */

/**
 * @defgroup E35_Public_Functions E35模块公共函数
 * @{
 */

/**
 * @brief 上传按键数据
 * @param sequence 序列号
 * @param option 按键选项
 * @param battery 电池电量百分比
 * @return rt_err_t 上传结果
 * @retval RT_EOK 上传成功
 * @retval -RT_ERROR 上传失败
 * @retval -RT_EINVAL 参数无效
 * @note 该函数创建并发送按键上传请求帧
 */
rt_err_t press_upload(uint16_t sequence, uint8_t option, uint8_t battery)
{
    protocol_error_t err;
    char tx_buf[256];
    rt_uint16_t frame_len = 256;

    /* 参数验证 */
    if (option == 0 || battery > 100)
    {
        LOG_E("按键上传参数无效，选项:%d，电量:%d%%", option, battery);
        return -RT_EINVAL;
    }

    /* 检查模块状态 */
    if (!g_e35_state.initialized)
    {
        LOG_E("E35模块未初始化");
        return -RT_ERROR;
    }

    /* 检查入网状态 */
    if (g_e35_state.join_status != JOIN_STATUS_JOINED)
    {
        LOG_W("当前非已入网状态，无法上传按键数据");
        return -RT_ERROR;
    }

    LOG_D("创建按键上传帧，序列号:%d，选项:%d，电量:%d%%", sequence, option, battery);

    /* 创建按键上传请求帧 */
    err = protocol_create_press_req_frame(g_e35_state.gateway_id,
                                          g_e35_state.node_id,
                                          sequence, option, battery,
                                          tx_buf, &frame_len);
    if (err != PROTOCOL_OK)
    {
        LOG_E("创建PRESS_REQ帧失败，错误码: %d", err);
        return -RT_ERROR;
    }

    /* 发送数据 */
    if (e35_send_data(tx_buf, frame_len) != RT_EOK)
    {
        LOG_E("发送按键上传帧失败");
        return -RT_ERROR;
    }

    LOG_D("按键上传帧发送成功，序列号:%d", sequence);
    return RT_EOK;
}

/**
 * @brief 获取E35模块状态
 * @param state 状态结构体指针
 * @return rt_err_t 获取结果
 * @retval RT_EOK 获取成功
 * @retval -RT_EINVAL 参数无效
 * @note 该函数获取E35模块的当前状态信息
 */
rt_err_t e35_get_module_state(e35_module_state_t *state)
{
    if (state == RT_NULL)
    {
        return -RT_EINVAL;
    }

    *state = g_e35_state;
    return RT_EOK;
}

/**
 * @brief 设置入网状态
 * @param status 入网状态
 * @note 该函数设置E35模块的入网状态，供外部调用
 */
void e35_set_join_status(rt_uint8_t status)
{
    if (status < JOIN_STATUS_NONE || status > JOIN_STATUS_JOINED)
    {
        LOG_E("无效的入网状态: %d", status);
        return;
    }

    g_e35_state.join_status = (join_status_t)status;
    LOG_D("设置入网状态: %d", status);
}

/**
 * @brief 获取入网状态
 * @return rt_uint8_t 当前入网状态
 * @note 该函数获取E35模块的当前入网状态
 */
rt_uint8_t e35_get_join_status(void)
{
    return (rt_uint8_t)g_e35_state.join_status;
}



/**
 * @brief RF模块初始化函数
 * @return int 初始化结果
 * @retval 0 初始化成功
 * @retval -1 初始化失败
 * @note 该函数初始化RF通信模块的所有组件
 */
int rf_init(void)
{
    rt_err_t result;
    rt_thread_t comm_thread;

    LOG_I("初始化RF通信模块...");

    /* 创建LPUART信号量 */
    lpuart_sem = rt_sem_create("lpuart_sem", 0, RT_IPC_FLAG_PRIO);
    if (lpuart_sem == RT_NULL)
    {
        LOG_E("创建LPUART信号量失败");
        return -1;
    }

    /* 初始化AT客户端 */
    result = at_client_init("lpuart1", 1024, 128);
    if (result != RT_EOK)
    {
        LOG_E("初始化AT客户端失败");
        rt_sem_delete(lpuart_sem);
        return -1;
    }

    /* 获取AT客户端句柄 */
    lpuart_cli = at_client_get("lpuart1");
    if (lpuart_cli == RT_NULL)
    {
        LOG_E("获取AT客户端句柄失败");
        rt_sem_delete(lpuart_sem);
        return -1;
    }

    /* 配置AT客户端 */
    lpuart_cli->recvd_data = lpuart_sem;
    lpuart_cli->at_mode = 0; /* 初始为AT模式 */

    /* 查找UART设备 */
    uart_device = rt_device_find("lpuart1");
    if (uart_device == RT_NULL)
    {
        LOG_E("未找到lpuart1设备");
        rt_sem_delete(lpuart_sem);
        return -1;
    }

    /* 初始化E35模块 */
    result = e35_module_init();
    if (result != RT_EOK)
    {
        LOG_E("初始化E35模块失败，错误码: %d", result);
        rt_sem_delete(lpuart_sem);
        return -1;
    }

    /* 切换到透传模式 */
    lpuart_cli->at_mode = 1;

    /* 创建无线通信处理线程 */
    comm_thread = rt_thread_create("e35_comm",
                                   e35_comm_thread,
                                   RT_NULL,
                                   2048,
                                   RT_THREAD_PRIORITY_MAX / 2,
                                   20);

    if (comm_thread == RT_NULL)
    {
        LOG_E("创建无线通信线程失败");
        rt_sem_delete(lpuart_sem);
        return -1;
    }

    /* 启动通信线程 */
    rt_thread_startup(comm_thread);

    LOG_I("RF通信模块初始化成功");
    LOG_I("E35通信线程已启动");

    return 0;
}

/**
 * @brief RF模块反初始化函数
 * @return int 反初始化结果
 * @retval 0 反初始化成功
 * @note 该函数清理RF通信模块的所有资源
 */
int rf_deinit(void)
{
    LOG_I("反初始化RF通信模块...");

    /* 重置模块状态 */
    rt_memset(&g_e35_state, 0, sizeof(g_e35_state));

    /* 清理信号量 */
    if (lpuart_sem != RT_NULL)
    {
        rt_sem_delete(lpuart_sem);
        lpuart_sem = RT_NULL;
    }

    /* 清理设备句柄 */
    uart_device = RT_NULL;
    lpuart_cli = RT_NULL;
    e35_device = RT_NULL;

    LOG_I("RF通信模块反初始化完成");
    return 0;
}

/**
 * @}
 */
