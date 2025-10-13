#include <binary/error.hpp>

namespace qn {

std::string_view ByteReaderError::message() const {
    using enum Code;

    switch (m_code) {
        case OutOfBoundsRead: return "ByteReader out of bounds read";
        case VarintOverflow: return "Tried reading a varint that cannot fit into a 64-bit integer";
        case StringTooLong: return "Tried decoding a string longer than the maximum permitted size";
    }
}

std::string_view ByteWriterError::message() const {
    using enum Code;

    switch (m_code) {
        case OutOfBoundsWrite: return "ByteWriter out of bounds write";
        case StringTooLong: return "Tried encoding a string longer than the maximum permitted size";
    }
}

}