#include <table.h>
#include <evaluate.h>
#include <iostream>

int main(){
    // evaluate takes encoded cards, so the string form is converted here, at the
    // boundary. A simulator would do this once for the whole deck at startup.
    std::cout << evaluate(cardToInt("AH"), cardToInt("AD"), cardToInt("AS"),
                          cardToInt("AC"), cardToInt("KH")) << std::endl;
    return 0;
}
