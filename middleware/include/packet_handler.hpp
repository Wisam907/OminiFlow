#pragma once

#include "protocol.h"
#include <algorithm>
#include <cstdint>
#include <span>
#include <memory>
#include <iostream>
#include <vector>

namespace ominiflow {

/**
 * @brief DataPacket 包装器
 * 核心设计：不持有数据所有权，或通过std::unique_ptr管理生命周期
 * 利用std::span提供切片视图，避免内存拷贝
 */
class DataPacket {
public:
    // 构造函数：直接接收原始字节流
    explicit DataPacket(std::vector<uint8_t>&& raw_data)
        : data_(std::move(raw_data)) {}
    
    // 获取协议头视图（零拷贝）
    const OminiFlowHeader_t* header() const {
        if (data_.size() < sizeof(OminiFlowHeader_t))
            return nullptr;
        return reinterpret_cast<const OminiFlowHeader_t*>(data_.data());
    }

    // 获取负载视图（零拷贝）
    // 使用非拥有型视图std::span返回一个边界安全、零开销抽象的数据切片
    std::span<const uint8_t> payload() const {
        if (data_.size() < sizeof(OminiFlowHeader_t))
            return {};
        return {data_.data() + sizeof(OminiFlowHeader_t), data_.size() - sizeof(OminiFlowHeader_t)};
    }

    // 校验魔数
    bool isValid() const {
        auto h = header();
        return h && (h->magic == OMINI_PROTO_HEADER_MAGIC);
    }
private:
    std::vector<uint8_t> data_;
};

}   // namespace ominiflow