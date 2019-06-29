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

Solver::Solver(const Architecture& arch, z3::context& c): arch_(arch), ctx_(c), solver_(c), no_of_actions_(c), optimize_handle_(1), model_(c) {
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
                    add_constraints();

                    result_ = solver_.check();
                    if(result_ == sat){
                        model_ = solver_.get_model();
                        cout << "Satisfiable" << endl;
                        cout << "Printing sat constraints to file" << endl;
                        // print model


                        return true;
                    }else{
                        cout << "Unsat - (w=" << width << ", h=" << height << ", t=" << time << ")" << endl;
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
        add_constraints();

        result_ = solver_.check();
        if(result_ == sat){
            model_ = solver_.get_model();
            cout << "Satisfiable" << endl;
            cout << "Printing sat constraints to file" << endl;
            // print model


            return true;
        }else{
            cout << "Unsatisfiable" << endl;
        }
    } catch(z3::exception e){
        cerr << e.msg() << endl;
        return true;
    }
    return false;
}

void Solver::print_solution(ostream& out){
    if(result_ == unsat){
        return;
    }

    expr TRUE = ctx_.bool_val(true);

    out << "Dispenser position(s): " << endl;
    for(int i = 0; i < no_of_modules_; i++){
        if(arch_.modules_[i].type_ == DISPENSER){
            Module module = arch_.modules_[i];
            for(int p = 0; p < perimeter_cur_; p++){
                if(eq(model_.eval(dispenser_[p][i]), TRUE)){
                    out << module.label_ << " at " << p << endl;
                }
            }
        }
    }
    out << endl;

    out << "Sink position(s): " << endl;
    for(int p = 0; p < perimeter_cur_; p++){
        if(eq(model_.eval(sink_[p]), TRUE)){
            out << "Sink" << " at " << p << endl;
        }
    }
    out << endl;

    out << "Detector position(s): " << endl;
    for(auto module: arch_.modules_){
        if(module.type_ == DETECTOR){
            for(int x = 0; x < width_cur_; x++){
                for(int y = 0; y < height_cur_; y++){
                    if(eq(model_.eval(detector_[x][y][module.id_]), TRUE)){
                        out << module.label_ << " at (" << x << ", " << y << ")" << endl;
                    }
                }
            }
        }
    }
    out << endl;

    for(int t = 0; t <= time_cur_; t++){
        out << "time = " << t << endl;
        for(int y = 0; y < height_cur_; y++){
            for(int x = 0; x < width_cur_; x++){
                bool flag = false;
                for(auto module: arch_.modules_){
                    if(module.type_ == DETECTOR){
                        if(eq(model_.eval(detector_[x][y][module.id_]) && model_.eval(detecting_[t][module.id_]), TRUE)){
                            out << "d ";
                            flag = true;
                            break;
                        }
                    }else if(module.type_ == MIXER){
                        if(eq(model_.eval(mixing_[t][x][y][module.id_]), TRUE)){
                            out << "m ";
                            flag = true;
                            break;
                        }
                    }
                }
                for(int i = 0; i < no_of_edges_; i++){
                    if(eq(model_.eval(c_[t][x][y][i]), TRUE)){
                        out << i << ' ';
                        flag = true;
                        break;
                    }
                }
                if(!flag)
                    out << "e ";
            }
            out << endl;
        }
        out << endl;
    }
}

