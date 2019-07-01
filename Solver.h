#pragma once

#include "z3++.h"
#include "Architecture.h"

#include <vector>
#include <string>
#include <fstream>
#include <iostream>

struct Node {
    int type_; // 0 - empty, 1 - sink, 2 - dispenser
    std::string label_;
};

class Solver {
private:
    // c^t_(x,y,id)
    // c_ used for droplets only
    // no of droplets = no of edges
    // droplet id = edge id
    std::vector<std::vector<std::vector<std::vector<z3::expr>>>> c_;
    // mixing^t_(x,y,i)
    std::vector<std::vector<std::vector<std::vector<z3::expr>>>> mixing_;
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
    int no_of_modules_;
    int no_of_nodes_;
    int no_of_edges_;

    int width_cur_;
    int height_cur_;
    int perimeter_cur_; // = (width_cur_ + height_cur_) * 2
    int time_cur_;
    z3::check_result result_;
    z3::model model_;

    Architecture& arch_;
    z3::context& ctx_;

    void init(int width, int height, int time);
    void add_constraints();
    void add_consistency_constraints();
    void add_placement_constraints();
    void add_movement_constraints();
    void add_operations();
    void add_objectives(); 
    void add_fluidic_constraints();
    void add_movement();

    bool is_point_inbound(int x, int y) { return (x >= 0) && (x < width_cur_) && (y >= 0) && (y < height_cur_); }

public:
    Solver(Architecture& arch, z3::context& c);

    bool solve();
    bool solve(int width, int height, int time);

    z3::optimize& get_solver() { return solver_; }
    int get_no_of_actions() { return no_of_actions_; }

    void print_solver(std::ostream& out = std::cout); 
    void print_solution(std::ostream& out = std::cout);
    void save_solver(std::string filename);
    void save_solution(std::string filename);

   // return matrix[time][m][n]
    std::vector<std::vector<std::vector<int>>> get_grid(); 

    // return dispensers[p] (pos, label)
    std::vector<Node> get_sink_dispenser_pos();

    // return detectors[x][y] (flag, label)
    std::vector<std::vector<std::pair<bool, std::string>>> get_detector_pos();
};

inline void Solver::print_solver(std::ostream& out){
    out << solver_ << std::endl;
};

inline void Solver::save_solver(std::string filename){
    std::ofstream out(filename);
    if(out.is_open()){
        print_solver(out);
    }
    out.close();
};
