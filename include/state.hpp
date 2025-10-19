#pragma once

#include <video.hpp>
#include <renderer/text.hpp>

#include <functional>
#include <memory>
#include <stack>

struct Action {
public:
    std::function<void(std::string info)> perform;
    std::function<void(std::string info)> undo;

    std::string info; // any additional info that may be needed
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

    // std::shared_ptr<Clip> selectedClip = nullptr;
    std::string selectedClipId = "";
    int currentFrame = 0;
    int lastRenderedFrame = -1;
    bool isPlaying = false;

    std::string exportPath;

    ma_engine soundEngine;

    void undo() {
        if (undoStack.empty()) return;

        auto action = undoStack.top();
        action.undo(action.info);
        redoStack.push(action);
        undoStack.pop();
    }
    
    void redo() {
        if (redoStack.empty()) return;

        auto action = redoStack.top();
        action.perform(action.info);
        undoStack.push(action);
        redoStack.pop();
    }

    void deselect() {
        selectedClipId = "";
    }

    void selectClip(std::shared_ptr<Clip> clip) {
        selectedClipId = clip->uID;
    }

    void selectClip(std::string id) {
        selectedClipId = id;
    }

    std::shared_ptr<Clip> getSelectedClip() {
        if (selectedClipId.empty()) return nullptr;

        int trackIdx = video->getClipMap()[selectedClipId];
        return video->videoTracks[trackIdx]->getClip(selectedClipId);
    }

    bool isClipSelected() {
        return !selectedClipId.empty();
    }
};
