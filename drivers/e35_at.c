/*
 * E35-2G4T系列无线模块AT驱动
 * 基于RT-Thread AT组件实现
 */

#include <rtthread.h>
#include <at.h>
#include <rtdevice.h>
#include <string.h>
#include <stdio.h>
#include "e35_at.h"

#define DBG_TAG "e35.at"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

///* E35-2G4T模块配置参数结构体 */
//struct e35_config
//{
//    rt_uint8_t mode;        /* 工作模式 0:传输模式 1:配置模式 */
//    rt_uint8_t addr_h;      /* 地址高字节 */
//    rt_uint8_t addr_l;      /* 地址低字节 */
//    rt_uint8_t baud;        /* 串口波特率 0:9600 1:19200 2:38400 3:57600 4:115200 */
//    rt_uint8_t stopbit;     /* 停止位 0:1位 1:1.5位 2:2位 */
//    rt_uint8_t parity;      /* 校验位 0:无 1:偶 2:奇 */
//    rt_uint8_t rate;        /* 空中速率 0:250K 1:500K 2:1M 3:2M */
//    rt_uint8_t power;       /* 发射功率 0-26 */
//    rt_uint8_t channel;     /* 信道 0-80 */
//    rt_uint8_t trans;       /* 传输方式 0:透明 1:定点 */
//    rt_uint8_t packet;      /* 分包长度 23-48 */
//    rt_uint8_t drssi;       /* RSSI开关 0:关闭 1:开启 */
//    rt_uint8_t encrypt;     /* 加密开关 0:关闭 1:开启 */
//    rt_uint8_t key0;        /* 密钥0 */
//    rt_uint8_t key1;        /* 密钥1 */
//    rt_uint8_t lpwr;        /* 低功耗 0:关闭 1:开启 */
//};

/* E35-2G4T设备结构体 */
struct e35_device
{
    at_client_t client;
    struct e35_config config;
    rt_mutex_t lock;
    rt_bool_t initialized;
};

static struct e35_device *g_e35_dev = RT_NULL;

/* AT命令超时时间定义 */
#define E35_AT_TIMEOUT              (5 * RT_TICK_PER_SECOND)
#define E35_AT_MODE_SWITCH_TIMEOUT  (10 * RT_TICK_PER_SECOND)

/* E35模块初始化 */
static rt_err_t e35_init(struct e35_device *device)
{
    at_response_t resp = RT_NULL;
    rt_err_t result = RT_EOK;

    RT_ASSERT(device);
    RT_ASSERT(device->client);

    /* 创建响应结构体 */
    resp = at_create_resp(64, 0, E35_AT_TIMEOUT);
    if (resp == RT_NULL)
    {
        LOG_E("No memory for response structure!");
        return -RT_ENOMEM;
    }

    resp->line_num = 1;

    rt_mutex_take(device->lock, RT_WAITING_FOREVER);

    /* 进入配置模式 */
    result = at_exec_cmd(resp, "AT+MODE=1");
    if (result != RT_EOK)
    {
        LOG_E("AT+MODE=1 failed! ret:%d", result);
    }
    else
    {
        LOG_I("AT+MODE=1 OK");
        at_resp_get_line_by_kw(resp, "OK");
    }

    /* 查询设备型号 */
    if (at_exec_cmd(resp, "AT+DEVTYPE=?") == RT_EOK)
    {
        LOG_I("Device type: %s", at_resp_get_line(resp, 1));
    }

    /* 查询固件版本 */
    if (at_exec_cmd(resp, "AT+FWCODE=?") == RT_EOK)
    {
        LOG_I("Firmware: %s", at_resp_get_line(resp, 1));
    }

    device->initialized = RT_TRUE;
    LOG_I("E35 module initialized successfully");

__exit:
    rt_mutex_release(device->lock);
    at_delete_resp(resp);
    return result;
}

/* 切换到配置模式 */
static rt_err_t e35_enter_config_mode(struct e35_device *device)
{
    at_response_t resp = RT_NULL;
    rt_err_t result = RT_EOK;

    RT_ASSERT(device);

    resp = at_create_resp(32, 0, E35_AT_MODE_SWITCH_TIMEOUT);
    if (resp == RT_NULL)
    {
        return -RT_ENOMEM;
    }

    resp->line_num = 1;

    rt_mutex_take(device->lock, RT_WAITING_FOREVER);

    /* 切换到配置模式 */
    if (at_exec_cmd(resp, "AT+MODE=1") != RT_EOK)
    {
        LOG_E("Failed to enter config mode");
        result = -RT_ERROR;
        goto __exit;
    }

    /* 等待模式切换完成 */
    rt_thread_mdelay(200);
    
    device->config.mode = 1;
    LOG_D("Entered config mode");

__exit:
    rt_mutex_release(device->lock);
    at_delete_resp(resp);
    return result;
}

