/*
 * E35-2G4T模块使用示例
 * 演示如何使用E35-2G4T AT驱动进行无线通信
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "e35_at.h"

#define DBG_TAG "e35.example"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/* 全局变量 */
static struct e35_device *e35_device = RT_NULL;
static rt_device_t uart_device = RT_NULL;

/* UART接收回调函数 */
static rt_err_t uart_rx_callback(rt_device_t dev, rt_size_t size)
{
    /* 在传输模式下，接收到的数据就是无线传输的数据 */
    rt_uint8_t buffer[256];
    rt_size_t read_size;
    
    read_size = rt_device_read(dev, 0, buffer, size);
    if (read_size > 0)
    {
        buffer[read_size] = '\0';
        rt_kprintf("Received wireless data: %s\n", buffer);
    }
    
    return RT_EOK;
}

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
    result = e35_device_set_address(e35_device, 0, 0);
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
    
    /* 可选：开启数据加密 */
    // result = e35_device_set_encrypt(e35_device, 1, 123, 456);  /* key0=123, key1=456 */
    // if (result != RT_EOK)
    // {
    //     LOG_W("Failed to set encryption, continuing without encryption");
    // }
    
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
static rt_err_t e35_send_data(const char *data)
{
    rt_size_t len;
    rt_size_t sent;
    
    if (uart_device == RT_NULL)
    {
        LOG_E("UART device not initialized");
        return -RT_ERROR;
    }
    
    len = rt_strlen(data);
    sent = rt_device_write(uart_device, 0, data, len);
    
    if (sent != len)
    {
        LOG_E("Failed to send all data, sent %d/%d bytes", sent, len);
        return -RT_ERROR;
    }
    
    LOG_D("Sent %d bytes: %s", sent, data);
    return RT_EOK;
}

/* 无线通信测试任务 */
static void e35_test_thread(void *parameter)
{
    rt_uint32_t counter = 0;
    char test_data[64];
    
    /* 等待系统初始化完成 */
    rt_thread_mdelay(2000);
    
    LOG_I("E35 wireless communication test started");
    
    while (1)
    {
        /* 每5秒发送一次测试数据 */
        rt_snprintf(test_data, sizeof(test_data), "Hello E35! Counter: %d", counter++);
        
        if (e35_send_data(test_data) == RT_EOK)
        {
            LOG_I("Test data sent: %s", test_data);
        }
        
        rt_thread_mdelay(5000);
    }
}

/* P2P通信示例 */
static void e35_p2p_example(void)
{
    rt_err_t result;
    struct e35_config config;
    
    LOG_I("=== E35 P2P Communication Example ===");
    
    /* 配置为定点传输模式 */
    result = e35_device_set_trans_mode(e35_device, E35_TRANS_FIXED);
    if (result != RT_EOK)
    {
        LOG_E("Failed to set fixed transmission mode");
        return;
    }
    
    /* 设置目标地址 */
    result = e35_device_set_address(e35_device, 0, 2);  /* 发送到地址 0,2 */
    if (result != RT_EOK)
    {
        LOG_E("Failed to set target address");
        return;
    }
    
    /* 获取当前配置确认 */
    result = e35_device_get_config(e35_device, &config);
    if (result == RT_EOK)
    {
        LOG_I("Current config - Addr: %d,%d, Channel: %d, Power: %d", 
              config.addr_h, config.addr_l, config.channel, config.power);
    }
    
    /* 发送P2P数据 */
    e35_send_data("P2P Message: Hello from Node 1!");
    
    LOG_I("P2P message sent to address 0,2");
}

/* 广播通信示例 */
static void e35_broadcast_example(void)
{
    rt_err_t result;
    
    LOG_I("=== E35 Broadcast Communication Example ===");
    
    /* 设置广播地址 */
    result = e35_device_set_address(e35_device, 0xFF, 0xFF);
    if (result != RT_EOK)
    {
        LOG_E("Failed to set broadcast address");
        return;
    }
    
    /* 发送广播数据 */
    e35_send_data("Broadcast: This is a broadcast message!");
    
    LOG_I("Broadcast message sent");
}

/* 初始化函数 */
int e35_example_init(void)
{
    rt_err_t result;
    rt_thread_t tid;

    at_client_init("lpuart1", 1024, 128);
    
    /* 打开UART设备用于数据收发 */
    uart_device = rt_device_find("lpuart1");
    if (uart_device == RT_NULL)
    {
        LOG_E("Cannot find lpuart1 device");
        return -RT_ERROR;
    }
    
    /* 初始化E35模块 */
    result = e35_module_init();
    if (result != RT_EOK)
    {
        LOG_E("Failed to initialize E35 module");
        return result;
    }
    
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
        LOG_I("E35 test thread started");
    }
    
    return RT_EOK;
}

/* 应用程序入口 */
// INIT_APP_EXPORT(e35_example_init);

/* MSH命令实现 */
#ifdef RT_USING_FINSH
#include <finsh.h>

static void e35_send(int argc, char **argv)
{
    if (argc < 2)
    {
        rt_kprintf("Usage: e35_send <message>\n");
        return;
    }
    
    if (e35_send_data(argv[1]) == RT_EOK)
    {
        rt_kprintf("Message sent: %s\n", argv[1]);
    }
    else
    {
        rt_kprintf("Failed to send message\n");
    }
}
MSH_CMD_EXPORT(e35_send, Send message via E35 module);

static void e35_p2p(int argc, char **argv)
{
    e35_p2p_example();
}
MSH_CMD_EXPORT(e35_p2p, Test E35 P2P communication);

static void e35_broadcast(int argc, char **argv)
{
    e35_broadcast_example();
}
MSH_CMD_EXPORT(e35_broadcast, Test E35 broadcast communication);

static void e35_info(int argc, char **argv)
{
    struct e35_config config;
    rt_err_t result;
    
    if (e35_device == RT_NULL)
    {
        rt_kprintf("E35 device not initialized\n");
        return;
    }
    
    result = e35_device_get_config(e35_device, &config);
    if (result != RT_EOK)
    {
        rt_kprintf("Failed to get E35 configuration\n");
        return;
    }
    
    rt_kprintf("=== E35 Module Configuration ===\n");
    rt_kprintf("Mode:       %s\n", config.mode ? "Config" : "Transmission");
    rt_kprintf("Address:    %d,%d\n", config.addr_h, config.addr_l);
    rt_kprintf("Baud Rate:  %d\n", config.baud);
    rt_kprintf("Air Rate:   %d\n", config.rate);
    rt_kprintf("Power:      %d\n", config.power);
    rt_kprintf("Channel:    %d\n", config.channel);
    rt_kprintf("Trans Mode: %s\n", config.trans ? "Fixed" : "Transparent");
    rt_kprintf("Encryption: %s\n", config.encrypt ? "Enabled" : "Disabled");
    rt_kprintf("Low Power:  %s\n", config.lpwr ? "Enabled" : "Disabled");
}
MSH_CMD_EXPORT(e35_info, Show E35 module information);

#endif /* RT_USING_FINSH */