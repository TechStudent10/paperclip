#pragma once

#include <fmt/base.h>
class Action {
public:
    virtual void perform() {};
    virtual void undo() {
        fmt::println("you shouldnt be here");
    };
};
