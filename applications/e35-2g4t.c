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

#define DBG_TAG "e35.2g4t"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

typedef enum {
    JOIN_STATUS_NONE = 0,   //不入网
    JOIN_STATUS_JOINING,    //入网中
    JOIN_STATUS_JOINED,     //已入网
}join_status_t;

/* 全局变量 */
static struct e35_device *e35_device = RT_NULL;
static rt_device_t uart_device = RT_NULL;
static rt_sem_t lpuart_sem = RT_NULL;
at_client_t lpuart_cli = RT_NULL;
rt_uint32_t gateway_id = 0;
rt_uint32_t node_id = 0;
rt_uint8_t work_channel = 0;
rt_uint8_t timeslot = 0;
rt_uint8_t rssi = 0;
rt_uint8_t join_status = JOIN_STATUS_JOINING; //TODO
rt_uint8_t ch_switch = 0;

const rt_uint8_t beacon_ch_table[3] = {0, 27, 54};

/* 初始化E35模块 */
static rt_err_t e35_module_init(void)
{
    rt_err_t result = RT_EOK;
    
    /* 创建E35设备实例 */
    e35_device = e35_create("lpuart1");  /* 假设使用uart2连接E35模块 */
    if (e35_device == RT_NULL)
    {
        LOG_E("Failed to create E35 device");
        return -RT_ERROR;
    }
    
    /* 初始化E35设备 */
    result = e35_device_init(e35_device);
    if (result != RT_EOK)
    {
        LOG_E("Failed to initialize E35 device");
        return result;
    }
    
    /* 配置E35模块参数 */
    LOG_I("Configuring E35 module...");
    
    /* 设置模块地址为 1,1 */
    result = e35_device_set_address(e35_device, 255, 255);
    if (result != RT_EOK)
    {
        LOG_E("Failed to set address");
        return result;
    }
    
    /* 设置串口参数：9600,8N1 */
    // result = e35_device_set_uart(e35_device, E35_BAUD_9600, E35_STOPBIT_1, E35_PARITY_NONE);
    // if (result != RT_EOK)
    // {
    //     LOG_E("Failed to set UART parameters");
    //     return result;
    // }
    
    /* 设置射频参数：250Kbps空中速率，10dBm功率，信道0 */
    result = e35_device_set_rf_params(e35_device, E35_RATE_250K, 0, 0);
    if (result != RT_EOK)
    {
        LOG_E("Failed to set RF parameters");
        return result;
    }
    
    /* 设置为透明传输模式 */
    result = e35_device_set_trans_mode(e35_device, E35_TRANS_TRANSPARENT);
    if (result != RT_EOK)
    {
        LOG_E("Failed to set transmission mode");
        return result;
    }

    /* 打开DRSSI */
    result = e35_device_set_drssi(e35_device, 1);
    if (result != RT_EOK)
    {
        LOG_E("Failed to set drssi");
        return result;
    }
    
    /* 可选：开启数据加密 */
    result = e35_device_set_encrypt(e35_device, 0, 123, 456);  /* key0=123, key1=456 */
    if (result != RT_EOK)
    {
        LOG_W("Failed to set encryption, continuing without encryption");
    }
    
    /* 切换到传输模式开始工作 */
    result = e35_device_enter_trans_mode(e35_device);
    if (result != RT_EOK)
    {
        LOG_E("Failed to enter transmission mode");
        return result;
    }
    
    LOG_I("E35 module configured successfully");
    return RT_EOK;
}

/* 发送无线数据 */
static rt_err_t e35_send_data(const char *data, rt_size_t len)
{
    rt_size_t sent;
    
    if (uart_device == RT_NULL)
    {
        LOG_E("UART device not initialized");
        return -RT_ERROR;
    }
    
    sent = rt_device_write(uart_device, 0, data, len);
    
    if (sent != len)
    {
        LOG_E("Failed to send all data, sent %d/%d bytes", sent, len);
        return -RT_ERROR;
    }
    
    LOG_D("Sent %d bytes: %s", sent, data);
    return RT_EOK;
}

static void fhss_timeout(void *parameter)
{
    ch_switch = 1;
}

