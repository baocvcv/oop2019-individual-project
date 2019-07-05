#pragma once

#include "Architecture.h"
#include "Solver.h"

#include <string>
#include <vector>
#include "z3++.h"

class OnePassSynth {
public:
    OnePassSynth(std::string filename): filename_(filename), ctx_(), arc_(filename_), solver_(arc_, ctx_) {}
    
    // solve according to the limit set in input file
    bool solve() { return solver_.solve(); }

    // solve under these conditions
    bool solve(int width, int height, int time) { return solver_.solve(width, height, time); }

    // print solution to screen
    void print_solution() { solver_.print_solution(); }

    // print flow diagrm to filename.dot
    void print_flow_diagram(std::string filename) { arc_.print_to_graph(filename); }
    
    // return matrix[time][m][n], -3: empty, -2: mixing, -1: detecting, >=0: droplet ids
    std::vector<std::vector<std::vector<int>>> get_grid() { return solver_.get_grid(); }

    // return sink_dispensers[p] (pos, label)
    // Node {
    //   int type_;
    //   std::string label_;
    //}
    std::vector<Node> get_sink_dispenser_pos() { return solver_.get_sink_dispenser_pos(); }

    // return detectors[x][y] (flag, label)
    std::vector<std::vector<std::pair<bool, std::string>>> get_detector_pos() { return solver_.get_detector_pos(); }

    Solver get_solver() { return solver_; }

private:
    std::string filename_;
    z3::context ctx_;
    Architecture arc_;
    Solver solver_;
};
