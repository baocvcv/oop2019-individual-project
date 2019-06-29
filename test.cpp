#include <iostream>
#include <cstdio>
#include "z3++.h"

using namespace std;
using namespace z3;

int main(){
    context ctx;

    solver s(ctx);

    expr x = ctx.bool_const("x");
    expr y = ctx.bool_const("y");
    s.add(x&&!y);

    expr_vector v(ctx);
    for(int i = 0; i < 3; i++){
        char name[40];
        sprintf(name, "v%d", i);
        v.push_back(ctx.bool_const(name));
    }
    s.add(atmost(v, 1));

    s.check();

    cout << s;

    model m = s.get_model();
    if(eq(m.eval(y), ctx.bool_val(false))){
        cout << "works" << endl;
    }else{
        cout << "fails" << endl;
    }
    return 0;
}