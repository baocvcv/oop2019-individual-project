#include "Architecture.h"
#include "Solver.h"
#include "OnePassSynth.h"

#include "z3++.h"
#include <iostream>

using namespace std;
using namespace z3;

int main(int argc, char** argv){
    string filename;
    if(argc <= 1){
        filename = "testcase/test1.txt";
    }else{
        filename = argv[1]; 
    }

/*     Architecture g;
    g.build_from_file(filename);

    string outputFilename = filename.substr(0, filename.find_first_of('.')) + "_out.dot";
    g.print_to_graph(outputFilename);

    context c;
    Solver solver(g, c);
    solver.solve();
    solver.print_solution();
 */

    OnePassSynth ops(filename);
    ops.solve();
    ops.print_solution();
    auto v = ops.get_grid();
    for(unsigned int t = 0; t < v.size(); t++){
        for(unsigned int y = 0; y < v[t].size(); y++){
            for(unsigned int x = 0; x < v[t][y].size(); x++){
                cout << v[t][y][x] << ' ';
            }
            cout << endl;
        }
        cout << endl;
    }
    cout << endl;

    auto w = ops.get_sink_dispenser_pos();
    for(unsigned int p = 0; p < w.size(); p++){
        cout << p << " type= " << w[p].type_ << " l= " << w[p].label_ << endl;
    }
    cout << endl;

    auto m = ops.get_detector_pos();
    for(int y = 0; y < m.size(); y++){
        for(int x = 0; x < m[y].size(); x++){
            if(m[y][x].first){
                cout << m[y][x].second << ' ';
            }else{
                cout << "0   ";
            }
        }
        cout << endl;
    }
    cout << endl;
    
    return 0;
}
