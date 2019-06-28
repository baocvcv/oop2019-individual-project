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

    string outputFilename = filename.substr(0, filename.find_first_of('.')) + "_out.txt";
    g.print_to_graph(outputFilename);

    context c;
    Solver solver(g, c);
    solver.solve(2,2,3);
    solver.print_solver();

    return 0;
}