/* 切换到传输模式 */
static rt_err_t e35_enter_trans_mode(struct e35_device *device)
{
    at_response_t resp = RT_NULL;
    rt_err_t result = RT_EOK;

    RT_ASSERT(device);

    resp = at_create_resp(32, 0, E35_AT_MODE_SWITCH_TIMEOUT);
    if (resp == RT_NULL)
    {
        return -RT_ENOMEM;
    }

    resp->line_num = 1;

    rt_mutex_take(device->lock, RT_WAITING_FOREVER);

    /* 切换到传输模式 */
    if (at_exec_cmd(resp, "AT+MODE=0") != RT_EOK)
    {
        LOG_E("Failed to enter transmission mode");
        result = -RT_ERROR;
        goto __exit;
    }

    /* 等待模式切换完成 */
    rt_thread_mdelay(200);
    
    device->config.mode = 0;
    LOG_D("Entered transmission mode");

__exit:
    rt_mutex_release(device->lock);
    at_delete_resp(resp);
    return result;
}

/* 设置模块地址 */
static rt_err_t e35_set_address(struct e35_device *device, rt_uint8_t addr_h, rt_uint8_t addr_l)
{
    at_response_t resp = RT_NULL;
    rt_err_t result = RT_EOK;
    char cmd[32];

    RT_ASSERT(device);

    /* 确保在配置模式下 */
    if (device->config.mode != 1)
    {
        result = e35_enter_config_mode(device);
        if (result != RT_EOK)
        {
            return result;
        }
    }

    resp = at_create_resp(32, 0, E35_AT_TIMEOUT);
    if (resp == RT_NULL)
    {
        return -RT_ENOMEM;
    }

    resp->line_num = 1;

    rt_mutex_take(device->lock, RT_WAITING_FOREVER);

    rt_snprintf(cmd, sizeof(cmd), "AT+ADDR=%d,%d", addr_h, addr_l);
    
    if (at_exec_cmd(resp, cmd) != RT_EOK)
    {
        LOG_E("Failed to set address");
        result = -RT_ERROR;
        goto __exit;
    }

    device->config.addr_h = addr_h;
    device->config.addr_l = addr_l;
    LOG_D("Set address: %d,%d", addr_h, addr_l);

__exit:
    rt_mutex_release(device->lock);
    at_delete_resp(resp);
    return result;
}

/* 设置串口参数 */
static rt_err_t e35_set_uart(struct e35_device *device, rt_uint8_t baud, rt_uint8_t stopbit, rt_uint8_t parity)
{
    at_response_t resp = RT_NULL;
    rt_err_t result = RT_EOK;
    char cmd[32];

    RT_ASSERT(device);

    /* 确保在配置模式下 */
    if (device->config.mode != 1)
    {
        result = e35_enter_config_mode(device);
        if (result != RT_EOK)
        {
            return result;
        }
    }

    resp = at_create_resp(32, 0, E35_AT_TIMEOUT);
    if (resp == RT_NULL)
    {
        return -RT_ENOMEM;
    }

    resp->line_num = 1;

    rt_mutex_take(device->lock, RT_WAITING_FOREVER);

    rt_snprintf(cmd, sizeof(cmd), "AT+UART=%d,%d,%d", baud, stopbit, parity);
    
    if (at_exec_cmd(resp, cmd) != RT_EOK)
    {
        LOG_E("Failed to set UART parameters");
        result = -RT_ERROR;
        goto __exit;
    }

    device->config.baud = baud;
    device->config.stopbit = stopbit;
    device->config.parity = parity;
    LOG_D("Set UART: baud=%d, stop=%d, parity=%d", baud, stopbit, parity);

__exit:
    rt_mutex_release(device->lock);
    at_delete_resp(resp);
    return result;
}

