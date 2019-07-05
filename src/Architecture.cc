#include "Architecture.h"
#include "Module.h"
#include <fstream>
#include <iostream>
#include <string>
#include <string.h>

using namespace std;

#define DEBUG(x) cout<<x<<' ';

Architecture::Architecture() {
    num_sink_ = 0;
    num_dispenser_ = 0;
    num_mixer_ = 0;
    num_detector_ = 0;
}

Architecture::Architecture(const string& filename){
    build_from_file(filename);
    print_to_graph(filename);
}

vector<string> split(string s, char c){
    vector<string> res;
    while(s.find(c) != string::npos){
        string tmp = s.substr(0, s.find(c));
        tmp = tmp.substr(tmp.find_first_not_of(' '), tmp.find_last_not_of(' ')-tmp.find_first_not_of(' ')+1);
        res.push_back(tmp);
        s = s.substr(s.find(c)+1);
    }
    string tmp = s;
    tmp = tmp.substr(tmp.find_first_not_of(' '), tmp.find_last_not_of(' ')-tmp.find_first_not_of(' ')+1);
    res.push_back(tmp);
    return res;
}

void Architecture::build_from_file(const string &filename){
    // erase previous data
    num_sink_ = num_dispenser_ = 0;
    for(auto edges: forward_edges_){
        edges_.clear();
    }
    forward_edges_.clear();
    for(auto edges: backward_edges_){
        edges.clear();
    }
    backward_edges_.clear();
    modules_.clear();

    ifstream in_file(filename);
    // process line by line
    while(!in_file.eof()){
        string line;
        getline(in_file, line);
        if(line.find('(') == string::npos){
            continue;
        }

        string type = line.substr(0, line.find(' '));
        auto params = split(line.substr(line.find('(')+1, line.find(')') - line.find('(')-1), ','); // TODO: test this
        if(params.size() < 1){
            cout << "Error reading input file: " << filename << endl;
        }

        if(type == "DAGNAME"){
            label_ = params[0];
        }else if(type == "EDGE"){
            int u = stoi(params[0]);
            int v = stoi(params[1]);
            edges_.push_back(make_pair(u-1, v-1));
        }else if(type == "NODE"){
            Module m;
            m.id_ = stoi(params[0]) - 1;
            string type_module = params[1];
            if(type_module == "MIX"){
                m.type_ = MIXER;
                m.drops_ = stoi(params[2]);
                m.time_ = stoi(params[3]);
                m.label_ = params[4];
                num_mixer_++;
            }else if(type_module == "DISPENSE"){
                m.type_ = DISPENSER;
                m.fluid_type_ = params[2];
                m.volume_ = stoi(params[3]);
                m.label_ = params[4];
                num_dispenser_++;
            }else if(type_module == "OUTPUT"){
                m.type_ = SINK;
                m.sink_name_ = params[2];
                m.label_ = params[3];
                num_sink_++;
            }else if(type_module == "DETECT"){
                m.type_ = DETECTOR;
                m.drops_ = stoi(params[2]);
                m.time_ = stoi(params[3]);
                m.label_ = params[4];
                num_detector_++;
            }else{
                cout << "Module type not yet supported!" << endl;
            }
            nodes_.push_back(m);
            if(modules_.count(m.label_) == 0){
                modules_[m.label_] = m;
            }
        }else if(type == "TIME"){
            time_limit_ = stoi(params[0]);
        }else if(type == "SIZE"){
            width_limit_ = stoi(params[0]);
            height_limit_ = stoi(params[1]);
        }else if(type == "MOD"){
            string label = params[0];
            switch(modules_[label].type_){
                case NONE:
                    break;
                case SINK:
                case DISPENSER:
                    modules_[label].desired_amount_ = stoi(params[1]);
                    break;
                case MIXER:
                    modules_[label].w = stoi(params[1]);
                    modules_[label].h = stoi(params[2]);
                    break;
                case DETECTOR:
                    break;
            }
        }
    }
    in_file.close();

    // prepare edge lists
    forward_edges_.assign(nodes_.size(), vector<int>());
    backward_edges_.assign(nodes_.size(), vector<int>());
    for(auto edge: edges_){
        forward_edges_[edge.first].push_back(edge.second);
        backward_edges_[edge.second].push_back(edge.first);
    }
}

void Architecture::print_to_graph(string filename){
    filename = filename.substr(0, filename.find_last_of('.'));

    ofstream out_file(filename+".dot");
    out_file << "graph \"" << label_ << "\" {\n";
    for(auto m: nodes_){
        out_file << m.id_ << " [label=\"" << m.label_ << "\"";
        if(m.type_ == DISPENSER){
            out_file << ", shape=box, color=green";
        }else if(m.type_ == MIXER){
            out_file << ", shape=polygon, sides=4, skew=.5, color=yellow";
        }else if(m.type_ == SINK){
            out_file << ", shape=triangle, color=lightblue";
        }
        out_file << "]\n";
    }
    for(auto e: edges_){
        out_file << e.first << " -- " << e.second << endl;
    }
    out_file << "}" << endl;
    out_file.close();

    char cmd[200];
    sprintf(cmd, "dot -Tpng -o %s %s", (filename+".png").c_str(), (filename+".dot").c_str());
    system(cmd);
}