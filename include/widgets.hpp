#pragma once

#include "imgui_internal.h"
#include <functional>
#include <imgui.h>

#include <video.hpp>
#include <vector>
#include <string>

struct TimelineClip {
    int id;
    std::string name;
    float startTime;     // in seconds
    float duration;       // in seconds
    ImU32 color;
    bool selected;

    std::shared_ptr<Clip> clip;
    
    TimelineClip(int _id, const std::string& _name, float _start, float _duration, std::shared_ptr<Clip> clip, ImU32 _color = IM_COL32(100, 150, 200, 255));
    
    float GetEndTime() const;
};

struct TimelineTrack {
    int id;
    std::string name;
    std::vector<TimelineClip> clips;
    bool muted;
    bool locked;
    float height;
    
    TimelineTrack(int _id, const std::string& _name, float _height = 60.0f);
};

enum class TrackType {
    Audio,
    Video,
    // Disco
};

class VideoTimeline {
private:
    // std::vector<TimelineTrack> tracks;
    float playheadTime;
    float zoomFactor;
    float scrollX;
    float timelineLength;  // total timeline duration in seconds
    float pixelsPerSecond;
    
    // UI state
    ImVec2 dragOffset;
    bool isDragging;
    bool isScrubbing;
    
    int draggingTrackIdx;

    bool isMovingBetweenTracks;
    int originalTrackId;
    ImVec2 dragStartPos;

    // Constants
    static constexpr float TRACK_HEADER_WIDTH = 100.0f;
    static constexpr float RULER_HEIGHT = 30.0f;
    static constexpr float MIN_ZOOM = 10.0f;   // pixels per second
    static constexpr float MAX_ZOOM = 200.0f;
    static constexpr float RESIZE_HANDLE_WIDTH = 10.0f;

    enum ResizeMode {
        RESIZE_NONE,
        RESIZE_LEFT,    // Resizing start time
        RESIZE_RIGHT    // Resizing duration
    };
    
    ResizeMode resizeMode;

    std::shared_ptr<Clip> resizingClip;
    float resizeOriginalStart;
    float resizeOriginalDuration;
    int resizingTrackIdx;

    ImVec2 canvasSize;
    ImVec2 canvasPos;

    int getTrackIndexAtPosition(float y_pos, const ImVec2& canvas_pos) const;

    // Private rendering methods
    void drawRuler(ImDrawList* drawList, const ImVec2& canvasPos, const ImVec2& canvasSize);
    void drawTrack(ImDrawList* drawList, const ImVec2& canvasPos, const ImVec2& canvasSize, size_t trackIndex, size_t trackListSize, TrackType type);
    void drawClip(ImDrawList* drawList, const TimelineClip& clip, const ImVec2& trackPos, const ImVec2& trackSize, TrackType type);
    void drawPlayhead(ImDrawList* drawList, const ImVec2& canvasPos, const ImVec2& canvasSize);
    void handleInteractions(const ImVec2& canvasPos, const ImVec2& canvasSize, bool isHovered, bool isActive);
    ResizeMode GetResizeMode(const ImVec2& mouse_pos, std::shared_ptr<Clip> clip, const ImVec2& track_pos, const ImVec2& track_size, float clickTime);

public:
    VideoTimeline();

    float isPlacingClip = false;
    ImVec2 hoverPos;

    float trackScrollY = 0.f;
    int selectedTrackIdx = 0;
    int hoveredTrackIdx = 0;
    TrackType selectedTrackType;
    TrackType hoveredTrackType;
    TrackType ogTrackType;
    TrackType placeType;

    std::function<void(int frame, int trackIdx)> placeCb = nullptr;
    
    // Track management
    void addTrack(const std::string& name);
    
    // Clip management
    void addClip(int track_id, const std::string& name, float start_time, float duration, ImU32 color = IM_COL32(100, 150, 200, 255));
    
    // Playhead control
    void setPlayheadTime(float time);
    float getPlayheadTime() const;
    
    // Zoom control
    void setZoom(float zoom);
    
    // Main rendering function
    void render();

    void scrollToTime(float time, bool center = true);
};

// https://github.com/ocornut/imgui/issues/1901
// by zfedoran
namespace ImGui {
    
    inline bool BufferingBar(const char* label, float value,  const ImVec2& size_arg, const ImU32& bg_col, const ImU32& fg_col) {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;
        
        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);

        ImVec2 pos = window->DC.CursorPos;
        ImVec2 size = size_arg;
        size.x -= style.FramePadding.x * 2;
        
        const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ItemSize(bb, style.FramePadding.y);
        if (!ItemAdd(bb, id))
            return false;
        
        // Render
        const float circleStart = size.x * 0.7f;
        const float circleEnd = size.x;
        const float circleWidth = circleEnd - circleStart;
        
        window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart, bb.Max.y), bg_col);
        window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart*value, bb.Max.y), fg_col);
        
        const float t = g.Time;
        const float r = size.y / 2;
        const float speed = 1.5f;
        
        const float a = speed*0;
        const float b = speed*0.333f;
        const float c = speed*0.666f;
        
        const float o1 = (circleWidth+r) * (t+a - speed * (int)((t+a) / speed)) / speed;
        const float o2 = (circleWidth+r) * (t+b - speed * (int)((t+b) / speed)) / speed;
        const float o3 = (circleWidth+r) * (t+c - speed * (int)((t+c) / speed)) / speed;
        
        window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o1, bb.Min.y + r), r, bg_col);
        window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o2, bb.Min.y + r), r, bg_col);
        window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o3, bb.Min.y + r), r, bg_col);

        return true;
    }

    inline bool Spinner(const char* label, float radius, int thickness, const ImU32& color) {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;
        
        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);
        
        ImVec2 pos = window->DC.CursorPos;
        ImVec2 size((radius )*2, (radius + style.FramePadding.y)*2);
        
        const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ItemSize(bb, style.FramePadding.y);
        if (!ItemAdd(bb, id))
            return false;
        
        // Render
        window->DrawList->PathClear();
        
        int num_segments = 30;
        int start = abs(ImSin(g.Time*1.8f)*(num_segments-5));
        
        const float a_min = IM_PI*2.0f * ((float)start) / (float)num_segments;
        const float a_max = IM_PI*2.0f * ((float)num_segments-3) / (float)num_segments;

        const ImVec2 centre = ImVec2(pos.x+radius, pos.y+radius+style.FramePadding.y);
        
        for (int i = 0; i < num_segments; i++) {
            const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
            window->DrawList->PathLineTo(ImVec2(centre.x + ImCos(a+g.Time*8) * radius,
                                                centre.y + ImSin(a+g.Time*8) * radius));
        }

        window->DrawList->PathStroke(color, false, thickness);

        return true;
    }
    
}