void Solver::init(int width, int height, int time){
    // init variables
    c_.clear();
    detecting_.clear();
    mixing_.clear();
    detector_.clear();
    dispenser_.clear();
    sink_.clear();
    solver_ = optimize(ctx_);

    width_cur_ = width;
    height_cur_ = height;
    perimeter_cur_ = (width + height) * 2;
    time_cur_ = time;

    no_of_modules_ = arch_.modules_.size();
    no_of_edges_ = arch_.edges.size();

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
                for(int id = 0; id < no_of_edges_; id++){
                    char name[50];
                    sprintf(name, "c^%d_(%d,%d,%d)", t, w, h, id); // c^t_(x,y,id)
                    c_[t][w][h].push_back(ctx_.bool_const(name));

                    expr e = ite(c_[t][w][h][id], one, zero);
                    counter.push_back(e);
                }
            }
        }
    }
    c_[0].resize(width);
    for(int w = 0; w < width; w++){
        c_[0][w].resize(height);
        for(int h = 0; h < height; h++){
            c_[0][w][h].resize(no_of_edges_, ctx_.bool_val(false));
        }
    }

    // mixing^t_(x,y,id)
    mixing_.resize(time+1);
    for(int t = 0; t <= time; t++){
        mixing_[t].resize(width);
        for(int x = 0; x < width; x++){
            mixing_[t][x].resize(height);
            for(int y = 0; y < height; y++){
                for(int i = 0; i < no_of_modules_; i++){
                    char name[50];
                    sprintf(name, "mixing^%d_(%d,%d,%d)", t, x, y, i);
                    mixing_[t][x][y].push_back(ctx_.bool_const(name));

                    counter.push_back(ite(mixing_[t][x][y][i], one, zero));
                }
            }
        }
    }
    mixing_[0].resize(width);
    for(int w = 0; w < width; w++){
        mixing_[0][w].resize(height);
        for(int h = 0; h < height; h++){
            mixing_[0][w][h].resize(no_of_modules_, ctx_.bool_val(false));
        }
    }

    // detecting^t_(l)
    detecting_.resize(time+1);
    detecting_[0].resize(no_of_modules_, ctx_.bool_val(false));
    for(int t = 1; t <= time; t++){
        for(int i = 0; i < no_of_modules_; i++){
            char name[50];
            sprintf(name, "detecting^%d_(%d)", t, i);
            detecting_[t].push_back(ctx_.bool_const(name));

            expr e = ite(detecting_[t][i], one, zero);
            counter.push_back(e);
        } 
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
            for(int l = 0; l < no_of_modules_; l++){
                char name[50];
                sprintf(name, "detector_(%d,%d,%d)", w, h, l);
                detector_[w][h].push_back(ctx_.bool_const(name));
            }
        }
    }

    // dispenser_(p,l)
    dispenser_.resize(perimeter_cur_);
    for(int p = 0; p < perimeter_cur_; p++){
        for(int l = 0; l < no_of_modules_; l++){
            char name[50];
            sprintf(name, "dispenser_(%d,%d)", p, l);
            dispenser_[p].push_back(ctx_.bool_const(name));
        }
    }

    // sink_(p)
    //sink_.resize(perimeter_cur_);
    for(int p = 0; p < perimeter_cur_; p++){
        char name[50];
        sprintf(name, "sink_(%d)", p);
        //sink_[p] = ctx_.bool_const(name);
        sink_.push_back(ctx_.bool_const(name));
    }
}

