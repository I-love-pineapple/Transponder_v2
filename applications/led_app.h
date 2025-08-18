#pragma once

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief LED状态枚举定义
 * @note 未入网、入网中、已入网、上传按键中、按键上传完成、休眠
 */
typedef enum {
    LED_STATUS_NOT_JOINED = 0,      /**< 未入网状态 */
    LED_STATUS_JOINING,             /**< 入网中状态 */
    LED_STATUS_JOINED,              /**< 已入网状态 */
    LED_STATUS_KEY_UPLOADING,       /**< 上传按键中状态 */
    LED_STATUS_KEY_UPLOAD_DONE,     /**< 按键上传完成状态 */
    LED_STATUS_SLEEP,               /**< 休眠状态 */
    LED_STATUS_MAX                  /**< 状态数量 */
} led_status_t;

/**
 * @brief LED应用程序初始化函数
 * @return int 初始化结果
 * @retval 0 初始化成功
 * @retval -1 初始化失败
 * @note 初始化LED应用程序，添加LED到qled管理器
 */
int led_app_init(void);

/**
 * @brief LED状态切换函数
 * @param status LED状态
 * @note 根据不同状态设置相应的LED显示模式
 */
void led_app_switch(rt_uint8_t status);

#ifdef __cplusplus
}
#endif