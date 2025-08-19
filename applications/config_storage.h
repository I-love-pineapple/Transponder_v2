/**
 * @file config_storage.h
 * @brief 配置存储接口头文件
 * @details 提供配置数据的掉电存储功能接口
 * @author RT-Thread Team
 * @date 2024-12-27
 * @version 1.0.0
 * 
 * @copyright Copyright (c) 2024 RT-Thread Development Team
 * 
 * @par 修改日志:
 * <table>
 * <tr><th>日期       <th>版本  <th>作者     <th>说明
 * <tr><td>2024-12-27 <td>1.0.0 <td>RT-Thread Team <td>创建配置存储接口
 * </table>
 */

#ifndef __CONFIG_STORAGE_H__
#define __CONFIG_STORAGE_H__

#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup Config_Storage_Types 配置存储类型定义
 * @{
 */

/**
 * @brief 存储类型枚举
 */
typedef enum
{
    STORAGE_TYPE_FLASH = 0,             /**< 内部Flash存储 */
    STORAGE_TYPE_EEPROM,                /**< 外部EEPROM存储 */
    STORAGE_TYPE_FILE,                  /**< 文件系统存储 */
    STORAGE_TYPE_MAX
} storage_type_t;

/**
 * @brief 存储区域信息结构体
 */
typedef struct
{
    storage_type_t type;                /**< 存储类型 */
    rt_uint32_t base_addr;              /**< 基地址 */
    rt_uint32_t size;                   /**< 大小 */
    rt_uint32_t sector_size;            /**< 扇区大小 */
    rt_bool_t write_protected;          /**< 是否写保护 */
} storage_region_t;

/**
 * @brief 存储操作结果枚举
 */
typedef enum
{
    STORAGE_OK = 0,                     /**< 操作成功 */
    STORAGE_ERROR,                      /**< 一般错误 */
    STORAGE_INVALID_PARAM,              /**< 参数无效 */
    STORAGE_NOT_FOUND,                  /**< 未找到 */
    STORAGE_NO_SPACE,                   /**< 空间不足 */
    STORAGE_WRITE_PROTECTED,            /**< 写保护 */
    STORAGE_CRC_ERROR,                  /**< CRC校验错误 */
    STORAGE_TIMEOUT,                    /**< 操作超时 */
    STORAGE_MAX
} storage_result_t;

/**
 * @}
 */

/**
 * @defgroup Config_Storage_Constants 配置存储常量
 * @{
 */

/** @brief 配置数据最大大小 */
#define CONFIG_DATA_MAX_SIZE            1024

/** @brief 配置数据备份份数 */
#define CONFIG_BACKUP_COUNT             2

/** @brief Flash扇区大小 */
#define FLASH_SECTOR_SIZE               2048

/** @brief 配置区域基地址 (根据实际硬件调整) */
#define CONFIG_FLASH_BASE_ADDR          0x0807F000  /* STM32L4最后一个扇区 */

/**
 * @}
 */

/**
 * @defgroup Config_Storage_Functions 配置存储函数
 * @{
 */

/**
 * @brief 初始化配置存储
 * @return storage_result_t 初始化结果
 * @retval STORAGE_OK 初始化成功
 * @retval STORAGE_ERROR 初始化失败
 */
storage_result_t config_storage_init(void);

/**
 * @brief 反初始化配置存储
 * @return storage_result_t 反初始化结果
 * @retval STORAGE_OK 反初始化成功
 */
storage_result_t config_storage_deinit(void);

/**
 * @brief 读取配置数据
 * @param data 数据缓冲区
 * @param size 数据大小
 * @return storage_result_t 读取结果
 * @retval STORAGE_OK 读取成功
 * @retval STORAGE_NOT_FOUND 未找到配置
 * @retval STORAGE_CRC_ERROR CRC校验失败
 * @retval STORAGE_INVALID_PARAM 参数无效
 */
storage_result_t config_storage_read(void *data, rt_uint32_t size);

/**
 * @brief 写入配置数据
 * @param data 数据缓冲区
 * @param size 数据大小
 * @return storage_result_t 写入结果
 * @retval STORAGE_OK 写入成功
 * @retval STORAGE_ERROR 写入失败
 * @retval STORAGE_NO_SPACE 空间不足
 * @retval STORAGE_WRITE_PROTECTED 写保护
 * @retval STORAGE_INVALID_PARAM 参数无效
 */
storage_result_t config_storage_write(const void *data, rt_uint32_t size);

/**
 * @brief 擦除配置数据
 * @return storage_result_t 擦除结果
 * @retval STORAGE_OK 擦除成功
 * @retval STORAGE_ERROR 擦除失败
 * @retval STORAGE_WRITE_PROTECTED 写保护
 */
storage_result_t config_storage_erase(void);

/**
 * @brief 验证配置数据完整性
 * @return storage_result_t 验证结果
 * @retval STORAGE_OK 数据完整
 * @retval STORAGE_CRC_ERROR CRC校验失败
 * @retval STORAGE_NOT_FOUND 未找到配置
 */
storage_result_t config_storage_verify(void);

/**
 * @brief 获取存储区域信息
 * @param region 存储区域信息结构体指针
 * @return storage_result_t 获取结果
 * @retval STORAGE_OK 获取成功
 * @retval STORAGE_INVALID_PARAM 参数无效
 */
storage_result_t config_storage_get_region_info(storage_region_t *region);

/**
 * @brief 检查存储空间使用情况
 * @param used_size 已使用大小
 * @param free_size 剩余大小
 * @param total_size 总大小
 * @return storage_result_t 检查结果
 * @retval STORAGE_OK 检查成功
 * @retval STORAGE_INVALID_PARAM 参数无效
 */
storage_result_t config_storage_get_usage(rt_uint32_t *used_size, rt_uint32_t *free_size, rt_uint32_t *total_size);

/**
 * @brief 格式化存储区域
 * @return storage_result_t 格式化结果
 * @retval STORAGE_OK 格式化成功
 * @retval STORAGE_ERROR 格式化失败
 * @retval STORAGE_WRITE_PROTECTED 写保护
 * @warning 此操作会清除所有配置数据，请谨慎使用
 */
storage_result_t config_storage_format(void);

/**
 * @brief 备份配置数据
 * @return storage_result_t 备份结果
 * @retval STORAGE_OK 备份成功
 * @retval STORAGE_ERROR 备份失败
 * @retval STORAGE_NO_SPACE 空间不足
 */
storage_result_t config_storage_backup(void);

/**
 * @brief 恢复配置数据
 * @param backup_index 备份索引 (0 ~ CONFIG_BACKUP_COUNT-1)
 * @return storage_result_t 恢复结果
 * @retval STORAGE_OK 恢复成功
 * @retval STORAGE_NOT_FOUND 备份不存在
 * @retval STORAGE_CRC_ERROR 备份数据损坏
 * @retval STORAGE_INVALID_PARAM 参数无效
 */
storage_result_t config_storage_restore(rt_uint8_t backup_index);

/**
 * @brief 获取存储结果字符串
 * @param result 存储结果
 * @return const char* 结果字符串
 */
const char* config_storage_result_to_string(storage_result_t result);

/**
 * @brief 获取存储类型字符串
 * @param type 存储类型
 * @return const char* 类型字符串
 */
const char* config_storage_type_to_string(storage_type_t type);

/**
 * @brief 执行存储自检
 * @return storage_result_t 自检结果
 * @retval STORAGE_OK 自检通过
 * @retval STORAGE_ERROR 自检失败
 */
storage_result_t config_storage_self_test(void);

/**
 * @}
 */

/**
 * @defgroup Config_Storage_Advanced 高级存储功能
 * @{
 */

/**
 * @brief 设置写保护
 * @param enable 是否启用写保护
 * @return storage_result_t 设置结果
 * @retval STORAGE_OK 设置成功
 * @retval STORAGE_ERROR 设置失败
 */
storage_result_t config_storage_set_write_protection(rt_bool_t enable);

/**
 * @brief 设置存储加密
 * @param enable 是否启用加密
 * @param key 加密密钥
 * @param key_len 密钥长度
 * @return storage_result_t 设置结果
 * @retval STORAGE_OK 设置成功
 * @retval STORAGE_ERROR 设置失败
 * @retval STORAGE_INVALID_PARAM 参数无效
 * @note 预留接口，当前实现可为空
 */
storage_result_t config_storage_set_encryption(rt_bool_t enable, const rt_uint8_t *key, rt_uint32_t key_len);

/**
 * @brief 获取存储统计信息
 * @param read_count 读取次数
 * @param write_count 写入次数
 * @param error_count 错误次数
 * @return storage_result_t 获取结果
 * @retval STORAGE_OK 获取成功
 * @retval STORAGE_INVALID_PARAM 参数无效
 */
storage_result_t config_storage_get_statistics(rt_uint32_t *read_count, rt_uint32_t *write_count, rt_uint32_t *error_count);

/**
 * @brief 重置存储统计信息
 * @return storage_result_t 重置结果
 * @retval STORAGE_OK 重置成功
 */
storage_result_t config_storage_reset_statistics(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __CONFIG_STORAGE_H__ */
