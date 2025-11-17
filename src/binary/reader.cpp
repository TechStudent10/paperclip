// stolen from https://github.com/dankmeme01/qunet-cpp/blob/main/src/buffers/ByteReader.cpp

#include <binary/reader.hpp>
#include <cstring>

template <typename T = void>
using Result = qn::ByteReader::Result<T>;

namespace qn {

ByteReader::ByteReader(std::span<const uint8_t> data) : m_data(data), m_reserve({}) {}
ByteReader::ByteReader(const uint8_t* data, size_t size) : m_data(data, data + size), m_reserve({}) {}
ByteReader::ByteReader(const std::vector<uint8_t>& data) : m_data(data.begin(), data.end()), m_reserve({}) {}

ByteReader ByteReader::withTwoSpans(std::span<const uint8_t> first, std::span<const uint8_t> second) {
    return ByteReader(first, second);
}

ByteReader::ByteReader(std::span<const uint8_t> first, std::span<const uint8_t> second) : m_data(first), m_reserve(second) {}

Result<void> ByteReader::readBytes(uint8_t* data, size_t size) {
    if (size > this->remainingSize()) {
        return Err(ByteReaderError::OutOfBoundsRead);
    }

    // read from 1st span if applicable
    if (m_pos < m_data.size()) {
        size_t toRead = std::min(size, m_data.size() - m_pos);
        std::memcpy(data, m_data.data() + m_pos, toRead);
        m_pos += toRead;
        data += toRead;
        size -= toRead;
    }

    // read from 2nd span if needed
    if (size == 0) {
        return Ok();
    }

    // position relative to the reserve span
    size_t relPos = m_pos - m_data.size();

    QN_DEBUG_ASSERT(relPos + size <= m_reserve.size());

    std::memcpy(data, m_reserve.data() + relPos, size);
    m_pos += size;

    return Ok();
}

std::vector<uint8_t> ByteReader::readToEnd() {
    std::vector<uint8_t> out;
    out.resize(this->remainingSize());
    this->readBytes(out.data(), out.size()).unwrap();
    return out;
}

Result<void> ByteReader::skip(size_t size) {
    if (m_pos + size > m_data.size())  {
        return Err(ByteReaderError::OutOfBoundsRead);
    }

    m_pos += size;

    return Ok();
}

Result<uint8_t> ByteReader::readU8() {
    return this->readBytesAs<uint8_t>();
}

Result<uint16_t> ByteReader::readU16() {
    return this->readBytesAs<uint16_t>();
}

Result<uint32_t> ByteReader::readU32() {
    return this->readBytesAs<uint32_t>();
}

Result<uint64_t> ByteReader::readU64() {
    return this->readBytesAs<uint64_t>();
}

Result<bool> ByteReader::readBool() {
    return this->readU8().map([](uint8_t v) { return v != 0; });
}

Result<int8_t> ByteReader::readI8() {
    return this->readBytesAs<int8_t>();
}

Result<int16_t> ByteReader::readI16() {
    return this->readBytesAs<int16_t>();
}

Result<int32_t> ByteReader::readI32() {
    return this->readBytesAs<int32_t>();
}

Result<int64_t> ByteReader::readI64() {
    return this->readBytesAs<int64_t>();
}

Result<float> ByteReader::readF32() {
    return this->readBytesAs<float>();
}

Result<double> ByteReader::readF64() {
    return this->readBytesAs<double>();
}

Result<float> ByteReader::readFloat() {
    return this->readF32();
}

Result<double> ByteReader::readDouble() {
    return this->readF64();
}

Result<int64_t> ByteReader::readVarInt() {
    int64_t value = 0;
    int shift = 0;
    int size = 64;
    uint8_t byte;

    while (true) {
        byte = GEODE_UNWRAP(this->readU8());

        if (shift == 64 && byte != 0 && byte != 1) {
            return Err(ByteReaderError::VarintOverflow);
        }

        value |= (uint64_t)(byte & 0x7f) << shift;
        shift += 7;

        if ((byte & 0x80) == 0) {
            break;
        }
    }

    if (shift < size && (byte & 0x40) != 0) {
        value |= -1 << shift;
    }

    return Ok(value);
}

Result<uint64_t> ByteReader::readVarUint() {
    uint64_t value = 0;
    size_t shift = 0;

    while (true) {
        uint8_t byte = GEODE_UNWRAP(this->readU8());

        if (shift == 63 && byte > 1) {
            return Err(ByteReaderError::VarintOverflow);
        }

        value |= (static_cast<uint64_t>(byte & 0x7f) << shift);

        if (!(byte & 0x80)) {
            return Ok(value);
        }

        shift += 7;
    }

    return Err(ByteReaderError::VarintOverflow);
}

Result<std::string> ByteReader::readStringVar() {
    auto len = GEODE_UNWRAP(this->readVarUint());
    return this->readFixedString(len);
}

Result<std::string> ByteReader::readStringU8() {
    auto len = GEODE_UNWRAP(this->readU8());
    return this->readFixedString(len);
}

Result<std::string> ByteReader::readStringU16() {
    auto len = GEODE_UNWRAP(this->readU16());
    return this->readFixedString(len);
}

Result<std::string> ByteReader::readStringU32() {
    auto len = GEODE_UNWRAP(this->readU32());
    return this->readFixedString(len);
}

Result<std::string> ByteReader::readString() {
    return this->readStringU16();
}

Result<std::string> ByteReader::readFixedString(size_t len) {
    if (len > 1024 * 1024) {
        return Err(ByteReaderError::StringTooLong);
    }

    std::string out(len, '\0');
    GEODE_UNWRAP(this->readBytes((uint8_t*)out.data(), len));

    return Ok(std::move(out));
}

size_t ByteReader::remainingSize() const {
    return m_data.size() + m_reserve.size() - m_pos;
}

size_t ByteReader::position() const {
    return m_pos;
}

void ByteReader::setPosition(size_t pos) {
    m_pos = pos;
}

}
