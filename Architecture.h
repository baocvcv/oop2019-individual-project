#pragma once

#include <string>
#include <vector>

class Architecture {
private:
    std::string label_;
    std::vector<std::vector<int>> forward_edges_;
    std::vector<std::vector<int>> backward_edges_;
    std::vector<Node> modules_;
    int width_limit_;
    int height_limit_;
    int time_limit_;

    int sink_num_;
    int dispenser_num_;

public:
    Architecture();
    void build_from_file(const std::string &filename);
    void print_to_graph(const std::string &filename);

}