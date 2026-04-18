#ifndef OMINIFLOW_PROTOCOL_H__
#define OMINIFLOW_PROTOCOL_H__

#include <stdint.h>
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

// 消息类型宏定义枚举
typedef enum {
    OMINI_MSG_INVALID = 0,

    OMINI_MSG_HEARTBEAT = 1,
    OMINI_MSG_SENSOR_IMU,
    OMINI_MSG_CONTROL_CMD,
    OMINI_MSG_CONFIG_REQ,

    OMINI_MSG_TYPE_MAX
}OminiFlowMsgType_e;

// 固定对齐方式，确保跨平台对齐一致性
// 对齐系数设置为1字节，结构体成员之间不再插入任何填充字节，完全按成员声明的长度紧密排列
#pragma pack(push, 1)

typedef struct {
    uint16_t magic;         // Magic数字: 0x4F46 (ASCII: 'OF') 
    uint8_t  version;       // 协议版本：0x01
    uint8_t  msg_type;      // 消息类型：请见枚举宏定义OminiFlowMsgType_e
    uint32_t seq_num;       // 包序列号：用于丢包检测
    uint64_t timestamp_us;  // 时间戳：微秒级。用于延迟检测
    uint32_t payload_len;   // 负载长度
    uint16_t checksum;      // CRC校验和
}OminiFlowHeader_t;

// 从堆栈中恢复字节对齐配置，避免影响其他结构体导致读写速度减慢甚至崩溃
#pragma push(pop)

#ifdef __cplusplus
}
#endif

#endif // OMINIFLOW_PROTOCOL_H__