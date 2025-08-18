#include <qled.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <drv_common.h>
#include "led_app.h"


#define DBG_TAG "led_app"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define LED_RED_PIN     GET_PIN(A, 6)   /**< 红色LED GPIO引脚 PA6 */
#define LED_GREEN_PIN   GET_PIN(A, 7)   /**< 绿色LED GPIO引脚 PA7 */
#define LED_BLUE_PIN    GET_PIN(A, 5)   /**< 蓝色LED GPIO引脚 PA5 */

/**
 * @brief LED应用程序初始化函数
 * @return int 初始化结果
 * @retval 0 初始化成功
 * @retval -1 初始化失败
 * @note 初始化LED应用程序，添加LED到qled管理器
 */
int led_app_init(void)
{
    /* 添加LED到qled管理器 */
    if (qled_add(LED_RED_PIN, 0) != 0)
    {
        LOG_E("添加红色LED失败");
        return -1;
    }

    if (qled_add(LED_GREEN_PIN, 0) != 0)
    {
        LOG_E("添加绿色LED失败");
        return -1;
    }

    if (qled_add(LED_BLUE_PIN, 0) != 0)
    {
        LOG_E("添加蓝色LED失败");
        return -1;
    }

    LOG_I("LED应用程序初始化成功");

    /* 初始化为未入网状态 */
    led_app_switch(LED_STATUS_NOT_JOINED);

    return 0;
}

static void upload_over_cb(void)
{
    led_app_switch(LED_STATUS_JOINED);
}

/**
 * @brief LED状态切换函数
 * @param status LED状态
 * @note 根据不同状态设置相应的LED显示模式
 */
void led_app_switch(rt_uint8_t status)
{
    /* 首先关闭所有LED */
    qled_set_off(LED_RED_PIN);
    qled_set_off(LED_GREEN_PIN);
    qled_set_off(LED_BLUE_PIN);

    switch (status)
    {
        case LED_STATUS_NOT_JOINED:
            /* 未入网：红色LED慢闪（10ms亮，990ms灭） */
            LOG_I("LED状态切换：未入网");
            qled_set_blink(LED_RED_PIN, 10, 990);
            break;

        case LED_STATUS_JOINING:
            /* 入网中：红色LED快闪（200ms亮，200ms灭） */
            LOG_I("LED状态切换：入网中");
            qled_set_blink(LED_RED_PIN, 200, 200);
            break;

        case LED_STATUS_JOINED:
            /* 已入网：绿色LED常亮 */
            LOG_I("LED状态切换：已入网");
            qled_set_blink(LED_GREEN_PIN, 10, 990);
            break;

        case LED_STATUS_KEY_UPLOADING:
            /* 上传按键中：蓝色LED快闪（10ms亮，190ms灭） */
            LOG_I("LED状态切换：上传按键中");
            qled_set_blink(LED_BLUE_PIN, 10, 190);
            break;

        case LED_STATUS_KEY_UPLOAD_DONE:
            LOG_I("LED状态切换：按键上传完成");
            qled_set_special(LED_BLUE_PIN, (const u16[]){1000, 0}, 2, upload_over_cb);
            break;

        case LED_STATUS_SLEEP:
            /* 休眠：所有LED关闭 */
            LOG_I("LED状态切换：休眠");
            /* 所有LED已在开头关闭，无需额外操作 */
            break;

        default:
            LOG_W("未知的LED状态：%d", status);
            /* 默认关闭所有LED */
            break;
    }
}

static led_test(int argc, char *argv[])
{
    led_app_switch(atoi(argv[1]));
}
MSH_CMD_EXPORT(led_test, led_test [status]);
