#pragma once

#include "Architecture.h"
#include "Solver.h"

#include <string>
#include <vector>
#include "z3++.h"

class OnePassSynth {
public:
    OnePassSynth(std::string filename);
    
    // solve according to the limit set in input file
    bool solve();

    // solve under these conditions
    bool solve(int width, int height, int time);
    
    // return matrix[time][m][n]
    std::vector<std::vector<std::vector<int>>> get_grid();

    // return sink[p]
    std::vector<int> get_sink_pos();

    // return dispensers[p] (pos, label)
    std::vector<std::pair<int, std::string>> get_dispenser_pos();

    // return detectors[x][y] (flag, label)
    std::vector<std::vector<std::pair<bool, std::string>>> get_detector_pos();

private:
    Solver solver;
    z3::context ctx;
    Architecture arc;
    std::string filename;
};
