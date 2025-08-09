/**
 * @file drv_storage.h
 * @brief 内部Flash存储驱动头文件
 * @details 定义内部Flash存储驱动的接口函数、宏定义和数据结构，提供2KB存储区域管理功能
 * @author RT-Thread Team
 * @date 2024-08-09
 * @version 1.0.0
 * 
 * @copyright Copyright (c) 2024 RT-Thread Development Team
 * 
 * @par 修改日志:
 * <table>
 * <tr><th>日期       <th>版本  <th>作者     <th>说明
 * <tr><td>2024-08-09 <td>1.0.0 <td>RT-Thread Team <td>首次创建
 * </table>
 */

#ifndef __DRV_STORAGE_H__
#define __DRV_STORAGE_H__

#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"
#include "drv_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup Storage_Driver_Exported_Types 存储驱动导出类型定义
 * @{
 */

/**
 * @brief 存储驱动错误码枚举定义
 * @note 用于标识存储驱动操作的结果状态
 */
typedef enum
{
    STORAGE_ERR_OK = 0,         /**< 操作成功 */
    STORAGE_ERR_INVALID_PARAM,  /**< 参数无效 */
    STORAGE_ERR_OUT_OF_RANGE,   /**< 地址超出范围 */
    STORAGE_ERR_ALIGNMENT,      /**< 地址对齐错误 */
    STORAGE_ERR_ERASE_FAILED,   /**< 擦除失败 */
    STORAGE_ERR_WRITE_FAILED,   /**< 写入失败 */
    STORAGE_ERR_READ_FAILED,    /**< 读取失败 */
    STORAGE_ERR_NOT_INIT        /**< 驱动未初始化 */
} storage_err_t;

/**
 * @brief 存储驱动状态结构体定义
 * @note 用于表示存储驱动的当前状态信息
 */
typedef struct
{
    rt_bool_t initialized;      /**< 初始化状态标志 */
    rt_uint32_t start_addr;     /**< 存储区域起始地址 */
    rt_uint32_t end_addr;       /**< 存储区域结束地址 */
    rt_uint32_t capacity;       /**< 存储区域总容量（字节） */
    rt_uint32_t write_count;    /**< 写入操作计数 */
    rt_uint32_t read_count;     /**< 读取操作计数 */
    rt_uint32_t erase_count;    /**< 擦除操作计数 */
} storage_info_t;

/**
 * @}
 */

/**
 * @defgroup Storage_Driver_Exported_Macros 存储驱动导出宏定义
 * @{
 */

/** @brief 存储区域容量定义（2KB） */
#define STORAGE_CAPACITY            2048

/** @brief 存储区域起始地址定义 */
#define STORAGE_START_ADDRESS       (STM32_FLASH_END_ADDRESS - STORAGE_CAPACITY)

/** @brief 存储区域结束地址定义 */
#define STORAGE_END_ADDRESS         STM32_FLASH_END_ADDRESS

/** @brief Flash写入对齐要求（8字节） */
#define STORAGE_WRITE_ALIGNMENT     8

/** @brief 最小写入单位（8字节） */
#define STORAGE_MIN_WRITE_SIZE      8

/** @brief 最大单次写入大小 */
#define STORAGE_MAX_WRITE_SIZE      STORAGE_CAPACITY

/** @brief 地址对齐检查宏 */
#define IS_STORAGE_ALIGNED(addr)    (((addr) % STORAGE_WRITE_ALIGNMENT) == 0)

/** @brief 地址范围检查宏 */
#define IS_STORAGE_RANGE_VALID(offset, size) \
    (((offset) + (size)) <= STORAGE_CAPACITY)

/**
 * @}
 */

/**
 * @defgroup Storage_Driver_Exported_Functions 存储驱动导出函数
 * @{
 */

/**
 * @brief 存储驱动初始化函数
 * @return storage_err_t 初始化结果
 * @retval STORAGE_ERR_OK 初始化成功
 * @retval STORAGE_ERR_INVALID_PARAM 参数无效
 * @note 该函数初始化存储驱动，分配RAM缓存区域，设置存储区域地址范围
 * 
 * @par 示例:
 * @code
 * storage_err_t result = drv_storage_init();
 * if (result == STORAGE_ERR_OK)
 * {
 *     rt_kprintf("Storage driver initialized successfully\n");
 * }
 * @endcode
 */
storage_err_t drv_storage_init(void);

/**
 * @brief 存储驱动反初始化函数
 * @return storage_err_t 反初始化结果
 * @retval STORAGE_ERR_OK 反初始化成功
 * @note 该函数释放存储驱动资源，清理RAM缓存区域
 * 
 * @par 示例:
 * @code
 * storage_err_t result = drv_storage_deinit();
 * if (result == STORAGE_ERR_OK)
 * {
 *     rt_kprintf("Storage driver deinitialized successfully\n");
 * }
 * @endcode
 */
storage_err_t drv_storage_deinit(void);

/**
 * @brief 从存储区域读取数据
 * @param offset 相对于存储区域起始地址的偏移量（字节）
 * @param buffer 用于存储读取数据的缓冲区指针
 * @param size 要读取的数据大小（字节）
 * @return storage_err_t 读取结果
 * @retval STORAGE_ERR_OK 读取成功
 * @retval STORAGE_ERR_INVALID_PARAM 参数无效
 * @retval STORAGE_ERR_OUT_OF_RANGE 地址超出范围
 * @retval STORAGE_ERR_NOT_INIT 驱动未初始化
 * @retval STORAGE_ERR_READ_FAILED 读取失败
 * @note 该函数从指定偏移量读取指定大小的数据到缓冲区
 * 
 * @par 示例:
 * @code
 * rt_uint8_t read_buffer[256];
 * storage_err_t result = drv_storage_read(0, read_buffer, 256);
 * if (result == STORAGE_ERR_OK)
 * {
 *     rt_kprintf("Data read successfully\n");
 * }
 * @endcode
 */
storage_err_t drv_storage_read(rt_uint32_t offset, rt_uint8_t *buffer, rt_uint32_t size);

/**
 * @brief 向存储区域写入数据
 * @param offset 相对于存储区域起始地址的偏移量（字节）
 * @param buffer 要写入的数据缓冲区指针
 * @param size 要写入的数据大小（字节）
 * @return storage_err_t 写入结果
 * @retval STORAGE_ERR_OK 写入成功
 * @retval STORAGE_ERR_INVALID_PARAM 参数无效
 * @retval STORAGE_ERR_OUT_OF_RANGE 地址超出范围
 * @retval STORAGE_ERR_ALIGNMENT 地址对齐错误
 * @retval STORAGE_ERR_NOT_INIT 驱动未初始化
 * @retval STORAGE_ERR_ERASE_FAILED 擦除失败
 * @retval STORAGE_ERR_WRITE_FAILED 写入失败
 * @note 该函数实现擦除前写入机制，使用RAM缓存确保8字节对齐写入
 * 
 * @par 示例:
 * @code
 * rt_uint8_t write_data[256] = {0x01, 0x02, 0x03, ...};
 * storage_err_t result = drv_storage_write(0, write_data, 256);
 * if (result == STORAGE_ERR_OK)
 * {
 *     rt_kprintf("Data written successfully\n");
 * }
 * @endcode
 */
storage_err_t drv_storage_write(rt_uint32_t offset, const rt_uint8_t *buffer, rt_uint32_t size);

/**
 * @brief 擦除整个存储区域
 * @return storage_err_t 擦除结果
 * @retval STORAGE_ERR_OK 擦除成功
 * @retval STORAGE_ERR_NOT_INIT 驱动未初始化
 * @retval STORAGE_ERR_ERASE_FAILED 擦除失败
 * @note 该函数擦除整个2KB存储区域，擦除后所有数据将变为0xFF
 * 
 * @par 示例:
 * @code
 * storage_err_t result = drv_storage_erase();
 * if (result == STORAGE_ERR_OK)
 * {
 *     rt_kprintf("Storage area erased successfully\n");
 * }
 * @endcode
 */
storage_err_t drv_storage_erase(void);

/**
 * @brief 获取存储区域容量
 * @return rt_uint32_t 存储区域总容量（字节）
 * @note 该函数返回存储区域的总容量，固定为2048字节
 * 
 * @par 示例:
 * @code
 * rt_uint32_t capacity = drv_storage_get_capacity();
 * rt_kprintf("Storage capacity: %d bytes\n", capacity);
 * @endcode
 */
rt_uint32_t drv_storage_get_capacity(void);

/**
 * @brief 获取存储驱动状态信息
 * @param info 用于存储状态信息的结构体指针
 * @return storage_err_t 获取结果
 * @retval STORAGE_ERR_OK 获取成功
 * @retval STORAGE_ERR_INVALID_PARAM 参数无效
 * @retval STORAGE_ERR_NOT_INIT 驱动未初始化
 * @note 该函数获取存储驱动的详细状态信息，包括地址范围、操作计数等
 * 
 * @par 示例:
 * @code
 * storage_info_t info;
 * storage_err_t result = drv_storage_get_info(&info);
 * if (result == STORAGE_ERR_OK)
 * {
 *     rt_kprintf("Storage info - Start: 0x%08X, End: 0x%08X, Capacity: %d\n",
 *                info.start_addr, info.end_addr, info.capacity);
 * }
 * @endcode
 */
storage_err_t drv_storage_get_info(storage_info_t *info);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __DRV_STORAGE_H__ */
