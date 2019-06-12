#pragma once

#include <string>
#include <sstream>
#include <cstdio>

enum Type {
    NONE,
    DISPENSER,
    MIXER,
    DETECTOR,
    SINK
};
struct Module {
    int id_;
    Type type_;
    std::string label_;
    int time_;

    // for dispenser
    std::string fluid_type_;
    int volume_;

    // for mixer
    int drops_;
    int w, h;

    // for sink
    std::string sink_name_;

    //Module(int id, std::string label, int type = NONE) : id_(id), label_(label), type_(type) {}

    std::string to_string() {
        char buf[100];
        sprintf(buf, "%s %d", label_.c_str(), id_);
        return std::string(buf);
    }
};