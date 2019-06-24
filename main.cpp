#include "Architecture.h"

#include "z3++.h"
#include <iostream>

using namespace std;
using namespace z3;

int main(){
    Architecture g;
    g.build_from_file("test.txt");
    g.print_to_graph("test_out.txt");

    context c;

    expr x = c.bool_const("x");
    expr y = c.bool_const("y");
    
    solver s(c);
    s.add(x&&y);
    cout << s << endl;
    s.check();

    cout << "Model: \n";
    model m = s.get_model();
    cout << m << endl;
    cout << m.eval(x) << endl;
    cout << m.eval(y) << endl;

    return 0;
}