/* 设置射频参数 */
static rt_err_t e35_set_rf_params(struct e35_device *device, rt_uint8_t rate, rt_uint8_t power, rt_uint8_t channel)
{
    at_response_t resp = RT_NULL;
    rt_err_t result = RT_EOK;
    char cmd[32];

    RT_ASSERT(device);

    /* 确保在配置模式下 */
    if (device->config.mode != 1)
    {
        result = e35_enter_config_mode(device);
        if (result != RT_EOK)
        {
            return result;
        }
    }

    resp = at_create_resp(32, 0, E35_AT_TIMEOUT);
    if (resp == RT_NULL)
    {
        return -RT_ENOMEM;
    }

    resp->line_num = 1;

    rt_mutex_take(device->lock, RT_WAITING_FOREVER);

    /* 设置空中速率 */
    rt_snprintf(cmd, sizeof(cmd), "AT+RATE=%d", rate);
    if (at_exec_cmd(resp, cmd) != RT_EOK)
    {
        LOG_E("Failed to set air rate");
        result = -RT_ERROR;
        goto __exit;
    }

    /* 设置发射功率 */
    rt_snprintf(cmd, sizeof(cmd), "AT+POWER=%d", power);
    if (at_exec_cmd(resp, cmd) != RT_EOK)
    {
        LOG_E("Failed to set power");
        result = -RT_ERROR;
        goto __exit;
    }

    /* 设置信道 */
    rt_snprintf(cmd, sizeof(cmd), "AT+CHANNEL=%d", channel);
    if (at_exec_cmd(resp, cmd) != RT_EOK)
    {
        LOG_E("Failed to set channel");
        result = -RT_ERROR;
        goto __exit;
    }

    device->config.rate = rate;
    device->config.power = power;
    device->config.channel = channel;
    LOG_D("Set RF params: rate=%d, power=%d, channel=%d", rate, power, channel);

__exit:
    rt_mutex_release(device->lock);
    at_delete_resp(resp);
    return result;
}

/* 设置传输模式 */
static rt_err_t e35_set_trans_mode(struct e35_device *device, rt_uint8_t trans)
{
    at_response_t resp = RT_NULL;
    rt_err_t result = RT_EOK;
    char cmd[32];

    RT_ASSERT(device);

    /* 确保在配置模式下 */
    if (device->config.mode != 1)
    {
        result = e35_enter_config_mode(device);
        if (result != RT_EOK)
        {
            return result;
        }
    }

    resp = at_create_resp(32, 0, E35_AT_TIMEOUT);
    if (resp == RT_NULL)
    {
        return -RT_ENOMEM;
    }

    resp->line_num = 1;

    rt_mutex_take(device->lock, RT_WAITING_FOREVER);

    rt_snprintf(cmd, sizeof(cmd), "AT+TRANS=%d", trans);
    
    if (at_exec_cmd(resp, cmd) != RT_EOK)
    {
        LOG_E("Failed to set transmission mode");
        result = -RT_ERROR;
        goto __exit;
    }

    device->config.trans = trans;
    LOG_D("Set transmission mode: %d", trans);

__exit:
    rt_mutex_release(device->lock);
    at_delete_resp(resp);
    return result;
}

/* 设置加密参数 */
static rt_err_t e35_set_encrypt(struct e35_device *device, rt_uint8_t encrypt, rt_uint8_t key0, rt_uint8_t key1)
{
    at_response_t resp = RT_NULL;
    rt_err_t result = RT_EOK;
    char cmd[32];

    RT_ASSERT(device);

    /* 确保在配置模式下 */
    if (device->config.mode != 1)
    {
        result = e35_enter_config_mode(device);
        if (result != RT_EOK)
        {
            return result;
        }
    }

    resp = at_create_resp(32, 0, E35_AT_TIMEOUT);
    if (resp == RT_NULL)
    {
        return -RT_ENOMEM;
    }

    resp->line_num = 1;

    rt_mutex_take(device->lock, RT_WAITING_FOREVER);

    /* 设置加密开关 */
    rt_snprintf(cmd, sizeof(cmd), "AT+ENCRYPT=%d", encrypt);
    if (at_exec_cmd(resp, cmd) != RT_EOK)
    {
        LOG_E("Failed to set encrypt switch");
        result = -RT_ERROR;
        goto __exit;
    }

    /* 如果开启加密，设置密钥 */
    if (encrypt)
    {
        rt_snprintf(cmd, sizeof(cmd), "AT+KEY=%d,%d", key0, key1);
        if (at_exec_cmd(resp, cmd) != RT_EOK)
        {
            LOG_E("Failed to set encrypt key");
            result = -RT_ERROR;
            goto __exit;
        }
    }

    device->config.encrypt = encrypt;
    device->config.key0 = key0;
    device->config.key1 = key1;
    LOG_D("Set encrypt: %d, key: %d,%d", encrypt, key0, key1);

__exit:
    rt_mutex_release(device->lock);
    at_delete_resp(resp);
    return result;
}

