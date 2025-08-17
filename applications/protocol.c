#include "protocol.h"
#include <string.h>

#define DBG_TAG "protocol"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/* =========================== 基础函数实现 =========================== */

uint16_t protocol_calculate_checksum(const uint8_t *data, uint16_t length)
{
    uint16_t checksum = 0;
    
    if (data == NULL) {
        return 0;
    }
    
    for (uint16_t i = 0; i < length; i++) {
        checksum += data[i];
    }
    
    return checksum;
}

bool protocol_verify_checksum(const uint8_t *frame, uint16_t frame_len)
{
    if (frame == NULL || frame_len < PROTOCOL_MIN_FRAME_SIZE) {
        return false;
    }
    
    // 提取帧中的校验和
    uint16_t received_checksum = (frame[frame_len - 4]) | frame[frame_len - 3] << 8;
    
    // 计算校验和（不包括校验和本身和结束符）
    uint16_t calculated_checksum = protocol_calculate_checksum(frame, frame_len - 4);
    
    return (received_checksum == calculated_checksum);
}

protocol_error_t protocol_create_frame(uint8_t frame_type, const uint8_t *payload, 
                                     uint8_t payload_len, uint8_t *frame_data, uint16_t *frame_len)
{
    if (frame_data == NULL || frame_len == NULL) {
        return PROTOCOL_INVALID_PARAM;
    }
    
    if (payload_len > PROTOCOL_MAX_PAYLOAD_SIZE) {
        return PROTOCOL_BUFFER_FULL;
    }
    
    uint16_t required_len = 4 + payload_len + 4; // 头部4字节 + 载荷 + 尾部4字节
    if (*frame_len < required_len) {
        return PROTOCOL_BUFFER_FULL;
    }
    
    uint16_t offset = 0;
    
    // 构建帧头
    frame_data[offset++] = (PROTOCOL_START_FLAG >> 8) & 0xFF;
    frame_data[offset++] = PROTOCOL_START_FLAG & 0xFF;
    frame_data[offset++] = payload_len;
    frame_data[offset++] = frame_type;
    
    // 复制载荷
    if (payload_len > 0 && payload != NULL) {
        memcpy(&frame_data[offset], payload, payload_len);
        offset += payload_len;
    }
    
    // 计算校验和（从起始符到载荷结束）
    uint16_t checksum = protocol_calculate_checksum(frame_data, offset);
    
    // 添加校验和
    frame_data[offset++] = checksum & 0xFF;
    frame_data[offset++] = (checksum >> 8) & 0xFF;
    
    // 添加结束符
    frame_data[offset++] = (PROTOCOL_END_FLAG >> 8) & 0xFF;
    frame_data[offset++] = PROTOCOL_END_FLAG & 0xFF;
    
    *frame_len = offset;
    
    return PROTOCOL_OK;
}

protocol_error_t protocol_parse_frame(const uint8_t *data, uint16_t data_len, protocol_frame_t *frame)
{
    if (data == NULL || frame == NULL) {
        return PROTOCOL_INVALID_PARAM;
    }
    
    if (data_len < PROTOCOL_MIN_FRAME_SIZE) {
        return PROTOCOL_FRAME_TOO_SHORT;
    }
    
    if (data_len > PROTOCOL_MAX_FRAME_SIZE) {
        return PROTOCOL_FRAME_TOO_LONG;
    }
    
    // 验证起始符
    uint16_t start_flag = (data[0] << 8) | data[1];
    if (start_flag != PROTOCOL_START_FLAG) {
        return PROTOCOL_ERROR;
    }
    
    // 验证结束符
    uint16_t end_flag = (data[data_len - 2] << 8) | data[data_len - 1];
    if (end_flag != PROTOCOL_END_FLAG) {
        return PROTOCOL_ERROR;
    }
    
    // 验证校验和
    if (!protocol_verify_checksum(data, data_len)) {
        LOG_E("checksum error rx:%x cal:%x", (data[data_len - 4] << 8) | data[data_len - 3], protocol_calculate_checksum(data, data_len - 4));
        return PROTOCOL_CHECKSUM_ERROR;
    }
    
    // 解析帧头
    frame->header.start_flag = start_flag;
    frame->header.length = data[2];
    frame->header.frame_type = data[3];
    
    // 检查长度字段的合理性
    uint16_t expected_frame_len = 4 + frame->header.length + 4; // 头(4) + 载荷 + 尾(4)
    if (expected_frame_len != data_len) {
        return PROTOCOL_ERROR;
    }
    
    // 复制载荷数据
    if (frame->header.length > 0) {
        memcpy(frame->payload, &data[4], frame->header.length);
    }
    
    // 解析帧尾
    frame->tail.checksum = (data[data_len - 4] << 8) | data[data_len - 3];
    frame->tail.end_flag = end_flag;
    
    return PROTOCOL_OK;
}

