#pragma once

#include "z3++.h"
#include "Architecture.h"
#include <vector>
#include <string>


/**
 * @brief The state of the grid(w*h) at a certain point of time
 * 
 */

class GridState {
    std::vector<std::vector<std::vector<z3::expr>>> grid_;
    int w_;
    int h_;
    int n_;

public:
    GridState(int w, int h, int n): w_(w), h_(h), n_(n) {
        grid_.resize(w_);
        for(int i = 0; i < w_; i++){
            grid_[i].resize(h_);
            for(int j = 0; j < h_; j++){
                grid_[i][j].resize(n_);
                for(int k = 0; k < n_; k++){
                    grid_[i][j][k]
                }
            }
        }
    }
    
    ~GridState();
};

class Solver {
    // c_(x,y,i,t)
    std::vector<std::vector<std::vector<std::vector<z3::expr>>>> c_;
    // dectector_(x,y,l)
    std::vector<std::vector<std::vector<z3::expr>>> dectector_;
    // dispenser_(p,l)
    std::vector<std::vector<z3::expr>> dispenser_;
    // sink(p)
    std::vector<z3::expr> sink_;

    z3::optimize solver_;
    z3::expr num_droplets_;
    z3::optimize::handle optimize_handle_;
    
    int width_limit_;
    int height_limit_;
    int time_limit_;

    const Architecture& arch_;
    z3::context& c;

    void add_consistency_constraints();
    void add_placement_constraints();
    void add_movement_constraints();
    void add_fluidic_consdtraints();

public:
    Solver(const Architecture& arch, z3::context& c);

    z3::optimize& get_solver() { return solver_; }
    int get_num_droplets() { return num_droplets_; }
    void print(const z3::model& m);

};