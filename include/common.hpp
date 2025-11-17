#pragma once

#include <matjson.hpp>
#include <matjson/std.hpp>
#include <matjson/reflect.hpp>

#include <binary/reader.hpp>
#include <binary/writer.hpp>

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

public:
    void write(qn::HeapByteWriter& writer) {
        writer.writeI16(x);
        writer.writeI16(y);
    }

    void read(qn::ByteReader& reader) {
        x = reader.readI16().unwrapOr(0);
        y = reader.readI16().unwrapOr(0);
    }

    Vector2D operator+(const Vector2D& other) {
        return { x + other.x, y + other.y };
    }

    Vector2D operator-(const Vector2D& other) {
        return { x - other.x, y - other.y };
    }

    Vector2D operator*(const Vector2D& other) {
        return { x * other.x, y * other.y };
    }

    Vector2D operator/(const Vector2D& other) {
        return { x / other.x, y / other.y };
    }
};

struct Vector2DF {
    float x;
    float y;

    JSON_METHODS(Vector2DF)

    void write(qn::HeapByteWriter& writer) {
        writer.writeF32(x);
        writer.writeF32(y);
    }

    void read(qn::ByteReader& reader) {
        x = reader.readF32().unwrapOr(0);
        y = reader.readF32().unwrapOr(0);
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