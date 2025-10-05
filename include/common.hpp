#pragma once

#include <matjson.hpp>
#include <matjson/std.hpp>
#include <matjson/reflect.hpp>

#include <format/binary/reader.hpp>
#include <format/binary/writer.hpp>

#define JSON_METHODS(className) public: \
    std::string toString() { \
        matjson::Value res = *this; \
        return res.dump(); \
    } \
     \
    static className fromString(std::string string) { \
        auto temp = matjson::parse(string); \
        if (temp.isErr()) { \
            return {}; \
        } \
        auto res = temp.unwrap().as<className>(); \
        if (res.isErr()) { \
            return {}; \
        } \
        return res.unwrap(); \
    }

struct Vector2D {
    int x;
    int y;

    JSON_METHODS(Vector2D)

    void write(qn::HeapByteWriter& writer) {
        writer.writeI16(x);
        writer.writeI16(y);
    }

    void read(qn::ByteReader& reader) {
        x = reader.readI16().unwrapOr(0);
        y = reader.readI16().unwrapOr(0);
    }
};

struct Vector1D {
    int number;

    JSON_METHODS(Vector1D);
};

struct Dimensions {
    Vector2D pos;
    Vector2D size;
    
    JSON_METHODS(Dimensions)
};

struct RGBAColor {
    int r; int g; int b; int a = 255;

    JSON_METHODS(RGBAColor)
};

struct DropdownOptions {
    std::vector<std::string> options;

    JSON_METHODS(DropdownOptions)
};