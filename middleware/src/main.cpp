#include <cstdint>
#include <iostream>
#include <vector>
#include "packet_handler.hpp"
#include "messages.pb.h"    // 自动生成的Protobuf头文件

int main() {
    std::cout << "--- OmniFlow Middleware Starting (C++20) ---" << std::endl;

    // 模拟从网络接收的原始字节流
    std::vector<uint8_t> buffer(sizeof(OminiFlowHeader_t) + 128);

    // 构造协议Header
    OminiFlowHeader_t* h = reinterpret_cast<OminiFlowHeader_t*>(buffer.data());
    h->magic = OMINI_PROTO_HEADER_MAGIC;
    h->msg_type = OMINI_MSG_SENSOR_IMU;
    h->payload_len = 0; // 不携带负载

    // 调用DataPacket解包
    ominiflow::DataPacket packet(std::move(buffer));

    if (packet.isValid()) {
        std::cout << "Packet received! Type: " << static_cast<int>(packet.header()->msg_type) << std::endl;
        std::cout << "Header Size: " << sizeof(OminiFlowHeader_t) << " bytes." << std::endl;
    } else {
        std::cerr << "Invalid Magic Number!" << std::endl;
    }

    // 验证Protobuf可用性
    ominiflow_proto::ControlCommand cmd;
    cmd.set_sample_rate_hz(100);
    std::cout << "Protobuf Test - Target Rate: " << cmd.sample_rate_hz() << "HZ" << std::endl;

    return 0;
}