#include "Architecture.h"
#include "Module.h"
#include <fstream>
#include <iostream>
#include <string>
#include <string.h>

using namespace std;

Architecture::Architecture() {
    sink_num_ = 0;
    dispenser_num_ = 0;
}

vector<string> split(string s, char c){
    vector<string> res;
    while(s.length() > 0){
        res.push_back(s.substr(0, s.find(c)));
        s = s.substr(s.find(c));
    }
    return res;
}

void Architecture::build_from_file(const std::string &filename){
    // erase previous data
    sink_num_ = dispenser_num_ = 0;
    for(auto edges: forward_edges_){
        edges.clear();
    }
    forward_edges_.clear();
    for(auto edges: backward_edges_){
        edges.clear();
    }
    backward_edges_.clear();
    modules_.clear();

    ifstream in_file(filename);
    string line;
    getline(in_file, line);
    // get the name of the file
    label_ = line.substr(line.find('('), line.find(')')-line.find('(')-1); // TODO: test this

    // process line by line
    while(!in_file.eof()){
        string line;
        getline(in_file, line);
        if(line.find('(') == string::npos){
            continue;
        }

        string type = line.substr(0, line.find(' '));
        auto params = split(line.substr(line.find('('), line.find(')') - line.find('(')-1), ','); // TODO: test this
        if(params.size() < 2){
            cout << "Error reading input file: " << filename << endl;
        }

        if(type == "EDGE"){
            int u = stoi(params[0]);
            int v = stoi(params[1]);
            edges.push_back(make_pair(u-1, v-1));
        }else if(type == "NODE"){
            Module m;
            m.id_ = stoi(params[0]) - 1;
            string type_module = params[1];
            if(type_module == "MIX"){
                m.type_ = MIXER;
                m.drops_ = stoi(params[2]);
                m.time_ = stoi(params[3]);
                m.label_ = params[4];
            }else if(type_module == "DISPENSE"){
                m.type_ = DISPENSER;
                m.fluid_type_ = params[2];
                m.volume_ = stoi(params[3]);
                m.label_ = params[4];
            }else if(type_module == "OUPTUT"){
                m.type_ = SINK;
                m.sink_name_ = params[2];
                m.label_ = params[3];
                sink_num_++;
            }else if(type_module == "DETECT"){
                m.type_ = DETECTOR;
                m.drops_ = stoi(params[2]);
                m.time_ = stoi(params[3]);
                m.label_ = stoi(params[4]);
            }else{
                cout << "Module type not yet supported!" << endl;
            }
            modules_.push_back(m);
        }else if(type == "TIME"){
            time_limit_ = stoi(params[0]);
        }else if(type == "SIZE"){
            width_limit_ = stoi(params[0]);
            height_limit_ = stoi(params[1]);
        }else if(type == "MOD"){
            int id = stoi(params[0]) - 1;
            switch(modules_[id].type_){
                case MIXER:
                    modules_[id].w = stoi(params[1]);
                    modules_[id].h = stoi(params[2]);
                    break;
            }
        }
    }
    in_file.close();

    // prepare edge lists
    forward_edges_.assign(modules_.size(), vector<int>());
    backward_edges_.assign(modules_.size(), vector<int>());
    for(auto edge: edges){
        forward_edges_[edge.first].push_back(edge.second);
        backward_edges_[edge.second].push_back(edge.first);
    }
}

void Architecture::print_to_graph(const string &filename){
    ofstream out_file(filename);
    out_file << "graph \"" << label_ << "\" {\n";
    for(auto m: modules_){
        out_file << m.id_ << " [label=\"" << m.label_ << m.id_ << "\"]\n";
    }
    for(auto e: edges){
        out_file << e.first << " -- " << e.second << endl;
    }
    out_file << "}" << endl;
    out_file.close();
}