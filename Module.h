#pragma once

#include <string>
#include <sstream>
#include <cstdio>

struct Module {
    enum Type {
        NONE,
        DISPENSER,
        MIXER,
        DETECTOR,
        SINK
    };

    int id_;
    Type type_;
    std::string label_;
    int time;

    // for dispenser
    std::string fluid_type_;
    int volume_;

    // for mixer
    int time_;
    int drops_;
    int w, h;

    // for sink
    std::string ink_name_;

    Module(int id, std::string label, int type = NONE) : id_(id), label_(label), type_(type) {}

    std::string to_string() {
        char buf[100];
        sprintf(buf, "%s %s", label_, id_);
        return std::string(buf);
    }
}