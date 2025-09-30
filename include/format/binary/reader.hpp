// stolen from https://github.com/dankmeme01/qunet-cpp/blob/main/include/qunet/buffers/ByteReader.hpp
#pragma once

#include "error.hpp"

#include <span>
#include <string>
#include <vector>
#include <stdint.h>
#include <stddef.h>

#include <Geode/Result.hpp>

namespace qn {

class ByteReader {
public:
    template <typename T = void>
    using Result = geode::Result<T, ByteReaderError>;

    ByteReader(std::span<const uint8_t> data);
    ByteReader(const uint8_t* data, size_t size);
    ByteReader(const std::vector<uint8_t>& data);

    static ByteReader withTwoSpans(std::span<const uint8_t> first, std::span<const uint8_t> second);

    Result<void> readBytes(uint8_t* data, size_t size);
    std::vector<uint8_t> readToEnd();
    Result<void> skip(size_t size);

    Result<uint8_t> readU8();
    Result<uint16_t> readU16();
    Result<uint32_t> readU32();
    Result<uint64_t> readU64();
    Result<bool> readBool();
    Result<int8_t> readI8();
    Result<int16_t> readI16();
    Result<int32_t> readI32();
    Result<int64_t> readI64();
    Result<float> readF32();
    Result<double> readF64();
    Result<float> readFloat();
    Result<double> readDouble();

    Result<int64_t> readVarInt();
    Result<uint64_t> readVarUint();

    Result<std::string> readStringVar();
    Result<std::string> readStringU8();
    Result<std::string> readStringU16();
    Result<std::string> readStringU32();
    // Alias to `readStringU16`
    Result<std::string> readString();
    Result<std::string> readFixedString(size_t len);

    size_t remainingSize() const;
    size_t position() const;
    void setPosition(size_t pos);

private:
    std::span<const uint8_t> m_data;
    std::span<const uint8_t> m_reserve{};
    size_t m_pos = 0;

    ByteReader(std::span<const uint8_t> first, std::span<const uint8_t> second);

    template <typename T>
    Result<T> readBytesAs() {
        T out;
        GEODE_UNWRAP(this->readBytes((uint8_t*)&out, sizeof(out)));
        return Ok(out);
    }
};

}