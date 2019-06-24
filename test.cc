#include <iostream>
#include "z3++.h"

using namespace z3;
using namespace std;

void func(context &c){
    expr x = c.bool_const("x");
    expr y = c.bool_const("y");
    
    return;
};

int main(){
    context c;

    expr x = c.bool_const("x");
    expr y = c.bool_const("y");
    
    func(c);
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
