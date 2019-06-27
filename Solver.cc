#include "z3++.h"
#include "Solver.h"

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <cstdio>

using namespace std;
using namespace z3;

const int dx[] = {-1, 0, 1, 0, 0};
const int dy[] = { 0,-1, 0, 1, 0};

Solver::Solver(const Architecture& arch, z3::context& c): arch_(arch), ctx_(c), solver_(c), no_of_actions_(c), optimize_handle_(1) {
    // read width, height, ...
    width_limit_ = arch_.width_limit_;
    height_limit_ = arch_.height_limit_;
    time_limit_ = arch.time_limit_;
}

bool Solver::solve(){
    try {
        for(int width = 1; width <= width_limit_; width++){
            for(int height = 1; height <= height_limit_; height++){
                for(int time = 1; time <= time_limit_; time++){
                    init(width, height, time);
                    add_consistency_constraints();
                    add_placement_constraints();
                    add_movement_constraints();
                    add_fluidic_constraints();

                    auto result = solver_.check();
                    if(result == sat){
                        cout << "Satisfiable" << endl;
                        cout << "Printing sat constraints to file" << endl;
                        // print model


                        return true;
                    }
                }
            }
        }    
    } catch(z3::exception e){
        cerr << e.msg() << endl;
        return true;
    }
    return false;
}

void Solver::init(int width, int height, int time){
    width_cur_ = width;
    height_cur_ = height;
    perimeter_cur_ = (width + height) * 2;
    time_cur_ = time;

    int no_of_modules = arch_.modules_.size();

    // count otal droplet/mixer appearance
    expr_vector counter(ctx_);
    // constants
    expr zero = ctx_.int_val(0);
    expr one = ctx_.int_val(1);
    // c^t_(x,y,id)
    c_.clear();
    c_.resize(time+1);
    for(int t = 0; t <= time; t++){
        c_[t].resize(width);
        for(int w = 0; w < width; w++){
            c_[t][w].resize(height);
            for(int h = 0; h < height; h++){
                // see definition of id
                c_[t][w][h].resize(no_of_modules*2);
                for(int id = 0; id < no_of_modules; id++){
                    char name[50];
                    // for modules
                    sprintf(name, "c^%d_(%d,%d,%d)", t, w, h, id); // c^t_(x,y,id)
                    c_[t][w][h][id] = ctx_.bool_const(name);

                    expr e = ite(c_[t][w][h][id], one, zero);
                    counter.push_back(e);
                }
                
                for(int i = 0; i < no_of_modules; i++){
                    char name[50];
                    // for droplets
                    sprintf(name, "c^%d_(%d,%d,%d)", t, w, h, i + no_of_modules); // c^t_(x,y,id)
                    c_[t][w][h][i + no_of_modules] = ctx_.bool_const(name);

                    expr e = ite(c_[t][w][h][no_of_modules + i], one, zero);
                    counter.push_back(e);
                }

            }
        }
    }

    // detecting^t_(l)
    detecting_.clear();
    detecting_.resize(time+1);
    for(int t = 0; t <= time; t++){
        detecting_[t].resize(no_of_modules);
        for(int i = 0; i < no_of_modules; i++){
            char name[50];
            sprintf(name, "detecting^%d_(%d)", t, i);
            detecting_[t][i] = ctx_.bool_const(name);

            expr e = ite(detecting_[t][i], one, zero);
            counter.push_back(e);
        } 
    }

    // add optimizing condition to solver to reduce total number of steps
    no_of_actions_ = ctx_.int_const("no_of_actions");
    solver_.add(no_of_actions_ == sum(counter));
    optimize_handle_ = solver_.minimize(no_of_actions_);

    // detector_(x,y,l)
    detector_.clear();
    detector_.resize(width);
    for(int w = 0; w < width; w++){
        detector_[w].resize(height);
        for(int h = 0; h < height; h++){
            detector_[w][h].resize(no_of_modules);
            for(int l = 0; l < no_of_modules; l++){
                char name[50];
                sprintf(name, "detector_(%d,%d,%d)", w, h, l);
                detector_[w][h][l] = ctx_.bool_const(name);
            }
        }
    }

    // dispenser_(p,l)
    dispenser_.clear();
    dispenser_.resize(perimeter_cur_);
    for(int p = 0; p < perimeter_cur_; p++){
        dispenser_[p].resize(no_of_modules);
        for(int l = 0; l < no_of_modules; l++){
            char name[50];
            sprintf(name, "dispenser_(%d,%d)", p, l);
            dispenser_[p][l] = ctx_.bool_const(name);
        }
    }

    // sink_(p)
    sink_.clear();
    sink_.resize(perimeter_cur_);
    for(int p = 0; p < perimeter_cur_; p++){
        char name[50];
        sprintf(name, "sink_(%d)", p);
        sink_[p] = ctx_.bool_const(name);
    }

    // TODO: should we add init condition of c at time 0?
}

