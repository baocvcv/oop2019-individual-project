#include "Architecture.h"
#include <iostream>

int main(){
    Architecture g;
    g.build_from_file("test.txt");
    g.print_to_graph("test_out.txt");
}