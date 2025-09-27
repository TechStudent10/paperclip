#pragma once

#include <video.hpp>

class State {
private:
    State() {};
public:
    static State& get() {
        static State instance;
        return instance;
    }

    Video* video;
    std::shared_ptr<TextRenderer> textRenderer;

    std::shared_ptr<Clip> draggingClip = nullptr;
    int currentFrame = 0;
    bool isPlaying = false;

    std::string exportPath;

    ma_engine soundEngine;
};