void Solver::add_consistency_constraints(){
    int no_of_modules = arch_.modules_.size();
    expr_vector constraint_vec(ctx_);

    // a cell may not be occupied by more than one droplet or mixer i per time step
    for(int t = 1; t <= time_cur_; t++){
        for(int x = 0; x < width_cur_; x++){
            for(int y = 0; y < height_cur_; y++){
                expr_vector v_tmp(ctx_);
                // mixer
                for(auto module: arch_.modules_){
                    if(module.type_ == MIXER){
                        v_tmp.push_back(c_[t][x][y][module.id_]);
                    }
                }

                // droplets
                for(int i = 0; i < no_of_modules; i++){
                    v_tmp.push_back(c_[t][x][y][i+no_of_modules]);
                }
                constraint_vec.push_back(atmost(v_tmp, 1));
            }
        }
    }
    solver_.add(mk_and(constraint_vec));

    // each droplet i may occur in at most one cell per time step
    constraint_vec.resize(0);
    for(int i = 0; i < no_of_modules; i++){
        for(int t = 1; t <= time_cur_; t++){
            expr_vector v_tmp(ctx_);
            for(int x = 0; x < width_cur_; x++){
                for(int y = 0; y < height_cur_; y++){
                    v_tmp.push_back(c_[t][x][y][i+no_of_modules]);
                }
            }
            constraint_vec.push_back(atmost(v_tmp, 1));
        }
    }
    solver_.add(mk_and(constraint_vec));

    // in each position p outside of the grid, there may be at most one dispenser (this applies for all types l) or sink
    constraint_vec.resize(0);
    for(int p = 0; p < perimeter_cur_; p++){
        expr_vector v_tmp(ctx_);
        v_tmp.push_back(sink_[p]);
        for(auto module: arch_.modules_){
            if(module.type_ == DISPENSER){
                v_tmp.push_back(dispenser_[p][module.id_]);
            }
        }
        constraint_vec.push_back(atmost(v_tmp, 1));
    }
    solver_.add(mk_and(constraint_vec));

    // each cell may be occupied by atmost one detector
    constraint_vec.resize(0);
    for(int x = 0; x < width_cur_; x++){
        for(int y = 0; y < height_cur_; y++){
            expr_vector v_tmp(ctx_);
            for(auto module: arch_.modules_){
                if(module.type_ == DETECTOR){
                    v_tmp.push_back(detector_[x][y][module.id_]);
                }
            }
            constraint_vec.push_back(atmost(v_tmp, 1));
        }
    }
    solver_.add(mk_and(constraint_vec));
}