void Solver::add_consistency_constraints(){
    expr_vector constraint_vec(ctx_);

    // a cell may not be occupied by more than one droplet or mixer i per time step
    for(int t = 1; t <= time_cur_; t++){
        for(int x = 0; x < width_cur_; x++){
            for(int y = 0; y < height_cur_; y++){
                expr_vector v_tmp(ctx_);
                // mixer
                for(auto module: arch_.modules_){
                    if(module.type_ == MIXER){
                        v_tmp.push_back(mixing_[t][x][y][module.id_]);
                    }
                }

                // droplets
                for(int i = 0; i < no_of_edges_; i++){
                    v_tmp.push_back(c_[t][x][y][i]);
                }
                constraint_vec.push_back(atmost(v_tmp, 1));
            }
        }
    }
    solver_.add(mk_and(constraint_vec));

    // each droplet i may occur in at most one cell per time step
    constraint_vec.resize(0);
    for(int i = 0; i < no_of_edges_; i++){
        for(int t = 1; t <= time_cur_; t++){
            expr_vector v_tmp(ctx_);
            for(int x = 0; x < width_cur_; x++){
                for(int y = 0; y < height_cur_; y++){
                    v_tmp.push_back(c_[t][x][y][i]);
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
            if(!v_tmp.empty()){
                constraint_vec.push_back(atmost(v_tmp, 1));
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
    for(int t = 1; t <= time_cur_; t++){
        for(int x = 0; x < width_cur_; x++){
            for(int y = 0; y < height_cur_; y++){
                for(int i = 0; i < no_of_edges_; i++){
                    // a) in t-1 droplet present at same/neighboring cell
                    expr_vector v1_tmp(ctx_);
                    for(int k = 0; k < 5; k++){
                        int x_new = x + dx[k];
                        int y_new = y + dy[k];
                        if(is_point_inbound(x_new, y_new)){
                            v1_tmp.push_back(c_[t-1][x_new][y_new][i]);
                        }
                    }
                    expr e1_tmp(mk_or(v1_tmp));

                    // b) a dispenser of the same type is nearby
                    expr_vector v2_tmp(ctx_);
                    int id = arch_.edges[i].first;
                    if(arch_.modules_[id].type_ == DISPENSER){ // if the dispenser is of the same type
                        if(x == 0){ // (x,y) on left edge
                            v2_tmp.push_back(dispenser_[perimeter_cur_ - 1 - y][id]);
                        }
                        if(x == width_cur_-1){ // right edge
                            v2_tmp.push_back(dispenser_[width_cur_ - 1 + y][id]);
                        }
                        if(y == 0){ // top edge
                            v2_tmp.push_back(dispenser_[x][id]);
                        }
                        if(y == height_cur_-1){ // bottom edge
                            v2_tmp.push_back(dispenser_[2*width_cur_ + height_cur_ - 1 - x][id]);
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
                            v3_tmp.push_back(c_[t-1][x_new][y_new][i]);
                        }
                    }
                    expr e3_tmp(ctx_);
                    id = arch_.edges[i].first;
                    if(arch_.modules_[id].type_ == MIXER){
                        e3_tmp = mixing_[t-1][x][y][id] && !mk_or(v3_tmp);
                    }else{
                        e3_tmp = ctx_.bool_val(false);
                    }

                    // d) result of a detecting op (because id is changed)
                    // TODO: chekc if this is correct
                    expr e4_tmp(ctx_);
                    int id_before_detection = -1;
                    int id_detector = arch_.edges[i].first;
                    if(arch_.modules_[id_detector].type_ == DETECTOR){
                        for(unsigned int m = 0; m < arch_.edges.size(); m++){
                            if(arch_.edges[m].second == arch_.modules_[id_detector].id_){
                                id_before_detection = arch_.edges[m].second;
                                break;
                            }
                        }
                        if(id_before_detection == -1){
                            e4_tmp = ctx_.bool_val(false);
                        }else{
                            e4_tmp = c_[t-1][x][y][id_before_detection] && detector_[x][y][id_detector] && detecting_[t-1][id_detector] && !detecting_[t][id_detector];
                        }
                    }else{
                        e4_tmp = ctx_.bool_val(false);
                    }

                    solver_.add(implies(c_[t][x][y][i], e1_tmp || e2_tmp || e3_tmp || e4_tmp));
                }
            }
        }
    }
}

void Solver::add_operations(){
    
    // mixing operation
    // TODO: change everything: ids and multiple mix output
    for(auto module: arch_.modules_){
        if(module.type_ == MIXER){
            expr_vector constraint_vec(ctx_);

            int id_after_mix;
            for(unsigned int m = 0; m < arch_.edges.size(); m++){
                if(arch_.edges[m].first == module.id_){
                    id_after_mix = m;
                    break;
                }
            }
            int d = module.time_;
            for(int t = d + 2; t <= time_cur_; t++){
                for(int x = 0; x < width_cur_; x++){
                    for(int y = 0; y < height_cur_; y++){
                        int w = module.w;
                        int h = module.h;
                        for(int k = 0; k < w * h; k++){
                            expr condition1 = c_[t][x][y][id_after_mix]; // output is present
                            expr_vector condition2_vec(ctx_); // droplet_i is not present at time t-1
                            for(int l = 0; l < 5; l++){
                                int x_new = x + dx[l];
                                int y_new = y + dy[l];
                                if(is_point_inbound(x_new, y_new)){
                                    condition2_vec.push_back(c_[t-1][x_new][y_new][id_after_mix]);
                                }
                            } 
                            expr condition2 = mk_and(condition2_vec); 

                            expr_vector result1_vec(ctx_);
                            for(auto u: arch_.backward_edges_[module.id_]){
                                int id_input;
                                for(unsigned int m = 0; m < arch_.edges.size(); m++){
                                    if(arch_.edges[m].first == u && arch_.edges[m].second == module.id_){
                                        id_input = m;
                                        break;
                                    }
                                }

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
                                expr a = mk_or(a_vec);

                                // input-droplet disappear after start of mixing operation
                                expr_vector b_vec(ctx_);
                                for(int k = 0; k < 5; k++){
                                    int x_new = x + dx[k];
                                    int y_new = y + dy[k];
                                    if(is_point_inbound(x_new, y_new)){
                                        b_vec.push_back(c_[t-d][x_new][y_new][id_input]);
                                    }
                                }
                                expr b = mk_and(b_vec);

                                result1_vec.push_back(a && !b);
                            }

                            // one of the subgrids is occupied
                            expr_vector c_vec(ctx_);
                            // Here I allow the output droplet to be anywhere of the mixer
                            // TODO: need to check if it is correct
                            int x0 = x - k % module.w;
                            int y0 = y - k / module.w;
                            expr_vector v_tmp(ctx_);
                            if(is_point_inbound(x0,y0) && is_point_inbound(x0+w-1, y0+w-1)){
                                for(int x_new = x0; x_new < x0+w; x_new++){
                                    for(int y_new = y0; y_new < y0+h; y_new++){
                                        for(int t_lag = t-d; t_lag < t; t_lag++){
                                            v_tmp.push_back(mixing_[t_lag][x_new][y_new][module.id_]);
                                        }
                                    }
                                }
                            }
                            c_vec.push_back(mk_and(v_tmp));

                            constraint_vec.push_back(implies(condition1 && condition2, mk_and(result1_vec) && mk_or(c_vec)));
                        }
                    }
                }
            }
            solver_.add(mk_and(constraint_vec));
        }
    }

    // detecting operation
    expr_vector constraint_vec(ctx_);
    for(auto module: arch_.modules_){
        if(module.type_ == DETECTOR){
            int id_detector = module.id_;
            int id_droplet;
            for(unsigned int m = 0; m < arch_.edges.size(); m++){ // find the droplet being detected
                if(arch_.edges[m].second == id_detector){
                    id_droplet = m;
                    break;
                }
            }
            for(int t = 1; t <= time_cur_; t++){
                for(int x = 0; x < width_cur_; x++){
                    for(int y = 0; y < height_cur_; y++){
                        expr a = (detecting_[t][id_detector] && !detecting_[t-1][id_detector] && c_[t][x][y][id_droplet]); // a -> b

                        expr_vector v_tmp(ctx_);
                        for(int t_lag = t; t_lag <= t+module.time_; t++){ // TODO: check if [t, t+d] or [t, t+d)
                            v_tmp.push_back(c_[t_lag][x][y][id_droplet] && detecting_[t_lag][id_detector]);
                        }
                        expr b = (detector_[x][y][id_detector] && mk_and(v_tmp));

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
                for(int i = 0; i < no_of_edges_; i++){
                    expr_vector a_vec(ctx_); // droplet_i present at t-1 but not at t
                    for(int k = 0; k < 5; k++){
                        int x_new = x + dx[k];
                        int y_new = y + dy[k];
                        if(is_point_inbound(x_new, y_new)){
                            a_vec.push_back(c_[t][x_new][y_new][i]);
                        }
                    }
                    expr a = (c_[t-1][x][y][i] && !mk_or(a_vec));

                    expr_vector b_vec(ctx_); // there is a sink at reachable position
                    if(x == 0){ // (x,y) on left edge
                        b_vec.push_back(sink_[perimeter_cur_ - 1 - y]);
                    }
                    if(x == width_cur_-1){ // right edge
                        b_vec.push_back(sink_[width_cur_ - 1 + y]);
                    }
                    if(y == 0){ // top edge
                        b_vec.push_back(sink_[x]);
                    }
                    if(y == height_cur_-1){ // bottom edge
                        b_vec.push_back(sink_[2*width_cur_ + height_cur_ - 1 - x]);
                    }
                    expr b = mk_or(b_vec);

                    expr_vector c1_vec(ctx_); // 
                    expr_vector c2_vec(ctx_); // mixing op in N of (x,y) at t && no mixing op at t-1
                    expr c(ctx_);
                    int id_mixer = arch_.edges[i].second;
                    if(arch_.modules_[id_mixer].type_ == MIXER){
                        for(int k = 0; k < 5; k++){
                            int x_new = x + dx[k];
                            int y_new = y + dy[k];
                            if(is_point_inbound(x_new, y_new)){
                                expr_vector v_tmp(ctx_);
                                c1_vec.push_back(mixing_[t][x_new][y_new][id_mixer]);
                                c2_vec.push_back(mixing_[t-1][x_new][y_new][id_mixer]);
                            }
                        }
                        c = (mk_or(c1_vec) && !mk_or(c2_vec));
                    }else{
                        c = ctx_.bool_val(false);
                    }

                    expr d(ctx_); // a droplet finished being detected
                    // TODO: not finished
                    int id_detector = arch_.edges[i].second;
                    if(arch_.modules_[id_detector].type_ == DETECTOR){
                        d = detector_[x][y][id_detector] && detecting_[t-1][id_detector] && !detecting_[t][id_detector];
                    }else{
                        d = ctx_.bool_val(false);
                    } 

                    constraint_vec.push_back(implies(a, b || c || d));
                }
            }
        }
    }
    solver_.add(mk_and(constraint_vec));
}

void Solver::add_objectives(){
    // all droplets have to be present for at least one step
    expr_vector all_droplets_appear_vec(ctx_);
    for(int i = 0; i < no_of_edges_; i++){
        expr_vector v_tmp(ctx_);
        for(int t = 1; t <= time_cur_; t++){
            for(int x = 0; x < width_cur_; x++){
                for(int y = 0; y < height_cur_; y++){
                        v_tmp.push_back(c_[t][x][y][i]);
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
    for(int i = 0; i < no_of_edges_; i++){
        for(int t = 1; t <= time_cur_; t++){
            for(int x = 0; x < width_cur_; x++){
                for(int y = 0; y < height_cur_; y++){
                    expr a = c_[t][x][y][i];

                    // (1): for any droplet^t_i, there should be no other droplet nearb at time t 
                    expr_vector v1_tmp(ctx_);
                    // (2): ...., also no droplet at time t+1
                    expr_vector v2_tmp(ctx_);
                    for(int x_new = x-1; x_new <= x+1; x_new++){
                        for(int y_new = y-1; y_new <= y+1; y_new++){
                            if(is_point_inbound(x_new, y_new)){
                                for(int j = 0; j < no_of_edges_; j++){
                                    if(j == i){
                                        continue;
                                    }
                                    v1_tmp.push_back(c_[t][x_new][y_new][j]);
                                    if(t < time_cur_){
                                        v2_tmp.push_back(c_[t+1][x_new][y_new][j]);
                                    }
                                }
                            }
                        }
                    }
                    if(t < time_cur_){
                        expr v1 = mk_or(v1_tmp);
                        expr v2 = mk_or(v2_tmp);
                        solver_.add(implies(a, !v1 && !v2));
                    }else{
                        expr v1 = mk_or(v1_tmp);
                        solver_.add(implies(a, !v1));
                    }
                }
            }
        }
    }
}

void Solver::add_constraints(){
    add_consistency_constraints();
    add_placement_constraints();
    add_movement_constraints();
    add_operations();
    add_objectives();
    add_fluidic_constraints();
}