/* 无线通信测试任务 */
static void e35_test_thread(void *parameter)
{
    rt_err_t result;
    volatile rt_uint16_t len = 0;
    char rx_buf[256];
    protocol_frame_t frame;
    protocol_error_t err;
    rt_timer_t timer_fhss;  //跳频定时器

    timer_fhss = rt_timer_create("fhss", fhss_timeout,
                             RT_NULL, 4000,
                             RT_TIMER_FLAG_PERIODIC);
    
    if(timer_fhss == RT_NULL)
    {
        LOG_E("Failed to create fhss timer");
        return;
    }
    rt_timer_start(timer_fhss);
    
    /* 等待系统初始化完成 */
    rt_thread_mdelay(200);
    
    LOG_I("E35无线通信测试开始");
    
    while (1)
    {
        if((ch_switch) && (join_status == JOIN_STATUS_JOINING))
        {
            static rt_uint32_t ch_idx = 0;
            ch_idx++;
            ch_switch = 0;
            work_channel = beacon_ch_table[ch_idx % 3];

            rt_device_read(lpuart_cli->device, 0, rx_buf, 256);//清空

            lpuart_cli->at_mode = 0;//AT
            e35_device_set_rf_params(e35_device, E35_RATE_250K, 0, work_channel);
            e35_device_enter_trans_mode(e35_device);
            lpuart_cli->at_mode = 1;//透传
            LOG_D("入网中 切换信道:%d", work_channel);
        }

        result = rt_sem_take(lpuart_sem, 10);
        if(result != RT_EOK)
        {
            continue;
        }
        rt_thread_mdelay(3);
        len = rt_device_read(lpuart_cli->device, 0, rx_buf, 256);
        if(len > 0)
        {
            len--;
            rssi = rx_buf[len];
            LOG_D("len:%d rssi:%d bytes: %.*s", len, rx_buf[len], len, rx_buf);
            LOG_RAW("hex: ");
            for (int i = 0; i < (len+1); i++)
            {
                LOG_RAW("%02X ", rx_buf[i]);
            }
            LOG_RAW("\n");

            err = protocol_parse_frame(rx_buf, len, &frame);
            if (err != PROTOCOL_OK)
            {
                LOG_E("Failed to parse frame, err: %d", err);
                continue;
            }

            switch (frame.header.frame_type)
            {
                /* 广播帧 */
                case FRAME_JOIN_BEACON:
                {
                    join_beacon_payload_t payload;
                    LOG_D("Received JOIN_BEACON frame");

                    if(join_status != JOIN_STATUS_JOINING)
                    {
                        LOG_D("Not in JOINING status, skip");
                        continue;
                    }

                    err = protocol_extract_join_beacon_payload(&frame, &payload);
                    if (err != PROTOCOL_OK)
                    {
                        LOG_E("Failed to extract JOIN_BEACON payload, err: %d", err);
                        continue;
                    }

                    if(rssi < 40)
                    {
                        LOG_D("rssi too low, skip");
                        continue;
                    }

                    gateway_id = payload.gateway_id;
                    work_channel = payload.work_channel;
                    
                    /* 发送入网请求帧 */
                    len = 256;
                    err = protocol_create_join_req_frame(gateway_id, node_id, rssi, rx_buf, &len);
                    if (err != PROTOCOL_OK)
                    {
                        LOG_E("Failed to create JOIN_REQ frame, err: %d", err);
                        continue;
                    }

                    e35_send_data(rx_buf, len);
                    rt_timer_stop(timer_fhss);
                    rt_timer_start(timer_fhss);

                    break;
                }
                
                /* 入网应答帧 */
                case FRAME_JOIN_RESP:
                {
                    join_resp_payload_t payload;
                    LOG_D("Received JOIN_RESP frame");

                    if(join_status != JOIN_STATUS_JOINING)
                    {
                        LOG_D("Not in JOINING status, skip");
                        continue;
                    }

                    err = protocol_extract_join_resp_payload(&frame, &payload);
                    if (err != PROTOCOL_OK)
                    {
                        LOG_E("Failed to extract JOIN_RESP payload, err: %d", err);
                        continue;
                    }

                    if(gateway_id != payload.gateway_id)
                    {
                        LOG_D("gateway_id not match, expect: %x, got: %x", gateway_id, payload.gateway_id);
                        continue;
                    }

                    if(node_id != payload.node_id)
                    {
                        LOG_D("node_id not match, expect: %x, got: %x", node_id, payload.node_id);
                        continue;
                    }

                    LOG_D("gateway_id:%x, node_id:%x, work_channel:%d, timeslot:%d ", 
                        payload.gateway_id, 
                        payload.node_id, 
                        payload.work_channel, 
                        payload.timeslot);

                    work_channel = payload.work_channel;
                    timeslot = payload.timeslot;

                    /* 发送入网确认帧 */
                    len = 256;
                    err = protocol_create_join_ack_frame(gateway_id, node_id, 0, rx_buf, &len);
                    if (err != PROTOCOL_OK)
                    {
                        LOG_E("Failed to create JOIN_ACK frame, err: %d", err);
                        continue;
                    }

                    e35_send_data(rx_buf, len);
                    rt_timer_stop(timer_fhss);

                    rt_thread_mdelay(100);

                    lpuart_cli->at_mode = 0;//AT
                    e35_device_set_rf_params(e35_device, E35_RATE_250K, 0, work_channel);
                    e35_device_enter_trans_mode(e35_device);
                    lpuart_cli->at_mode = 1;//透传

                    join_status = JOIN_STATUS_JOINED;//已入网

                    // rt_timer_start(timer_fhss);
                    break;
                }

                /* PRESS确认帧 */
                case FRAME_PRESS_ACK:
                    LOG_D("Received PRESS_ACK frame");
                    if(join_status != JOIN_STATUS_JOINED)
                    {
                        LOG_D("Not in JOINED status, skip");
                        continue;
                    }
                    break;

                /* 重置命令帧 */
                case FRAME_RESET_CMD:
                    LOG_D("Received RESET_CMD frame");
                    break;
                
                /* 未知帧 */
                default:
                    LOG_E("Received UNKNOWN frame");
                    break;
            }
        }
    }
}

