#include <table.h>
#include <evaluate.h>
#include <iostream>

int main(){
    std::vector<std::string> cards;
    cards.push_back("AH");
    cards.push_back("AD");
    cards.push_back("AS");
    cards.push_back("AC");
    cards.push_back("KH");
    std::cout << evaluate(cards) << std::endl;
    return 0;
}