/* 获取当前配置 */
static rt_err_t e35_get_config(struct e35_device *device, struct e35_config *config)
{
    at_response_t resp = RT_NULL;
    rt_err_t result = RT_EOK;

    RT_ASSERT(device);
    RT_ASSERT(config);

    /* 确保在配置模式下 */
    if (device->config.mode != 1)
    {
        result = e35_enter_config_mode(device);
        if (result != RT_EOK)
        {
            return result;
        }
    }

    resp = at_create_resp(64, 0, E35_AT_TIMEOUT);
    if (resp == RT_NULL)
    {
        return -RT_ENOMEM;
    }

    resp->line_num = 1;

    rt_mutex_take(device->lock, RT_WAITING_FOREVER);

    /* 查询各个参数... */
    /* 这里可以添加更多的参数查询命令 */
    
    /* 复制当前配置 */
    rt_memcpy(config, &device->config, sizeof(struct e35_config));

__exit:
    rt_mutex_release(device->lock);
    at_delete_resp(resp);
    return result;
}

/* 复位模块 */
static rt_err_t e35_reset(struct e35_device *device)
{
    at_response_t resp = RT_NULL;
    rt_err_t result = RT_EOK;

    RT_ASSERT(device);

    resp = at_create_resp(32, 0, E35_AT_TIMEOUT);
    if (resp == RT_NULL)
    {
        return -RT_ENOMEM;
    }

    resp->line_num = 1;

    rt_mutex_take(device->lock, RT_WAITING_FOREVER);

    if (at_exec_cmd(resp, "AT+RESET") != RT_EOK)
    {
        LOG_E("Failed to reset module");
        result = -RT_ERROR;
        goto __exit;
    }

    /* 等待复位完成 */
    rt_thread_mdelay(1000);
    LOG_I("Module reset completed");

__exit:
    rt_mutex_release(device->lock);
    at_delete_resp(resp);
    return result;
}

/* 恢复出厂设置 */
static rt_err_t e35_restore_default(struct e35_device *device)
{
    at_response_t resp = RT_NULL;
    rt_err_t result = RT_EOK;

    RT_ASSERT(device);

    resp = at_create_resp(32, 0, E35_AT_TIMEOUT);
    if (resp == RT_NULL)
    {
        return -RT_ENOMEM;
    }

    resp->line_num = 1;

    rt_mutex_take(device->lock, RT_WAITING_FOREVER);

    if (at_exec_cmd(resp, "AT+DEFAULT") != RT_EOK)
    {
        LOG_E("Failed to restore default settings");
        result = -RT_ERROR;
        goto __exit;
    }

    LOG_I("Restored to default settings");

__exit:
    rt_mutex_release(device->lock);
    at_delete_resp(resp);
    return result;
}

/* 创建E35设备 */
struct e35_device* e35_create(const char *client_name)
{
    struct e35_device *device = RT_NULL;

    RT_ASSERT(client_name);

    device = rt_calloc(1, sizeof(struct e35_device));
    if (device == RT_NULL)
    {
        LOG_E("No memory for E35 device");
        return RT_NULL;
    }

    /* 获取AT客户端 */
    device->client = at_client_get(client_name);
    if (device->client == RT_NULL)
    {
        LOG_E("Get AT client (%s) failed", client_name);
        rt_free(device);
        return RT_NULL;
    }

    /* 创建互斥锁 */
    device->lock = rt_mutex_create("e35_lock", RT_IPC_FLAG_FIFO);
    if (device->lock == RT_NULL)
    {
        LOG_E("Create mutex failed");
        rt_free(device);
        return RT_NULL;
    }

    /* 初始化默认配置 */
    device->config.mode = 0;        /* 传输模式 */
    device->config.addr_h = 0;      /* 地址高字节 */
    device->config.addr_l = 0;      /* 地址低字节 */
    device->config.baud = 4;        /* 115200bps */
    device->config.rate = 0;        /* 250Kbps */
    device->config.power = 0;       /* 默认功率 */
    device->config.channel = 0;     /* 信道0 */
    device->config.trans = 0;       /* 透明传输 */

    device->initialized = RT_FALSE;

    return device;
}

/* 销毁E35设备 */
void e35_destroy(struct e35_device *device)
{
    if (device)
    {
        if (device->lock)
        {
            rt_mutex_delete(device->lock);
        }
        rt_free(device);
    }
}

/* 设备接口函数 */
rt_err_t e35_device_init(struct e35_device *device)
{
    return e35_init(device);
}

rt_err_t e35_device_set_address(struct e35_device *device, rt_uint8_t addr_h, rt_uint8_t addr_l)
{
    return e35_set_address(device, addr_h, addr_l);
}

rt_err_t e35_device_set_uart(struct e35_device *device, rt_uint8_t baud, rt_uint8_t stopbit, rt_uint8_t parity)
{
    return e35_set_uart(device, baud, stopbit, parity);
}