/* 应答数据上传 */
void press_upload(uint16_t sequence, uint8_t option, uint8_t battery)
{
    rt_err_t err;
    char rx_buf[256];
    rt_size_t len = 256;

    if(join_status != JOIN_STATUS_JOINED)
    {
        LOG_D("Not in JOINED status, skip");
        return;
    }

    err = protocol_create_press_req_frame(gateway_id, node_id, sequence, option, battery, rx_buf, &len);
    if (err != PROTOCOL_OK)
    {
        LOG_E("Failed to create PRESS_REQ frame, err: %d", err);
        return;
    }

    e35_send_data(rx_buf, len);
    LOG_D("发送应答帧");
}



/* 初始化函数 */
int rf_init(void)
{
    rt_err_t result;
    rt_thread_t tid;

    lpuart_sem = rt_sem_create("lps", 0, RT_IPC_FLAG_PRIO);
    if (lpuart_sem == RT_NULL)
    {
        LOG_E("创建信号量失败");
        return -RT_ERROR;
    }

    at_client_init("lpuart1", 1024, 128);

    lpuart_cli = at_client_get("lpuart1");

    lpuart_cli->recvd_data = lpuart_sem;

    lpuart_cli->at_mode = 0;
    
    /* 打开UART设备用于数据收发 */
    uart_device = rt_device_find("lpuart1");
    if (uart_device == RT_NULL)
    {
        LOG_E("没有找到lpuart1设备");
        return -RT_ERROR;
    }
    
    /* 初始化E35模块 */
    result = e35_module_init();
    if (result != RT_EOK)
    {
        LOG_E("初始化E35模块失败");
        return result;
    }

    lpuart_cli->at_mode = 1;
    
    /* 创建测试线程 */
    tid = rt_thread_create("e35_test",
                          e35_test_thread,
                          RT_NULL,
                          2048,
                          RT_THREAD_PRIORITY_MAX / 2,
                          20);
    
    if (tid != RT_NULL)
    {
        rt_thread_startup(tid);
        LOG_I("E35测试线程启动");
    }
    
    return RT_EOK;
}
