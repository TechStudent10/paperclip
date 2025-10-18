#pragma once

#include <video.hpp>
#include <renderer/text.hpp>

#include <functional>
#include <memory>
#include <stack>

struct Action {
public:
    const std::function<void()> perform;
    const std::function<void()> undo;
};

class State {
private:
    State() {};
public:
    static State& get() {
        static State instance;
        return instance;
    }

    std::shared_ptr<Video> video;
    std::shared_ptr<TextRenderer> textRenderer;

    std::stack<Action> undoStack;
    std::stack<Action> redoStack;

    std::shared_ptr<Clip> selectedClip = nullptr;
    int currentFrame = 0;
    int lastRenderedFrame = -1;
    bool isPlaying = false;

    std::string exportPath;

    ma_engine soundEngine;

    void undo() {
        if (undoStack.empty()) return;

        auto action = undoStack.top();
        action.undo();
        redoStack.push(action);
        undoStack.pop();
    }
    
    void redo() {
        if (redoStack.empty()) return;

        auto action = redoStack.top();
        action.perform();
        undoStack.push(action);
        redoStack.pop();
    }

    void deselect() {
        selectedClip = nullptr;
    }
};
