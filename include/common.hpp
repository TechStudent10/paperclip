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

    Vector2D operator+(const int& other) {
        return { x + other, y + other };
    }

    Vector2D operator-(const int& other) {
        return { x - other, y - other };
    }

    Vector2D operator*(const int& other) {
        return { x * other, y * other };
    }

    Vector2D operator/(const int& other) {
        return { x / other, y / other };
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

    Vector2DF operator+(const Vector2DF& other) {
        return { x + other.x, y + other.y };
    }

    Vector2DF operator-(const Vector2DF& other) {
        return { x - other.x, y - other.y };
    }

    Vector2DF operator*(const Vector2DF& other) {
        return { x * other.x, y * other.y };
    }

    Vector2DF operator/(const Vector2DF& other) {
        return { x / other.x, y / other.y };
    }

    Vector2DF operator+(const float& other) {
        return { x + other, y + other };
    }

    Vector2DF operator-(const float& other) {
        return { x - other, y - other };
    }

    Vector2DF operator*(const float& other) {
        return { x * other, y * other };
    }

    Vector2DF operator/(const float& other) {
        return { x / other, y / other };
    }

    operator Vector2D() {
        return { .x = (int)x, .y = (int)y };
    }
};

struct Vector1D {
    int number;

    JSON_METHODS(Vector1D);
};

struct Transform {
    Vector2D position;
    // 0.5, 0.5 = center
    // 0, 0 = top left
    // 0, 1 = top right
    // 1, 0 = bottom left
    // 1, 1 = bottom right
    Vector2DF anchorPoint = { 0.5, 0.5 };
    float rotation;
    float pitch;
    float roll;

    JSON_METHODS(Transform)
};

struct Dimensions {
    Vector2D size;
    Transform transform;

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
