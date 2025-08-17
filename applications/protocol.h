#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================== 宏定义 =========================== */

/** @defgroup Protocol_Constants 协议常量定义
 * @{
 */
#define PROTOCOL_START_FLAG         0xA5A5      /**< 帧起始标识 */
#define PROTOCOL_END_FLAG           0xFAFA      /**< 帧结束标识 */
#define PROTOCOL_MAX_PAYLOAD_SIZE   255         /**< 最大载荷长度 */
#define PROTOCOL_MIN_FRAME_SIZE     8           /**< 最小帧长度 */
#define PROTOCOL_MAX_FRAME_SIZE     263         /**< 最大帧长度 */
/** @} */

/** @defgroup Frame_Types 帧类型定义
 * @{
 */
typedef enum {
    FRAME_JOIN_BEACON    = 0x01,    /**<下行 网关入网广播帧 */
    FRAME_JOIN_REQ       = 0x02,    /**<上行 节点入网请求帧 */
    FRAME_JOIN_RESP      = 0x03,    /**<下行 网关入网响应帧 */
    FRAME_JOIN_ACK       = 0x04,    /**<上行 节点入网确认帧 */
    FRAME_ANSWER_REQ     = 0x11,    /**<上行 应答数据上传帧 */
    FRAME_ANSWER_ACK     = 0x12,    /**<下行 应答确认帧 */
    FRAME_RESET_CMD      = 0x14     /**<下行 重置命令帧 */
} frame_type_t;
/** @} */

/** @defgroup Error_Codes 错误码定义
 * @{
 */
typedef enum {
    PROTOCOL_OK          = 0x00,    /**< 成功 */
    PROTOCOL_ERROR       = 0x01,    /**< 一般错误 */
    PROTOCOL_INVALID_PARAM = 0x02,  /**< 无效参数 */
    PROTOCOL_BUFFER_FULL = 0x03,    /**< 缓冲区满 */
    PROTOCOL_CHECKSUM_ERROR = 0x04, /**< 校验和错误 */
    PROTOCOL_FRAME_TOO_SHORT = 0x05, /**< 帧长度过短 */
    PROTOCOL_FRAME_TOO_LONG = 0x06   /**< 帧长度过长 */
} protocol_error_t;
/** @} */

/* =========================== 数据结构定义 =========================== */

/** @defgroup Data_Structures 数据结构定义
 * @{
 */

/**
 * @brief 通用帧头结构
 */
typedef struct __attribute__((packed)) {
    uint16_t start_flag;    /**< 起始符 0xA5A5 */
    uint8_t  length;        /**< 数据载荷长度 */
    uint8_t  frame_type;    /**< 帧类型 */
} frame_header_t;

/**
 * @brief 通用帧尾结构
 */
typedef struct __attribute__((packed)) {
    uint16_t checksum;      /**< 累加和 */
    uint16_t end_flag;      /**< 结束符 0xFAFA */
} frame_tail_t;

/**
 * @brief JOIN_BEACON帧载荷
 */
typedef struct __attribute__((packed)) {
    uint32_t gateway_id;    /**< 网关ID */
    uint8_t  work_channel;  /**< 工作频道 */
    uint8_t  reserved[3];   /**< 保留字段 */
} join_beacon_payload_t;

/**
 * @brief JOIN_REQ帧载荷
 */
typedef struct __attribute__((packed)) {
    uint32_t gateway_id;    /**< 网关ID */
    uint32_t node_id;       /**< 节点ID */
    uint8_t  rssi;          /**< 信号强度 */
    uint8_t  reserved[3];   /**< 保留字段 */
} join_req_payload_t;

/**
 * @brief JOIN_RESP帧载荷
 */
typedef struct __attribute__((packed)) {
    uint32_t gateway_id;    /**< 网关ID */
    uint32_t node_id;       /**< 节点ID */
    uint8_t  work_channel;  /**< 工作频道 */
    uint8_t  timeslot;      /**< 时隙 */
    uint8_t  reserved[2];   /**< 保留字段 */
} join_resp_payload_t;

/**
 * @brief JOIN_ACK帧载荷
 */
typedef struct __attribute__((packed)) {
    uint32_t gateway_id;    /**< 网关ID */
    uint32_t node_id;       /**< 节点ID */
    uint8_t  status;        /**< 状态 */
    uint8_t  reserved[3];   /**< 保留字段 */
} join_ack_payload_t;

/**
 * @brief ANSWER_REQ帧载荷
 */
