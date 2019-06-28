#include <iostream>
#include <cstdio>
#include "z3++.h"

using namespace z3;
using namespace std;


int main(){
    context c;

    expr x = c.bool_val(false);
    expr y = c.bool_const("y");
    expr_vector v(c);
    for(int i = 0; i < 3; i++){
        char name[10];
        sprintf(name, "v%d", i);
        v.push_back(c.bool_const(name));
    }
    
    optimize s(c);
    s.add(x||y);
    s.add(atmost(v, 2) && atleast(v, 2));
    //s.add(!mk_or(v));
    s.add(v[0] == v[2]);
    cout << s << endl;
    s.check();

    cout << "Model: \n";
    model m = s.get_model();
    cout << m << endl;
    cout << "x " << m.eval(x) << endl;
    cout << "y " << m.eval(y) << endl;

    for(auto vi: v){
        cout << m.eval(vi) << endl;
    }

    return 0;
}
