#pragma once
#include <string>
#include <vector>

//card --> binary function. The return type should be of type int
int cardToInt(std::string card);

std::vector<int> cardsToInts(std::vector<std::string>& cards);

int primeCombination(std::vector<int>& cards);

int evaluate(std::vector<std::string>& cards);