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
    // std::string selectedClipId = "";
    std::vector<std::string> selectedClips;
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

    // deselects all
    void deselect() {
        selectedClips.clear();
    }

    // deselects a specific ID
    void deselect(std::string id) {
        selectedClips.erase(
            std::remove(selectedClips.begin(), selectedClips.end(), id),
            selectedClips.end()
        );
    }

    void selectClip(std::shared_ptr<Clip> clip) {
        selectClip(clip->uID);
    }
    
    void selectClip(std::string id) {
        if (isClipSelected(id)) return;
        selectedClips.push_back(id);
    }

    std::unordered_map<std::string, std::shared_ptr<Clip>> getSelectedClips() {
        if (selectedClips.empty()) return {};

        std::unordered_map<std::string, std::shared_ptr<Clip>> result;
        for (auto clipID : selectedClips) {
            int trackIdx = video->getClipMap()[clipID];
            std::shared_ptr<Clip> clip;
            if (trackIdx < 0) {
                clip = video->audioTracks[-(trackIdx + 1)]->getClip(clipID);
            } else {
                clip = video->videoTracks[trackIdx]->getClip(clipID);
            }
            result[clip->uID] = clip;
        }
        return result;
    }

    bool areClipsSelected() {
        return !selectedClips.empty() && !getSelectedClips().empty();
    }

    bool areClipsLinked() {
        if (!areClipsSelected()) return false;
        if (selectedClips.size() <= 1) return false;

        bool res = true;
        auto clips = getSelectedClips();
        auto firstClip = clips.begin()->second;
        if (firstClip->linkedClips.size() == 0) return false;
        for (auto [_, clip] : clips) {
            if (firstClip == clip) continue;

            if (firstClip->linkedClips != clip->linkedClips) {
                res = false;
                break;
            }
        }
        return res;
    }

    bool isClipSelected(std::string id) {
        return std::find(selectedClips.begin(), selectedClips.end(), id) != selectedClips.end();
    }

    bool isClipSelected(std::shared_ptr<Clip> clip) {
        return isClipSelected(clip->uID);
    }
};
