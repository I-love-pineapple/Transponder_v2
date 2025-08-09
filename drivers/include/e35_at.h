/*
 * E35-2G4T系列无线模块AT驱动头文件
 */

#ifndef __E35_AT_H__
#define __E35_AT_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* E35-2G4T模块配置参数结构体 */
struct e35_config
{
    rt_uint8_t mode;        /* 工作模式 0:传输模式 1:配置模式 */
    rt_uint8_t addr_h;      /* 地址高字节 */
    rt_uint8_t addr_l;      /* 地址低字节 */
    rt_uint8_t baud;        /* 串口波特率 0:9600 1:19200 2:38400 3:57600 4:115200 */
    rt_uint8_t stopbit;     /* 停止位 0:1位 1:1.5位 2:2位 */
    rt_uint8_t parity;      /* 校验位 0:无 1:偶 2:奇 */
    rt_uint8_t rate;        /* 空中速率 0:250K 1:500K 2:1M 3:2M */
    rt_uint8_t power;       /* 发射功率 0-26 */
    rt_uint8_t channel;     /* 信道 0-80 */
    rt_uint8_t trans;       /* 传输方式 0:透明 1:定点 */
    rt_uint8_t packet;      /* 分包长度 23-48 */
    rt_uint8_t drssi;       /* RSSI开关 0:关闭 1:开启 */
    rt_uint8_t encrypt;     /* 加密开关 0:关闭 1:开启 */
    rt_uint8_t key0;        /* 密钥0 */
    rt_uint8_t key1;        /* 密钥1 */
    rt_uint8_t lpwr;        /* 低功耗 0:关闭 1:开启 */
};

/* E35设备句柄 */
struct e35_device;

/* 波特率定义 */
#define E35_BAUD_9600       0
#define E35_BAUD_19200      1
#define E35_BAUD_38400      2
#define E35_BAUD_57600      3
#define E35_BAUD_115200     4

/* 停止位定义 */
#define E35_STOPBIT_1       0
#define E35_STOPBIT_1_5     1
#define E35_STOPBIT_2       2

/* 校验位定义 */
#define E35_PARITY_NONE     0
#define E35_PARITY_EVEN     1
#define E35_PARITY_ODD      2

/* 空中速率定义 */
#define E35_RATE_250K       0
#define E35_RATE_500K       1
#define E35_RATE_1M         2
#define E35_RATE_2M         3

/* 传输方式定义 */
#define E35_TRANS_TRANSPARENT   0
#define E35_TRANS_FIXED         1

/* 工作模式定义 */
#define E35_MODE_TRANSMISSION   0
#define E35_MODE_CONFIG         1

/* API函数声明 */

/**
 * 创建E35设备实例
 * @param client_name AT客户端名称
 * @return E35设备句柄，失败返回RT_NULL
 */
struct e35_device* e35_create(const char *client_name);

/**
 * 销毁E35设备实例
 * @param device E35设备句柄
 */
void e35_destroy(struct e35_device *device);

/**
 * 初始化E35设备
 * @param device E35设备句柄
 * @return RT_EOK成功，其他值失败
 */
rt_err_t e35_device_init(struct e35_device *device);

/**
 * 设置模块地址
 * @param device E35设备句柄
 * @param addr_h 地址高字节 (0-255)
 * @param addr_l 地址低字节 (0-255)
 * @return RT_EOK成功，其他值失败
 */
rt_err_t e35_device_set_address(struct e35_device *device, rt_uint8_t addr_h, rt_uint8_t addr_l);

/**
 * 设置串口参数
 * @param device E35设备句柄
 * @param baud 波特率 (E35_BAUD_*)
 * @param stopbit 停止位 (E35_STOPBIT_*)
 * @param parity 校验位 (E35_PARITY_*)
 * @return RT_EOK成功，其他值失败
 */
rt_err_t e35_device_set_uart(struct e35_device *device, rt_uint8_t baud, rt_uint8_t stopbit, rt_uint8_t parity);

/**
 * 设置射频参数
 * @param device E35设备句柄
 * @param rate 空中速率 (E35_RATE_*)
 * @param power 发射功率 (0-26)
 * @param channel 信道 (0-80)
 * @return RT_EOK成功，其他值失败
 */
rt_err_t e35_device_set_rf_params(struct e35_device *device, rt_uint8_t rate, rt_uint8_t power, rt_uint8_t channel);

/**
 * 设置传输模式
 * @param device E35设备句柄
 * @param trans 传输方式 (E35_TRANS_*)
 * @return RT_EOK成功，其他值失败
 */
rt_err_t e35_device_set_trans_mode(struct e35_device *device, rt_uint8_t trans);

/**
 * 设置加密参数
 * @param device E35设备句柄
 * @param encrypt 加密开关 (0:关闭 1:开启)
 * @param key0 密钥0 (0-255)
 * @param key1 密钥1 (0-255)
 * @return RT_EOK成功，其他值失败
 */
rt_err_t e35_device_set_encrypt(struct e35_device *device, rt_uint8_t encrypt, rt_uint8_t key0, rt_uint8_t key1);

/**
 * 进入配置模式
 * @param device E35设备句柄
 * @return RT_EOK成功，其他值失败
 */
rt_err_t e35_device_enter_config_mode(struct e35_device *device);

/**
 * 进入传输模式
 * @param device E35设备句柄
 * @return RT_EOK成功，其他值失败
 */
rt_err_t e35_device_enter_trans_mode(struct e35_device *device);

/**
 * 获取当前配置
 * @param device E35设备句柄
 * @param config 配置结构体指针
 * @return RT_EOK成功，其他值失败
 */
rt_err_t e35_device_get_config(struct e35_device *device, struct e35_config *config);

/**
 * 复位模块
 * @param device E35设备句柄
 * @return RT_EOK成功，其他值失败
 */
rt_err_t e35_device_reset(struct e35_device *device);

/**
 * 恢复出厂设置
 * @param device E35设备句柄
 * @return RT_EOK成功，其他值失败
 */
rt_err_t e35_device_restore_default(struct e35_device *device);

#ifdef __cplusplus
}
#endif

#endif /* __E35_AT_H__ */