/* =========================== 特定帧创建函数 =========================== */

protocol_error_t protocol_create_join_beacon_frame(uint32_t gateway_id, uint8_t work_channel,
                                                  uint8_t *frame_data, uint16_t *frame_len)
{
    join_beacon_payload_t payload;
    payload.gateway_id = gateway_id;
    payload.work_channel = work_channel;
    memset(payload.reserved, 0, sizeof(payload.reserved));
    
    return protocol_create_frame(FRAME_JOIN_BEACON, (uint8_t*)&payload, sizeof(payload), frame_data, frame_len);
}

protocol_error_t protocol_create_join_req_frame(uint32_t gateway_id, uint32_t node_id, uint8_t rssi,
                                               uint8_t *frame_data, uint16_t *frame_len)
{
    join_req_payload_t payload;
    payload.gateway_id = gateway_id;
    payload.node_id = node_id;
    payload.rssi = rssi;
    memset(payload.reserved, 0, sizeof(payload.reserved));
    
    return protocol_create_frame(FRAME_JOIN_REQ, (uint8_t*)&payload, sizeof(payload), frame_data, frame_len);
}

protocol_error_t protocol_create_join_resp_frame(uint32_t gateway_id, uint32_t node_id, 
                                                uint8_t work_channel, uint8_t timeslot,
                                                uint8_t *frame_data, uint16_t *frame_len)
{
    join_resp_payload_t payload;
    payload.gateway_id = gateway_id;
    payload.node_id = node_id;
    payload.work_channel = work_channel;
    payload.timeslot = timeslot;
    memset(payload.reserved, 0, sizeof(payload.reserved));
    
    return protocol_create_frame(FRAME_JOIN_RESP, (uint8_t*)&payload, sizeof(payload), frame_data, frame_len);
}

protocol_error_t protocol_create_join_ack_frame(uint32_t gateway_id, uint32_t node_id, uint8_t status,
                                               uint8_t *frame_data, uint16_t *frame_len)
{
    join_ack_payload_t payload;
    payload.gateway_id = gateway_id;
    payload.node_id = node_id;
    payload.status = status;
    memset(payload.reserved, 0, sizeof(payload.reserved));
    
    return protocol_create_frame(FRAME_JOIN_ACK, (uint8_t*)&payload, sizeof(payload), frame_data, frame_len);
}

protocol_error_t protocol_create_answer_req_frame(uint32_t gateway_id, uint32_t node_id, 
                                                 uint16_t sequence, uint8_t option, uint8_t battery,
                                                 uint8_t *frame_data, uint16_t *frame_len)
{
    answer_req_payload_t payload;
    payload.gateway_id = gateway_id;
    payload.node_id = node_id;
    payload.sequence = sequence;
    payload.option = option;
    payload.battery = battery;
    
    return protocol_create_frame(FRAME_ANSWER_REQ, (uint8_t*)&payload, sizeof(payload), frame_data, frame_len);
}

protocol_error_t protocol_create_answer_ack_frame(uint32_t gateway_id, uint32_t node_id, uint8_t status,
                                                 uint8_t *frame_data, uint16_t *frame_len)
{
    answer_ack_payload_t payload;
    payload.gateway_id = gateway_id;
    payload.node_id = node_id;
    payload.status = status;
    memset(payload.reserved, 0, sizeof(payload.reserved));
    
    return protocol_create_frame(FRAME_ANSWER_ACK, (uint8_t*)&payload, sizeof(payload), frame_data, frame_len);
}

protocol_error_t protocol_create_reset_cmd_frame(uint32_t gateway_id, uint32_t node_id,
                                                uint8_t *frame_data, uint16_t *frame_len)
{
    // RESET_CMD帧使用ANSWER_ACK的载荷格式
    answer_ack_payload_t payload;
    payload.gateway_id = gateway_id;
    payload.node_id = node_id;
    payload.status = 0x00;
    memset(payload.reserved, 0, sizeof(payload.reserved));
    
    return protocol_create_frame(FRAME_RESET_CMD, (uint8_t*)&payload, sizeof(payload), frame_data, frame_len);
}

/* =========================== 载荷提取函数 =========================== */

protocol_error_t protocol_extract_join_beacon_payload(const protocol_frame_t *frame, join_beacon_payload_t *payload)
{
    if (frame == NULL || payload == NULL) {
        return PROTOCOL_INVALID_PARAM;
    }
    
    if (frame->header.frame_type != FRAME_JOIN_BEACON) {
        return PROTOCOL_ERROR;
    }
    
    if (frame->header.length != sizeof(join_beacon_payload_t)) {
        return PROTOCOL_ERROR;
    }
    
    memcpy(payload, frame->payload, sizeof(join_beacon_payload_t));
    return PROTOCOL_OK;
}

