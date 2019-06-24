#pragma once

#include "z3++.h"
#include <vector>
#include <string>


/**
 * @brief The state of the grid(w*h) at a certain point of time
 * 
 */

class GridState {
    std::vector<z3::expr> **grid_;
    int w_;
    int h_;

public:
    GridState(int w, int h);
    
    ~GridState();

    int get_id(int x, int y) { return grid_[x][y].id_; }
    bool is_occupied(int x, int y) { return grid_[x][y].is_occupied_; }
}