#include "z3++.h"
#include "Solver.h"

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <cstdio>

using namespace std;
using namespace z3;

#define EQ(exp, i) (atmost((exp), (i)) && atleast((exp), (i)))

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

                    result_ = solver_.check();
                    if(result_ == sat){
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

bool Solver::solve(int width, int height, int time){
    try {
        init(width, height, time);
        add_consistency_constraints();
        add_placement_constraints();
        add_movement_constraints();
        add_fluidic_constraints();

        result_ = solver_.check();
        if(result_ == sat){
            cout << "Satisfiable" << endl;
            cout << "Printing sat constraints to file" << endl;
            // print model


            return true;
        }
    } catch(z3::exception e){
        cerr << e.msg() << endl;
        return true;
    }
    return false;
}

void Solver::print_solution(){
    if(result_ == unsat){
        return;
    }


}

void Solver::init(int width, int height, int time){
    // init variables
    c_.clear();
    detecting_.clear();
    detector_.clear();
    dispenser_.clear();
    sink_.clear();
    solver_ = optimize(ctx_);

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
    c_.resize(time+1);
    for(int t = 1; t <= time; t++){
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
    for(int w = 0; w < width; w++){
        for(int h = 0; h < height; h++){
            for(int i = 0; i < no_of_modules*2; i++){
                c_[0][w][h][i] = ctx_.bool_val(false);
            }
        }
    }

    // detecting^t_(l)
    detecting_.resize(time+1);
    for(int t = 1; t <= time; t++){
        detecting_[t].resize(no_of_modules);
        for(int i = 0; i < no_of_modules; i++){
            char name[50];
            sprintf(name, "detecting^%d_(%d)", t, i);
            detecting_[t][i] = ctx_.bool_const(name);

            expr e = ite(detecting_[t][i], one, zero);
            counter.push_back(e);
        } 
    }
    for(int i = 0; i < no_of_modules; i++){
        detecting_[0][i] = ctx_.bool_val(false);
    }

    // add optimizing condition to solver to reduce total number of steps
    no_of_actions_ = ctx_.int_const("no_of_actions");
    solver_.add(no_of_actions_ == sum(counter));
    optimize_handle_ = solver_.minimize(no_of_actions_);

    // detector_(x,y,l)
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
    sink_.resize(perimeter_cur_);
    for(int p = 0; p < perimeter_cur_; p++){
        char name[50];
        sprintf(name, "sink_(%d)", p);
        sink_[p] = ctx_.bool_const(name);
    }

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

    // in my setup, the same droplet before and after dectecting have different ids, so they have to be set equal for the rest to work
    // TODO: check if is correct
    constraint_vec.resize(0);
    for(auto module: arch_.modules_){
        if(module.type_ == DETECTOR){
            int id_before_detecting = arch_.backward_edges_[module.id_][0] + no_of_modules;
            int id_after_detecting = module.id_ + no_of_modules;
            for(int t = 1; t <= time_cur_; t++){
                for(int x = 0; x < width_cur_; x++){
                    for(int y = 0; y < height_cur_; y++){
                        constraint_vec.push_back(c_[t][x][y][id_before_detecting] == c_[t][x][y][id_after_detecting]);
                    }
                }
            }
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
            constraint_vec.push_back(EQ(v_tmp, 1)); 
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
            constraint_vec.push_back(EQ(v_tmp, module.desired_amount_));
        }else if(module.type_ == SINK){
            expr_vector v_tmp(ctx_);
            for(int p = 0; p < perimeter_cur_; p++){
                v_tmp.push_back(sink_[p]);
            }
            constraint_vec.push_back(EQ(v_tmp, module.desired_amount_));
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
                    // droplet corresponding to Node type SINK does not appear
                    if(arch_.modules_[i].type_ == SINK){
                        continue;
                    }

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
    
    // mixing operation
    for(auto module: arch_.modules_){
        if(module.type_ == MIXER){
            expr_vector constraint_vec(ctx_);

            int id_after_mix = module.id_ + arch_.modules_.size();
            int d = module.time_;
            for(int t = d + 2; t <= time_cur_; t++){
                for(int x = 0; x < width_cur_; x++){
                    for(int y = 0; y < height_cur_; y++){
                        expr condition1(c_[t][x][y][id_after_mix]); // output is present
                        expr_vector condition2_vec(ctx_); // droplet_i is not present at time t-1
                        for(int k = 0; k < 5; k++){
                            int x_new = x + dx[k];
                            int y_new = y + dy[k];
                            if(is_point_inbound(x_new, y_new)){
                                condition2_vec.push_back(c_[t-1][x_new][y_new][id_after_mix]);
                            }
                        } 
                        expr condition2 = mk_and(condition2_vec); 

                        expr_vector result1_vec(ctx_);
                        for(auto id_input: arch_.backward_edges_[module.id_]){
                            // input droplet is present at time t-(d+1)
                            expr_vector a_vec(ctx_);
                            // TODO: is this conidition correct?? isn't that the droplet being in the neighborhood of G enough?
                            for(int k = 0; k < 5; k++){
                                int x_new = x + dx[k];
                                int y_new = y + dy[k];
                                if(is_point_inbound(x_new, y_new)){
                                    a_vec.push_back(c_[t-d-1][x_new][y_new][id_input]);
                                }
                            }
                            expr a = (EQ(a_vec, 1));

                            // input-droplet disappear after start of mixing operation
                            expr_vector b_vec(ctx_);
                            for(int k = 0; k < 5; k++){
                                int x_new = x + dx[k];
                                int y_new = y + dy[k];
                                if(is_point_inbound(x_new, y_new)){
                                    b_vec.push_back(c_[t-d][x_new][y_new][id_input]);
                                }
                            }
                            expr b = (EQ(b_vec, 0));

                            result1_vec.push_back(a && b);
                        }

                        // one of the subgrids is occupied
                        expr_vector c_vec(ctx_);
                        // Here I allow the output droplet to be anywhere of the mixer
                        // TODO: need to check if it is correct
                        for(int k = 0; k < module.w * module.h; k++){
                            int w = module.w;
                            int h = module.h;
                            int x0 = x - k % module.w;
                            int y0 = y - k / module.w;
                            expr_vector v_tmp(ctx_);
                            if(is_point_inbound(x0,y0) && is_point_inbound(x0+w-1, y0+w-1)){
                                for(int x_new = x0; x_new < x0+w; x_new++){
                                    for(int y_new = y0; y_new < y0+h; y_new++){
                                        for(int t_lag = t-d; t_lag < t; t_lag++){
                                            v_tmp.push_back(c_[t_lag][x_new][y_new][module.id_]);
                                        }
                                    }
                                }
                            }
                            c_vec.push_back(mk_and(v_tmp));
                        }
                        expr result = (mk_and(result1_vec) && mk_or(c_vec));
                        constraint_vec.push_back(implies(condition1 && condition2, result));
                    }
                }
            }
            solver_.add(mk_and(constraint_vec));
        }
    }

    // detecting operation
    int no_of_modules = arch_.modules_.size();
    expr_vector constraint_vec(ctx_);
    for(auto module: arch_.modules_){
        if(module.type_ == DETECTOR){
            int id = module.id_;
            for(int t = 1; t <= time_cur_; t++){
                for(int x = 0; x < width_cur_; x++){
                    for(int y = 0; y < height_cur_; y++){
                        expr a = (detecting_[t][id] && !detecting_[t-1][id] && c_[t][x][y][id + no_of_modules]); // a -> b

                        expr_vector v_tmp(ctx_);
                        for(int t_lag = t; t_lag <= t+module.time_; t++){ // TODO: check if [t, t+d] or [t, t+d)
                            v_tmp.push_back(c_[t_lag][x][y][id+no_of_modules] && detecting_[t_lag][id]);
                        }
                        expr b = (detector_[x][y][id] && mk_and(v_tmp));

                        constraint_vec.push_back(implies(a, b));
                    }
                }
            }
        }
    }
    solver_.add(mk_and(constraint_vec));

    // Disappearance of droplet
    constraint_vec.resize(0);
    for(int t = 1; t <= time_cur_; t++){
        for(int x = 0; x < width_cur_; x++){
            for(int y = 0; y < height_cur_; y++){
                for(int i = 0; i < no_of_modules; i++){
                    // droplet of SINK does not appear
                    if(arch_.modules_[i].type_ == SINK){
                        continue;
                    }

                    expr_vector a_vec(ctx_); // droplet_i present at t-1 but not at t
                    for(int k = 0; k < 5; k++){
                        int x_new = x + dx[k];
                        int y_new = y + dy[k];
                        if(is_point_inbound(x_new, y_new)){
                            a_vec.push_back(c_[t][x_new][y_new][i+no_of_modules]);
                        }
                    }
                    expr a = (c_[t-1][x][y][i+no_of_modules] && !mk_or(a_vec));

                    expr_vector b_vec(ctx_); // there is a sink at reachable position
                    if(x == 0){ // (x,y) on left edge
                        b_vec.push_back(sink_[perimeter_cur_ - 1 - y]);
                    }else if(x == width_cur_-1){ // right edge
                        b_vec.push_back(sink_[width_cur_ - 1 + y]);
                    }else if(y == 0){ // top edge
                        b_vec.push_back(sink_[x]);
                    }else if(y == height_cur_-1){ // bottom edge
                        b_vec.push_back(sink_[2*width_cur_ + height_cur_ - 1 - x]);
                    }
                    expr b = mk_or(b_vec);

                    expr_vector c1_vec(ctx_); // mixing op in N of (x,y) at t
                    expr_vector c2_vec(ctx_); // no mixing op at t-1
                    for(auto module: arch_.modules_){
                        if(module.type_ == MIXER){
                            int id_mixer = module.id_;
                            for(int k = 0; k < 5; k++){
                                int x_new = x + dx[k];
                                int y_new = y + dy[k];
                                if(is_point_inbound(x_new, y_new)){
                                    c1_vec.push_back(c_[t][x_new][y_new][id_mixer]);
                                    c2_vec.push_back(c_[t-1][x_new][y_new][id_mixer]);
                                }
                            }
                        }
                    }
                    expr c = (mk_or(c1_vec) && !mk_or(c2_vec));

                    constraint_vec.push_back(implies(a, b || c));
                }
            }
        }
    }
    solver_.add(mk_and(constraint_vec));
}

void Solver::add_objectives(){
    // all droplets have to be present for at least one step
    expr_vector all_droplets_appear_vec(ctx_);
    for(int i = 0; i < arch_.modules_.size(); i++){
        if(arch_.modules_[i].type_ == SINK){
            continue;
        }

        expr_vector v_tmp(ctx_);
        for(int t = 1; t <= time_cur_; t++){
            for(int x = 0; x < width_cur_; x++){
                for(int y = 0; y < height_cur_; y++){
                        v_tmp.push_back(c_[t][x][y][i+arch_.modules_.size()]);
                }
            }
        }
        all_droplets_appear_vec.push_back(mk_or(v_tmp));
    }
    solver_.add(mk_and(all_droplets_appear_vec));

    // detecting op must be done
    expr_vector detection_triggered_vec(ctx_);
    for(auto module: arch_.modules_){
        if(module.type_ == DETECTOR){
            expr_vector v_tmp(ctx_);
            for(int t = 1; t <= time_cur_; t++){
                v_tmp.push_back(detecting_[t][module.id_]);
            }
            detection_triggered_vec.push_back(EQ(v_tmp, module.time_));
        }
    }
    solver_.add(mk_and(detection_triggered_vec));
}

void Solver::add_fluidic_constraints(){

    int no_of_modules = arch_.modules_.size();

    for(int i = 0; i < no_of_modules; i++){
        if(arch_.modules_[i].type_ == SINK){
            continue;
        }
        for(int t = 1; t <= time_cur_; t++){
            for(int x = 0; x < width_cur_; x++){
                for(int y = 0; y < height_cur_; y++){
                    expr a = c_[t][x][y][i+no_of_modules];

                    // (1): for any droplet^t_i, there should be no other droplet nearb at time t 
                    expr_vector v1_tmp(ctx_);
                    // (2): ...., also no droplet at time t+1
                    expr_vector v2_tmp(ctx_);
                    for(int x_new = x-1; x_new <= x+1; x_new++){
                        for(int y_new = y-1; y_new <= y+1; y_new++){
                            if(is_point_inbound(x_new, y_new)){
                                for(int j = 0; j < no_of_modules; j++){
                                    if(arch_.modules_[i].type_ == SINK || j == i){
                                        continue;
                                    }
                                    v1_tmp.push_back(c_[t][x_new][y_new][j+no_of_modules]);
                                    v2_tmp.push_back(c_[t+1][x_new][y_new][j+no_of_modules]);
                                }
                            }
                        }
                    }
                    expr v1 = mk_or(v1_tmp);
                    expr v2 = mk_or(v2_tmp);
                    solver_.add(implies(a, !v1 && !v2));
                }
            }
        }
    }
}