#include <cmath>
#include <utils.hpp>

namespace utils {
    void drawImage(Frame* frame, unsigned char* data, Vector2D size, Vector2D position, double rotation) {
        // x'' = (x' * cos(θ)) - (y' * sin(θ))
        // y'' = (x' * sin(θ)) + (y' * cos(θ))
        // x'' = rotated x
        // y'' = rotated y
        // x' = x - center x
        // y' = y - center y

        double radians = rotation * PI_DIV_180;

        double cosR = cosf(radians);
        double sinR = sinf(radians);
        double centerX = (double)size.x / 2.f;
        double centerY = (double)size.y / 2.f;

        auto forwardMappedPos = [cosR, sinR, centerX, centerY](int x, int y) {
            Vector2D result;

            double xPrime = x - centerX;
            double yPrime = y - centerY;
            result.x = static_cast<int>(std::round((xPrime * cosR) - (yPrime * sinR) + centerX));
            result.y = static_cast<int>(std::round((xPrime * sinR) + (yPrime * cosR) + centerY));

            return result;
        };

        auto inverseMappedPos = [cosR, sinR, centerX, centerY](int x, int y) {
            Vector2D result;

            double xPrime = x - centerX;
            double yPrime = y - centerY;
            result.x = static_cast<int>(std::round((xPrime * cosR) + (yPrime * sinR) + centerX));
            result.y = static_cast<int>(std::round(-(xPrime * sinR) + (yPrime * cosR) + centerY));

            return result;
        };

        // precompute rotated positions for the corners
        // 0, 0
        auto topLeft = forwardMappedPos(0, 0);

        // width, 0
        auto topRight = forwardMappedPos(size.x, 0);

        // 0, height
        auto bottomLeft = forwardMappedPos(0, size.y);

        // width, height
        auto bottomRight = forwardMappedPos(size.x, size.y);

        int minX = std::min({ topLeft.x, topRight.x, bottomLeft.x, bottomRight.x });
        int minY = std::min({ topLeft.y, topRight.y, bottomLeft.y, bottomRight.y });

        int maxX = std::max({ topLeft.x, topRight.x, bottomLeft.x, bottomRight.x });
        int maxY = std::max({ topLeft.y, topRight.y, bottomLeft.y, bottomRight.y });

        // fmt::println("drawImage: size=({}, {})", size.x, size.y);
        for (int dstY = minY; dstY < maxY; dstY++) {
            for (int dstX = minX; dstX < maxX; dstX++) {
                int frameX = dstX + position.x;
                int frameY = dstY + position.y;

                auto res = inverseMappedPos(dstX, dstY);
                res.x = std::clamp(res.x, 0, size.x - 1);
                res.y = std::clamp(res.y, 0, size.y - 1);
                
                if (res.x < 0 || res.x >= size.x) continue;
                if (res.y < 0 || res.y >= size.y) continue;
                
                if (frameX < 0 || frameX >= frame->width) continue;
                if (frameY < 0 || frameY >= frame->height) continue;
                
                int srcLoc = (res.y * size.x + res.x) * 3;
                int dstLoc = (frameY * frame->width + frameX) * 4;
                fmt::println("unimplemented");
                // uint8_t* dst = &frame->imageData[dstLoc];
                // // if (!data[srcLoc] || !data[srcLoc + 1] || !data[srcLoc + 2]) continue;

                // // fmt::println("{}, {}, {}", data[srcLoc], data[srcLoc + 1], data[srcLoc + 2]);
                // // if (srcLoc + 2 >= size.x * size.y * 3) {
                //     // fmt::println("OOB: res=({}, {}), srcLoc={}", res.x, res.y, srcLoc);
                //     // continue;
                // // }
                // dst[0] = data[srcLoc];
                // dst[1] = data[srcLoc + 1];
                // dst[2] = data[srcLoc + 2];
                // dst[3] = 255;
            }
        }
    }
}