void Solver::add_placement_constraints(){

    expr_vector constraint_vec(ctx_);
    
    // detectors over all possible cells, one detector for every type l of fluids is placed
    for(auto module: arch_.modules_){
        if(module.type_ == DETECTOR){
            expr_vector v_tmp(ctx_);
            for(int x = 0; x < width_cur_; x++){
                for(int y = 0; y < height_cur_; y++){
                    v_tmp.push_back(detector_[x][y][module.id_]);
                }
            }
            constraint_vec.push_back(sum(v_tmp) == 1); 
        }
    }
    solver_.add(mk_and(constraint_vec));

    // dispensers and sinks, desired amount of every type of dispensers and sinks are placed
    constraint_vec.resize(0);
    for(auto module: arch_.modules_){
        if(module.type_ == DISPENSER){
            expr_vector v_tmp(ctx_);
            for(int p = 0; p < perimeter_cur_; p++){
                v_tmp.push_back(dispenser_[p][module.id_]);
            }
            constraint_vec.push_back(sum(v_tmp) == module.desired_amount_);
        }else if(module.type_ == SINK){
            expr_vector v_tmp(ctx_);
            for(int p = 0; p < perimeter_cur_; p++){
                v_tmp.push_back(sink_[p]);
            }
            constraint_vec.push_back(sum(v_tmp) == module.desired_amount_);
        }
    }
    solver_.add(mk_and(constraint_vec));
}

void Solver::add_movement_constraints(){
    int no_of_modules = arch_.modules_.size();
    for(int t = 1; t <= time_cur_; t++){
        for(int x = 0; x < width_cur_; x++){
            for(int y = 0; y < height_cur_; y++){
                for(int i = 0; i < no_of_modules; i++){
                    // a) in t-1 droplet present at same/neighboring cell
                    expr_vector v1_tmp(ctx_);
                    for(int k = 0; k < 5; k++){
                        int x_new = x + dx[k];
                        int y_new = y + dy[k];
                        if(is_point_inbound(x_new, y_new)){
                            v1_tmp.push_back(c_[t-1][x_new][y_new][i + no_of_modules]);
                        }
                    }
                    expr e1_tmp(mk_or(v1_tmp));

                    // b) a dispenser of the same type is nearby
                    expr_vector v2_tmp(ctx_);
                    if(arch_.modules_[i].type_ == DISPENSER){ // if the dispenser is of the same type
                        for(int k = 0; k < 5; k++){
                            if(x == 0){ // (x,y) on left edge
                                v2_tmp.push_back(dispenser_[perimeter_cur_ - 1 - y][i]);
                            }else if(x == width_cur_-1){ // right edge
                                v2_tmp.push_back(dispenser_[width_cur_ - 1 + y][i]);
                            }else if(y == 0){ // top edge
                                v2_tmp.push_back(dispenser_[x][i]);
                            }else if(y == height_cur_-1){ // bottom edge
                                v2_tmp.push_back(dispenser_[2*width_cur_ + height_cur_ - 1 - x][i]);
                            }
                        }
                    }
                    expr e2_tmp(mk_or(v2_tmp));


                    // c) result of a mixing op
                    // (x,y) occupied by mixer at (t-1) &&
                    // neighboring cells not occupied by droplet i
                    expr_vector v3_tmp(ctx_);
                    for(int k = 0; k < 5; k++){
                        int x_new = x + dx[k];
                        int y_new = y + dy[k];
                        if(is_point_inbound(x_new, y_new)){
                            v3_tmp.push_back(c_[t-1][x_new][y_new][i+no_of_modules]);
                        }
                    }
                    expr e3_tmp(ctx_);
                    if(arch_.modules_[i].type_ == MIXER){
                        e3_tmp = c_[t-1][x][y][i] && !mk_or(v3_tmp);
                    }else{
                        e3_tmp = false && !mk_or(v3_tmp);
                    }

                    solver_.add(implies(c_[t][x][y][i+no_of_modules], e1_tmp || e2_tmp || e3_tmp));

                    //HACK: different from github: no detecting etc.
                }
            }
        }
    }
}

void Solver::add_operations(){


}

void Solver::add_fluidic_constraints(){


}