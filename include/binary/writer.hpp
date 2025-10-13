#pragma once

#include "error.hpp"

#include <vector>
#include <span>
#include <string_view>
#include <stdint.h>

namespace qn {

class HeapByteWriter {
public:
    template <typename T = void>
    using Result = geode::Result<T, ByteWriterError>;

    HeapByteWriter() = default;
    ~HeapByteWriter() = default;

    HeapByteWriter(const std::vector<uint8_t>& data);
    HeapByteWriter(std::vector<uint8_t>&& data);
    HeapByteWriter(std::span<const uint8_t> data);
    HeapByteWriter(const uint8_t* data, size_t size);

    HeapByteWriter(const HeapByteWriter& other) = default;
    HeapByteWriter& operator=(const HeapByteWriter& other) = default;
    HeapByteWriter(HeapByteWriter&& other) noexcept = default;
    HeapByteWriter& operator=(HeapByteWriter&& other) noexcept = default;

    void writeBytes(const uint8_t* data, size_t size);
    void writeBytes(std::span<const uint8_t> data);
    void writeBytes(const std::vector<uint8_t>& data);
    void writeZeroes(size_t size);

    void writeU8(uint8_t value);
    void writeU16(uint16_t value);
    void writeU32(uint32_t value);
    void writeU64(uint64_t value);
    void writeBool(bool value);
    void writeI8(int8_t value);
    void writeI16(int16_t value);
    void writeI32(int32_t value);
    void writeI64(int64_t value);
    void writeF32(float value);
    void writeF64(double value);
    void writeFloat(float value);
    void writeDouble(double value);

    Result<void> writeVarInt(int64_t value);
    Result<void> writeVarUint(uint64_t value);

    Result<void> writeStringVar(std::string_view str);
    Result<void> writeStringU8(std::string_view str);
    Result<void> writeStringU16(std::string_view str);
    Result<void> writeStringU32(std::string_view str);

    std::vector<uint8_t> toVector() const;
    std::vector<uint8_t> intoVector() &&;
    std::span<const uint8_t> written() const;

    Result<void> setPosition(size_t pos);
    size_t position() const;

    Result<void> performAt(size_t pos, auto&& func) {
        if (pos > m_buffer.size()) {
            return Err(ByteWriterError::OutOfBoundsWrite);
        }

        size_t oldPos = m_pos;
        m_pos = pos;
        func(*this);
        m_pos = oldPos;
        return Ok();
    }

private:
    std::vector<uint8_t> m_buffer;
    size_t m_pos = 0;
};

}