typedef struct __attribute__((packed)) {
    uint32_t gateway_id;    /**< 网关ID */
    uint32_t node_id;       /**< 节点ID */
    uint16_t sequence;      /**< 序列号 */
    uint8_t  option;        /**< 选项 */
    uint8_t  battery;       /**< 电池电量(%) */
} answer_req_payload_t;

/**
 * @brief ANSWER_ACK帧载荷
 */
typedef struct __attribute__((packed)) {
    uint32_t gateway_id;    /**< 网关ID */
    uint32_t node_id;       /**< 节点ID */
    uint8_t  status;        /**< 状态 */
    uint8_t  reserved[3];   /**< 保留字段 */
} answer_ack_payload_t;

/**
 * @brief 完整协议帧结构
 */
typedef struct __attribute__((packed)) {
    frame_header_t header;                          /**< 帧头 */
    uint8_t payload[PROTOCOL_MAX_PAYLOAD_SIZE];     /**< 载荷数据 */
    frame_tail_t tail;                              /**< 帧尾 */
} protocol_frame_t;
/** @} */

/* =========================== 函数声明 =========================== */

/** @defgroup Frame_Processing 帧处理函数
 * @{
 */

/**
 * @brief 计算帧校验和
 * @param[in] data 数据指针
 * @param[in] length 数据长度
 * @return 累加和
 */
uint16_t protocol_calculate_checksum(const uint8_t *data, uint16_t length);

/**
 * @brief 验证帧校验和
 * @param[in] frame 帧数据指针
 * @param[in] frame_len 帧长度
 * @return true=校验成功, false=校验失败
 */
bool protocol_verify_checksum(const uint8_t *frame, uint16_t frame_len);

/**
 * @brief 创建通用帧
 * @param[in] frame_type 帧类型
 * @param[in] payload 载荷数据
 * @param[in] payload_len 载荷长度
 * @param[out] frame_data 输出帧数据
 * @param[in,out] frame_len 输入缓冲区大小，输出实际帧长度
 * @return 错误码
 */
protocol_error_t protocol_create_frame(uint8_t frame_type, const uint8_t *payload, 
                                     uint8_t payload_len, uint8_t *frame_data, uint16_t *frame_len);

/**
 * @brief 解析接收到的帧
 * @param[in] data 接收数据
 * @param[in] data_len 数据长度
 * @param[out] frame 解析后的帧结构
 * @return 错误码
 */
protocol_error_t protocol_parse_frame(const uint8_t *data, uint16_t data_len, protocol_frame_t *frame);
/** @} */

/** @defgroup Specific_Frame_Functions 特定帧类型函数
 * @{
 */

/**
 * @brief 创建JOIN_BEACON帧
 * @param[in] gateway_id 网关ID
 * @param[in] work_channel 工作频道
 * @param[out] frame_data 输出帧数据
 * @param[in,out] frame_len 输入缓冲区大小，输出实际帧长度
 * @return 错误码
 */
protocol_error_t protocol_create_join_beacon_frame(uint32_t gateway_id, uint8_t work_channel,
                                                  uint8_t *frame_data, uint16_t *frame_len);

/**
 * @brief 创建JOIN_REQ帧
 * @param[in] gateway_id 网关ID
 * @param[in] node_id 节点ID
 * @param[in] rssi 信号强度
 * @param[out] frame_data 输出帧数据
 * @param[in,out] frame_len 输入缓冲区大小，输出实际帧长度
 * @return 错误码
 */
protocol_error_t protocol_create_join_req_frame(uint32_t gateway_id, uint32_t node_id, uint8_t rssi,
                                               uint8_t *frame_data, uint16_t *frame_len);

/**
 * @brief 创建JOIN_RESP帧
 * @param[in] gateway_id 网关ID
 * @param[in] node_id 节点ID
 * @param[in] work_channel 工作频道
 * @param[in] timeslot 时隙
 * @param[out] frame_data 输出帧数据
 * @param[in,out] frame_len 输入缓冲区大小，输出实际帧长度
 * @return 错误码
 */
protocol_error_t protocol_create_join_resp_frame(uint32_t gateway_id, uint32_t node_id, 
                                                uint8_t work_channel, uint8_t timeslot,
                                                uint8_t *frame_data, uint16_t *frame_len);

