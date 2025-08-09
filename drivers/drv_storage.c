/**
 * @file drv_storage.c
 * @brief 内部Flash存储驱动实现文件
 * @details 实现内部Flash存储驱动的核心功能，包括2KB存储区域管理、RAM缓存机制、8字节对齐写入等
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

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include "board.h"
#include "drv_flash.h"
#include "drv_storage.h"

/**
 * @defgroup Storage_Driver_Private_Defines 存储驱动私有宏定义
 * @{
 */

#define DBG_TAG "drv.storage"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/**
 * @}
 */

/**
 * @defgroup Storage_Driver_Private_Variables 存储驱动私有变量
 * @{
 */

/**
 * @brief 存储驱动状态信息
 * @note 保存驱动的初始化状态和统计信息
 */
static storage_info_t g_storage_info = {0};

/**
 * @brief RAM缓存区域
 * @note 用于实现擦除前写入机制，确保8字节对齐写入
 */
static rt_uint8_t g_storage_cache[STORAGE_CAPACITY] = {0};

/**
 * @brief 驱动初始化状态标志
 * @note 用于快速检查驱动是否已初始化
 */
static rt_bool_t g_storage_initialized = RT_FALSE;

/**
 * @}
 */

/**
 * @defgroup Storage_Driver_Private_Functions 存储驱动私有函数
 * @{
 */

/**
 * @brief 参数有效性检查
 * @param offset 偏移量
 * @param buffer 缓冲区指针
 * @param size 数据大小
 * @return storage_err_t 检查结果
 * @retval STORAGE_ERR_OK 参数有效
 * @retval STORAGE_ERR_INVALID_PARAM 参数无效
 * @retval STORAGE_ERR_OUT_OF_RANGE 地址超出范围
 * @note 该函数检查传入参数的有效性
 */
static storage_err_t _storage_check_params(rt_uint32_t offset, const void *buffer, rt_uint32_t size)
{
    /* 检查缓冲区指针 */
    if (buffer == RT_NULL)
    {
        LOG_E("缓冲区指针为空");
        return STORAGE_ERR_INVALID_PARAM;
    }
    
    /* 检查数据大小 */
    if (size == 0)
    {
        LOG_E("数据大小为0");
        return STORAGE_ERR_INVALID_PARAM;
    }
    
    /* 检查地址范围 */
    if (!IS_STORAGE_RANGE_VALID(offset, size))
    {
        LOG_E("地址超出范围: offset=%d, size=%d, capacity=%d", offset, size, STORAGE_CAPACITY);
        return STORAGE_ERR_OUT_OF_RANGE;
    }
    
    return STORAGE_ERR_OK;
}

/**
 * @brief 从Flash读取数据到RAM缓存
 * @return storage_err_t 读取结果
 * @retval STORAGE_ERR_OK 读取成功
 * @retval STORAGE_ERR_READ_FAILED 读取失败
 * @note 该函数将整个存储区域的数据读取到RAM缓存中
 */
static storage_err_t _storage_load_cache(void)
{
    int result;
    
    LOG_D("正在从Flash读取数据到缓存...");
    
    /* 从Flash读取数据到缓存 */
    result = stm32_flash_read(STORAGE_START_ADDRESS, g_storage_cache, STORAGE_CAPACITY);
    if (result != STORAGE_CAPACITY)
    {
        LOG_E("从Flash读取数据失败: 期望读取%d字节，实际读取%d字节", STORAGE_CAPACITY, result);
        return STORAGE_ERR_READ_FAILED;
    }
    
    LOG_D("缓存加载完成");
    return STORAGE_ERR_OK;
}

/**
 * @brief 将RAM缓存数据写入Flash
 * @return storage_err_t 写入结果
 * @retval STORAGE_ERR_OK 写入成功
 * @retval STORAGE_ERR_ERASE_FAILED 擦除失败
 * @retval STORAGE_ERR_WRITE_FAILED 写入失败
 * @note 该函数先擦除存储区域，然后将缓存数据写入Flash
 */
