// stolen from https://github.com/dankmeme01/qunet-cpp/blob/main/src/buffers/HeapByteWriter.cpp

#include <binary/writer.hpp>
#include <cstring>

template <typename T = void>
using Result = qn::HeapByteWriter::Result<T>;

namespace qn {

HeapByteWriter::HeapByteWriter(const std::vector<uint8_t>& data) : m_buffer(data) {}

HeapByteWriter::HeapByteWriter(std::vector<uint8_t>&& data) : m_buffer(std::move(data)) {}

HeapByteWriter::HeapByteWriter(std::span<const uint8_t> data) : m_buffer(data.begin(), data.end()) {}

HeapByteWriter::HeapByteWriter(const uint8_t* data, size_t size) : m_buffer(data, data + size) {}

void HeapByteWriter::writeBytes(const uint8_t* data, size_t size) {
    this->writeBytes(std::span{data, data + size});
}

void HeapByteWriter::writeBytes(std::span<const uint8_t> data) {
    size_t end = m_pos + data.size();

    if (end > m_buffer.size()) {
        m_buffer.resize(end);
    }

    std::memcpy(m_buffer.data() + m_pos, data.data(), data.size());
    m_pos = end;
}

void HeapByteWriter::writeBytes(const std::vector<uint8_t>& data) {
    this->writeBytes(std::span{data.begin(), data.end()});
}

void HeapByteWriter::writeZeroes(size_t size) {
    size_t end = m_pos + size;

    if (end > m_buffer.size()) {
        m_buffer.resize(end);
    }

    std::memset(m_buffer.data() + m_pos, 0, size);
    m_pos = end;
}

void HeapByteWriter::writeU8(uint8_t value) {
    this->writeBytes((const uint8_t*)&value, sizeof(value));
}

void HeapByteWriter::writeU16(uint16_t value) {
    this->writeBytes((const uint8_t*)&value, sizeof(value));
}

void HeapByteWriter::writeU32(uint32_t value) {
    this->writeBytes((const uint8_t*)&value, sizeof(value));
}

void HeapByteWriter::writeU64(uint64_t value) {
    this->writeBytes((const uint8_t*)&value, sizeof(value));
}

void HeapByteWriter::writeBool(bool value) {
    this->writeU8(value ? 1 : 0);
}

void HeapByteWriter::writeI8(int8_t value) {
    this->writeBytes((const uint8_t*)&value, sizeof(value));
}

void HeapByteWriter::writeI16(int16_t value) {
    this->writeBytes((const uint8_t*)&value, sizeof(value));
}

void HeapByteWriter::writeI32(int32_t value) {
    this->writeBytes((const uint8_t*)&value, sizeof(value));
}

void HeapByteWriter::writeI64(int64_t value) {
    this->writeBytes((const uint8_t*)&value, sizeof(value));
}

void HeapByteWriter::writeF32(float value) {
    this->writeBytes((const uint8_t*)&value, sizeof(value));
}

void HeapByteWriter::writeF64(double value) {
    this->writeBytes((const uint8_t*)&value, sizeof(value));
}

void HeapByteWriter::writeFloat(float value) {
    this->writeBytes((const uint8_t*)&value, sizeof(value));
}

void HeapByteWriter::writeDouble(double value) {
    this->writeBytes((const uint8_t*)&value, sizeof(value));
}

Result<void> HeapByteWriter::writeVarInt(int64_t value) {
    QN_ASSERT(false && "varint encoding not implemented yet");
    return Ok();
}

Result<void> HeapByteWriter::writeVarUint(uint64_t value) {
    size_t written = 0;

    while (true) {
        uint8_t byte = value & 0x7f;
        value >>= 7;

        if (value != 0) {
            // set continuation bit
            byte |= 0x80;
        }

        this->writeU8(byte);
        written++;

        if (value == 0) {
            break;
        }
    }

    return Ok();
}

Result<void> HeapByteWriter::writeStringVar(std::string_view str) {
    GEODE_UNWRAP(this->writeVarUint(str.size()));
    this->writeBytes((const uint8_t*)str.data(), str.size());

    return Ok();
}

Result<void> HeapByteWriter::writeStringU8(std::string_view str) {
    if (str.size() > std::numeric_limits<uint8_t>::max()) {
        return Err(ByteWriterError::StringTooLong);
    }

    this->writeU8(str.size());
    this->writeBytes((const uint8_t*)str.data(), str.size());

    return Ok();
}

Result<void> HeapByteWriter::writeStringU16(std::string_view str) {
    if (str.size() > std::numeric_limits<uint16_t>::max()) {
        return Err(ByteWriterError::StringTooLong);
    }

    this->writeU16(str.size());
    this->writeBytes((const uint8_t*)str.data(), str.size());

    return Ok();
}

Result<void> HeapByteWriter::writeStringU32(std::string_view str) {
    if (str.size() > std::numeric_limits<uint32_t>::max()) {
        return Err(ByteWriterError::StringTooLong);
    }

    this->writeU32(str.size());
    this->writeBytes((const uint8_t*)str.data(), str.size());

    return Ok();
}

std::vector<uint8_t> HeapByteWriter::toVector() const {
    return m_buffer;
}

std::vector<uint8_t> HeapByteWriter::intoVector() && {
    std::vector<uint8_t> result = std::move(m_buffer);
    m_buffer.clear();
    m_pos = 0;
    return result;
}

std::span<const uint8_t> HeapByteWriter::written() const {
    return std::span{m_buffer.begin(), m_buffer.begin() + m_pos};
}

Result<void> HeapByteWriter::setPosition(size_t pos) {
    if (m_pos > m_buffer.size()) {
        return Err(ByteWriterError::OutOfBoundsWrite);
    }

    m_pos = pos;
    return Ok();
}

size_t HeapByteWriter::position() const {
    return m_pos;
}

}