/**
 * @brief 创建JOIN_ACK帧
 * @param[in] gateway_id 网关ID
 * @param[in] node_id 节点ID
 * @param[in] status 状态
 * @param[out] frame_data 输出帧数据
 * @param[in,out] frame_len 输入缓冲区大小，输出实际帧长度
 * @return 错误码
 */
protocol_error_t protocol_create_join_ack_frame(uint32_t gateway_id, uint32_t node_id, uint8_t status,
                                               uint8_t *frame_data, uint16_t *frame_len);

/**
 * @brief 创建ANSWER_REQ帧
 * @param[in] gateway_id 网关ID
 * @param[in] node_id 节点ID
 * @param[in] sequence 序列号
 * @param[in] option 选项
 * @param[in] battery 电池电量
 * @param[out] frame_data 输出帧数据
 * @param[in,out] frame_len 输入缓冲区大小，输出实际帧长度
 * @return 错误码
 */
protocol_error_t protocol_create_answer_req_frame(uint32_t gateway_id, uint32_t node_id, 
                                                 uint16_t sequence, uint8_t option, uint8_t battery,
                                                 uint8_t *frame_data, uint16_t *frame_len);

/**
 * @brief 创建ANSWER_ACK帧
 * @param[in] gateway_id 网关ID
 * @param[in] node_id 节点ID
 * @param[in] status 状态
 * @param[out] frame_data 输出帧数据
 * @param[in,out] frame_len 输入缓冲区大小，输出实际帧长度
 * @return 错误码
 */
protocol_error_t protocol_create_answer_ack_frame(uint32_t gateway_id, uint32_t node_id, uint8_t status,
                                                 uint8_t *frame_data, uint16_t *frame_len);

/**
 * @brief 创建RESET_CMD帧
 * @param[in] gateway_id 网关ID
 * @param[in] node_id 节点ID
 * @param[out] frame_data 输出帧数据
 * @param[in,out] frame_len 输入缓冲区大小，输出实际帧长度
 * @return 错误码
 */
protocol_error_t protocol_create_reset_cmd_frame(uint32_t gateway_id, uint32_t node_id,
                                                uint8_t *frame_data, uint16_t *frame_len);
/** @} */

/** @defgroup Payload_Extract_Functions 载荷提取函数
 * @{
 */

/**
 * @brief 提取JOIN_BEACON帧载荷
 * @param[in] frame 协议帧
 * @param[out] payload 载荷结构
 * @return 错误码
 */
protocol_error_t protocol_extract_join_beacon_payload(const protocol_frame_t *frame, join_beacon_payload_t *payload);

/**
 * @brief 提取JOIN_REQ帧载荷
 * @param[in] frame 协议帧
 * @param[out] payload 载荷结构
 * @return 错误码
 */
protocol_error_t protocol_extract_join_req_payload(const protocol_frame_t *frame, join_req_payload_t *payload);

/**
 * @brief 提取JOIN_RESP帧载荷
 * @param[in] frame 协议帧
 * @param[out] payload 载荷结构
 * @return 错误码
 */
protocol_error_t protocol_extract_join_resp_payload(const protocol_frame_t *frame, join_resp_payload_t *payload);

/**
 * @brief 提取JOIN_ACK帧载荷
 * @param[in] frame 协议帧
 * @param[out] payload 载荷结构
 * @return 错误码
 */
protocol_error_t protocol_extract_join_ack_payload(const protocol_frame_t *frame, join_ack_payload_t *payload);

/**
 * @brief 提取ANSWER_REQ帧载荷
 * @param[in] frame 协议帧
 * @param[out] payload 载荷结构
 * @return 错误码
 */
protocol_error_t protocol_extract_answer_req_payload(const protocol_frame_t *frame, answer_req_payload_t *payload);

/**
 * @brief 提取ANSWER_ACK帧载荷
 * @param[in] frame 协议帧
 * @param[out] payload 载荷结构
 * @return 错误码
 */
protocol_error_t protocol_extract_answer_ack_payload(const protocol_frame_t *frame, answer_ack_payload_t *payload);
/** @} */

/** @defgroup Utility_Functions 工具函数
 * @{
 */

/**
 * @brief 获取帧类型字符串
 * @param[in] frame_type 帧类型
 * @return 帧类型字符串
 */
const char* protocol_get_frame_type_string(uint8_t frame_type);

/**
 * @brief 将错误码转换为字符串
 * @param[in] error 错误码
 * @return 错误描述字符串
 */
const char* protocol_error_to_string(protocol_error_t error);
/** @} */

#ifdef __cplusplus
}
#endif