static storage_err_t _storage_flush_cache(void)
{
    int result;
    rt_uint32_t write_addr;
    rt_uint32_t remaining_size;
    rt_uint32_t write_size;
    rt_uint32_t cache_offset;
    
    LOG_D("正在擦除存储区域...");
    
    /* 擦除存储区域 */
    result = stm32_flash_erase(STORAGE_START_ADDRESS, STORAGE_CAPACITY);
    if (result < 0)
    {
        LOG_E("擦除存储区域失败: 错误码=%d", result);
        g_storage_info.erase_count++;
        return STORAGE_ERR_ERASE_FAILED;
    }
    
    LOG_D("存储区域擦除完成");
    
    /* 8字节对齐写入缓存数据 */
    write_addr = STORAGE_START_ADDRESS;
    remaining_size = STORAGE_CAPACITY;
    cache_offset = 0;
    
    LOG_D("正在写入缓存数据到Flash...");
    
    while (remaining_size > 0)
    {
        /* 计算本次写入大小（8字节对齐） */
        write_size = (remaining_size >= STORAGE_WRITE_ALIGNMENT) ? STORAGE_WRITE_ALIGNMENT : remaining_size;
        
        /* 如果不足8字节，需要补齐到8字节 */
        if (write_size < STORAGE_WRITE_ALIGNMENT)
        {
            rt_uint8_t aligned_buffer[STORAGE_WRITE_ALIGNMENT];
            
            /* 复制数据到对齐缓冲区 */
            memcpy(aligned_buffer, &g_storage_cache[cache_offset], write_size);
            
            /* 剩余部分填充0xFF */
            memset(&aligned_buffer[write_size], 0xFF, STORAGE_WRITE_ALIGNMENT - write_size);
            
            /* 写入对齐的数据 */
            result = stm32_flash_write(write_addr, aligned_buffer, STORAGE_WRITE_ALIGNMENT);
            if (result < 0)
            {
                LOG_E("写入Flash失败: 地址=0x%08X, 错误码=%d", write_addr, result);
                return STORAGE_ERR_WRITE_FAILED;
            }
            
            break; /* 最后一次写入完成 */
        }
        else
        {
            /* 直接写入8字节对齐的数据 */
            result = stm32_flash_write(write_addr, &g_storage_cache[cache_offset], write_size);
            if (result < 0)
            {
                LOG_E("写入Flash失败: 地址=0x%08X, 错误码=%d", write_addr, result);
                return STORAGE_ERR_WRITE_FAILED;
            }
        }
        
        /* 更新地址和大小 */
        write_addr += write_size;
        cache_offset += write_size;
        remaining_size -= write_size;
    }
    
    LOG_D("缓存数据写入完成");
    g_storage_info.erase_count++;
    
    return STORAGE_ERR_OK;
}

/**
 * @}
 */

/**
 * @defgroup Storage_Driver_Public_Functions 存储驱动公共函数
 * @{
 */

/**
 * @brief 存储驱动初始化函数
 */
storage_err_t drv_storage_init(void)
{
    storage_err_t result;
    
    LOG_I("正在初始化存储驱动...");
    
    /* 检查是否已经初始化 */
    if (g_storage_initialized)
    {
        LOG_W("存储驱动已经初始化");
        return STORAGE_ERR_OK;
    }
    
    /* 初始化状态信息 */
    memset(&g_storage_info, 0, sizeof(storage_info_t));
    g_storage_info.start_addr = STORAGE_START_ADDRESS;
    g_storage_info.end_addr = STORAGE_END_ADDRESS;
    g_storage_info.capacity = STORAGE_CAPACITY;
    
    /* 清空缓存 */
    memset(g_storage_cache, 0xFF, STORAGE_CAPACITY);
    
    /* 从Flash加载现有数据到缓存 */
    result = _storage_load_cache();
    if (result != STORAGE_ERR_OK)
    {
        LOG_E("加载Flash数据到缓存失败");
        return result;
    }
    
    /* 设置初始化标志 */
    g_storage_info.initialized = RT_TRUE;
    g_storage_initialized = RT_TRUE;
    
    LOG_I("存储驱动初始化完成");
    LOG_I("存储区域: 0x%08X - 0x%08X (%d字节)", 
          g_storage_info.start_addr, g_storage_info.end_addr, g_storage_info.capacity);
    
    return STORAGE_ERR_OK;
}

/**
 * @brief 存储驱动反初始化函数
 */