protocol_error_t protocol_extract_join_req_payload(const protocol_frame_t *frame, join_req_payload_t *payload)
{
    if (frame == NULL || payload == NULL) {
        return PROTOCOL_INVALID_PARAM;
    }
    
    if (frame->header.frame_type != FRAME_JOIN_REQ) {
        return PROTOCOL_ERROR;
    }
    
    if (frame->header.length != sizeof(join_req_payload_t)) {
        return PROTOCOL_ERROR;
    }
    
    memcpy(payload, frame->payload, sizeof(join_req_payload_t));
    return PROTOCOL_OK;
}

protocol_error_t protocol_extract_join_resp_payload(const protocol_frame_t *frame, join_resp_payload_t *payload)
{
    if (frame == NULL || payload == NULL) {
        return PROTOCOL_INVALID_PARAM;
    }
    
    if (frame->header.frame_type != FRAME_JOIN_RESP) {
        return PROTOCOL_ERROR;
    }
    
    if (frame->header.length != sizeof(join_resp_payload_t)) {
        return PROTOCOL_ERROR;
    }
    
    memcpy(payload, frame->payload, sizeof(join_resp_payload_t));
    return PROTOCOL_OK;
}

protocol_error_t protocol_extract_join_ack_payload(const protocol_frame_t *frame, join_ack_payload_t *payload)
{
    if (frame == NULL || payload == NULL) {
        return PROTOCOL_INVALID_PARAM;
    }
    
    if (frame->header.frame_type != FRAME_JOIN_ACK) {
        return PROTOCOL_ERROR;
    }
    
    if (frame->header.length != sizeof(join_ack_payload_t)) {
        return PROTOCOL_ERROR;
    }
    
    memcpy(payload, frame->payload, sizeof(join_ack_payload_t));
    return PROTOCOL_OK;
}
    
protocol_error_t protocol_extract_answer_req_payload(const protocol_frame_t *frame, answer_req_payload_t *payload)
{
    if (frame == NULL || payload == NULL) {
        return PROTOCOL_INVALID_PARAM;
    }
    
    if (frame->header.frame_type != FRAME_ANSWER_REQ) {
        return PROTOCOL_ERROR;
    }
    
    if (frame->header.length != sizeof(answer_req_payload_t)) {
        return PROTOCOL_ERROR;
    }
    
    memcpy(payload, frame->payload, sizeof(answer_req_payload_t));
    return PROTOCOL_OK;
}

protocol_error_t protocol_extract_answer_ack_payload(const protocol_frame_t *frame, answer_ack_payload_t *payload)
{
    if (frame == NULL || payload == NULL) {
        return PROTOCOL_INVALID_PARAM;
    }
    
    if (frame->header.frame_type != FRAME_ANSWER_ACK) {
        return PROTOCOL_ERROR;
    }
    
    if (frame->header.length != sizeof(answer_ack_payload_t)) {
        return PROTOCOL_ERROR;
    }
    
    memcpy(payload, frame->payload, sizeof(answer_ack_payload_t));
    return PROTOCOL_OK;
}

/* =========================== 工具函数 =========================== */

const char* protocol_get_frame_type_string(uint8_t frame_type)
{
    switch (frame_type) {
        case FRAME_JOIN_BEACON:  return "JOIN_BEACON";
        case FRAME_JOIN_REQ:     return "JOIN_REQ";
        case FRAME_JOIN_RESP:    return "JOIN_RESP";
        case FRAME_JOIN_ACK:     return "JOIN_ACK";
        case FRAME_ANSWER_REQ:   return "ANSWER_REQ";
        case FRAME_ANSWER_ACK:   return "ANSWER_ACK";
        case FRAME_RESET_CMD:    return "RESET_CMD";
        default:                 return "UNKNOWN";
    }
}

const char* protocol_error_to_string(protocol_error_t error)
{
    switch (error) {
        case PROTOCOL_OK:             return "OK";
        case PROTOCOL_ERROR:          return "General Error";
        case PROTOCOL_INVALID_PARAM:  return "Invalid Parameter";
        case PROTOCOL_BUFFER_FULL:    return "Buffer Full";
        case PROTOCOL_CHECKSUM_ERROR: return "Checksum Error";
        case PROTOCOL_FRAME_TOO_SHORT: return "Frame Too Short";
        case PROTOCOL_FRAME_TOO_LONG: return "Frame Too Long";
        default:                      return "Unknown Error";
    }
}
