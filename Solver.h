#pragma once

#include "z3++.h"
#include "Architecture.h"
#include <vector>
#include <string>

class Solver {
    // numbering of ids: 
    //   [0, No. of modules) is allocated to modules (mixing, detecting...)
    //   [No. of modules, No. of modules * 2] is allocated to droplets (each operation could produce at most one droplet)
    // c^t_(x,y,id)
    std::vector<std::vector<std::vector<std::vector<z3::expr>>>> c_;
    // dectector_(x,y,l)
    std::vector<std::vector<std::vector<z3::expr>>> detector_;
    // detecting^t_(i)
    std::vector<std::vector<z3::expr>> detecting_;
    // dispenser_(p,l)
    std::vector<std::vector<z3::expr>> dispenser_;
    // sink(p)
    std::vector<z3::expr> sink_;

    z3::optimize solver_;
    z3::expr no_of_actions_;
    z3::optimize::handle optimize_handle_;
    
    int width_limit_;
    int height_limit_;
    int time_limit_;

    int width_cur_;
    int height_cur_;
    int perimeter_cur_; // = (width_cur_ + height_cur_) * 2
    int time_cur_;

    const Architecture& arch_;
    z3::context& ctx_;

    void init(int width, int height, int time);

    void add_consistency_constraints();
    void add_placement_constraints();
    void add_movement_constraints();
    void add_operations();
    void add_fluidic_constraints();

    bool is_point_inbound(int x, int y) { return (x >= 0) && (x < width_cur_) && (y >= 0) && (y < height_cur_); }

public:
    Solver(const Architecture& arch, z3::context& c);

    bool solve();

    z3::optimize& get_solver() { return solver_; }
    int get_no_of_actions() { return no_of_actions_; }
    void print(const z3::model& m);

};