rt_err_t e35_device_set_rf_params(struct e35_device *device, rt_uint8_t rate, rt_uint8_t power, rt_uint8_t channel)
{
    return e35_set_rf_params(device, rate, power, channel);
}

rt_err_t e35_device_set_trans_mode(struct e35_device *device, rt_uint8_t trans)
{
    return e35_set_trans_mode(device, trans);
}

rt_err_t e35_device_set_encrypt(struct e35_device *device, rt_uint8_t encrypt, rt_uint8_t key0, rt_uint8_t key1)
{
    return e35_set_encrypt(device, encrypt, key0, key1);
}

rt_err_t e35_device_enter_config_mode(struct e35_device *device)
{
    return e35_enter_config_mode(device);
}

rt_err_t e35_device_enter_trans_mode(struct e35_device *device)
{
    return e35_enter_trans_mode(device);
}

rt_err_t e35_device_get_config(struct e35_device *device, struct e35_config *config)
{
    return e35_get_config(device, config);
}

rt_err_t e35_device_reset(struct e35_device *device)
{
    return e35_reset(device);
}

rt_err_t e35_device_restore_default(struct e35_device *device)
{
    return e35_restore_default(device);
}

/* MSH命令实现 */
#ifdef RT_USING_FINSH
#include <finsh.h>

static void e35_test(int argc, char **argv)
{
    struct e35_device *device;
    
    if (argc < 2)
    {
        rt_kprintf("Usage: e35_test <client_name>\n");
        return;
    }

    device = e35_create(argv[1]);
    if (device == RT_NULL)
    {
        rt_kprintf("Create E35 device failed\n");
        return;
    }

    if (e35_device_init(device) != RT_EOK)
    {
        rt_kprintf("Initialize E35 device failed\n");
        e35_destroy(device);
        return;
    }

    rt_kprintf("E35 device test passed\n");
    
    /* 设置一些参数进行测试 */
    e35_device_set_address(device, 0, 1);
    e35_device_set_rf_params(device, 0, 0, 4);
    e35_device_enter_trans_mode(device);

    g_e35_dev = device;
}
MSH_CMD_EXPORT(e35_test, Test E35 module);

static void e35_config(int argc, char **argv)
{
    if (g_e35_dev == RT_NULL)
    {
        rt_kprintf("E35 device not initialized\n");
        return;
    }

    if (argc < 2)
    {
        rt_kprintf("Usage: e35_config <parameter> [value]\n");
        rt_kprintf("Parameters:\n");
        rt_kprintf("  addr <high> <low>    - Set address\n");
        rt_kprintf("  power <value>        - Set power (0-26)\n");
        rt_kprintf("  channel <value>      - Set channel (0-80)\n");
        rt_kprintf("  reset                - Reset module\n");
        return;
    }

    if (rt_strcmp(argv[1], "addr") == 0 && argc >= 4)
    {
        int addr_h = atoi(argv[2]);
        int addr_l = atoi(argv[3]);
        if (e35_device_set_address(g_e35_dev, addr_h, addr_l) == RT_EOK)
        {
            rt_kprintf("Set address to %d,%d\n", addr_h, addr_l);
        }
        else
        {
            rt_kprintf("Failed to set address\n");
        }
    }
    else if (rt_strcmp(argv[1], "power") == 0 && argc >= 3)
    {
        int power = atoi(argv[2]);
        if (e35_device_set_rf_params(g_e35_dev, g_e35_dev->config.rate, power, g_e35_dev->config.channel) == RT_EOK)
        {
            rt_kprintf("Set power to %d\n", power);
        }
        else
        {
            rt_kprintf("Failed to set power\n");
        }
    }
    else if (rt_strcmp(argv[1], "channel") == 0 && argc >= 3)
    {
        int channel = atoi(argv[2]);
        if (e35_device_set_rf_params(g_e35_dev, g_e35_dev->config.rate, g_e35_dev->config.power, channel) == RT_EOK)
        {
            rt_kprintf("Set channel to %d\n", channel);
        }
        else
        {
            rt_kprintf("Failed to set channel\n");
        }
    }
    else if (rt_strcmp(argv[1], "reset") == 0)
    {
        if (e35_device_reset(g_e35_dev) == RT_EOK)
        {
            rt_kprintf("Module reset completed\n");
        }
        else
        {
            rt_kprintf("Failed to reset module\n");
        }
    }
}
MSH_CMD_EXPORT(e35_config, Configure E35 module);

#endif /* RT_USING_FINSH */
