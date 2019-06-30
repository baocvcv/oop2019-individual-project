#pragma once

#include "Module.h"
#include <string>
#include <vector>
#include <map>

class Architecture {
public:
    std::string label_;
    std::vector<std::pair<int, int> > edges_;
    std::vector<std::vector<int> > forward_edges_;
    std::vector<std::vector<int> > backward_edges_;
    std::map<std::string, Module> modules_;
    std::vector<Module> nodes_;

    int width_limit_;
    int height_limit_;
    int time_limit_;

    int num_sink_;
    int num_dispenser_;
    int num_mixer_;
    int num_detector_;

    // read in the file and subtract id by 1 to make it start from 0
    Architecture();
    void build_from_file(const std::string &filename);
    void print_to_graph(const std::string &filename);

};