#pragma once

#include <vector>
#include <string>


/**
 * @brief The state of the grid(w*h) at a certain point of time
 * 
 */

class GridState {
    struct Cell {
        int id_;
        bool is_occupied_;

        Cell(int id, bool is_occupied=false): id_(id), is_occupied_(is_occupied) {}
    }

    Cell **grid_;
    int w_;
    int h_;

public:
    GridState(int w, int h);
    
    ~GridState();

    int get_id(int x, int y) { return grid_[x][y].id_; }
    bool is_occupied(int x, int y) { return grid_[x][y].is_occupied_; }
}