storage_err_t drv_storage_deinit(void)
{
    LOG_I("正在反初始化存储驱动...");
    
    /* 检查是否已经初始化 */
    if (!g_storage_initialized)
    {
        LOG_W("存储驱动未初始化");
        return STORAGE_ERR_OK;
    }
    
    /* 清空状态信息 */
    memset(&g_storage_info, 0, sizeof(storage_info_t));
    
    /* 清空缓存 */
    memset(g_storage_cache, 0, STORAGE_CAPACITY);
    
    /* 清除初始化标志 */
    g_storage_initialized = RT_FALSE;
    
    LOG_I("存储驱动反初始化完成");
    
    return STORAGE_ERR_OK;
}

/**
 * @brief 从存储区域读取数据
 */
storage_err_t drv_storage_read(rt_uint32_t offset, rt_uint8_t *buffer, rt_uint32_t size)
{
    storage_err_t result;

    LOG_D("读取数据: offset=%d, size=%d", offset, size);

    /* 检查驱动是否已初始化 */
    if (!g_storage_initialized)
    {
        LOG_E("存储驱动未初始化");
        return STORAGE_ERR_NOT_INIT;
    }

    /* 检查参数有效性 */
    result = _storage_check_params(offset, buffer, size);
    if (result != STORAGE_ERR_OK)
    {
        return result;
    }

    /* 从缓存中复制数据 */
    memcpy(buffer, &g_storage_cache[offset], size);

    /* 更新读取计数 */
    g_storage_info.read_count++;

    LOG_D("数据读取完成");

    return STORAGE_ERR_OK;
}

/**
 * @brief 向存储区域写入数据
 */
storage_err_t drv_storage_write(rt_uint32_t offset, const rt_uint8_t *buffer, rt_uint32_t size)
{
    storage_err_t result;

    LOG_D("写入数据: offset=%d, size=%d", offset, size);

    /* 检查驱动是否已初始化 */
    if (!g_storage_initialized)
    {
        LOG_E("存储驱动未初始化");
        return STORAGE_ERR_NOT_INIT;
    }

    /* 检查参数有效性 */
    result = _storage_check_params(offset, buffer, size);
    if (result != STORAGE_ERR_OK)
    {
        return result;
    }

    /* 将数据写入缓存 */
    memcpy(&g_storage_cache[offset], buffer, size);

    /* 将缓存数据刷新到Flash */
    result = _storage_flush_cache();
    if (result != STORAGE_ERR_OK)
    {
        LOG_E("刷新缓存到Flash失败");
        return result;
    }

    /* 更新写入计数 */
    g_storage_info.write_count++;

    LOG_D("数据写入完成");

    return STORAGE_ERR_OK;
}

/**
 * @brief 擦除整个存储区域
 */
storage_err_t drv_storage_erase(void)
{
    int result;

    LOG_I("正在擦除存储区域...");

    /* 检查驱动是否已初始化 */
    if (!g_storage_initialized)
    {
        LOG_E("存储驱动未初始化");
        return STORAGE_ERR_NOT_INIT;
    }

    /* 擦除Flash存储区域 */
    result = stm32_flash_erase(STORAGE_START_ADDRESS, STORAGE_CAPACITY);
    if (result < 0)
    {
        LOG_E("擦除存储区域失败: 错误码=%d", result);
        return STORAGE_ERR_ERASE_FAILED;
    }

    /* 清空缓存（设置为0xFF） */
    memset(g_storage_cache, 0xFF, STORAGE_CAPACITY);

    /* 更新擦除计数 */
    g_storage_info.erase_count++;

    LOG_I("存储区域擦除完成");

    return STORAGE_ERR_OK;
}

/**
 * @brief 获取存储区域容量
 */
rt_uint32_t drv_storage_get_capacity(void)
{
    return STORAGE_CAPACITY;
}

/**
 * @brief 获取存储驱动状态信息
 */
storage_err_t drv_storage_get_info(storage_info_t *info)
{
    /* 检查参数有效性 */
    if (info == RT_NULL)
    {
        LOG_E("信息结构体指针为空");
        return STORAGE_ERR_INVALID_PARAM;
    }

    /* 检查驱动是否已初始化 */
    if (!g_storage_initialized)
    {
        LOG_E("存储驱动未初始化");
        return STORAGE_ERR_NOT_INIT;
    }

    /* 复制状态信息 */
    memcpy(info, &g_storage_info, sizeof(storage_info_t));

    return STORAGE_ERR_OK;
}

/**
 * @}
 */
