#include "Architecture.h"
#include "Solver.h"

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

    Architecture g;
    g.build_from_file(filename);

    string outputFilename = filename.substr(0, filename.find_first_of('.')) + "_out.dot";
    g.print_to_graph(outputFilename);

    context c;
    Solver solver(g, c);
    // solver.solve();
    solver.solve(5,5,10);
    // solver.solve(3,3,3);
    solver.print_solution();
    solver.save_solver("testcase/solver.txt");

    return 0;
}
