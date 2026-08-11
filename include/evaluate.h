#pragma once
#include <cstdint>
#include <string>
#include <vector>

//card --> binary function. The return type should be of type int
int cardToInt(std::string card);

std::vector<std::uint32_t> cardsToInts(std::vector<std::string>& cards);

int primeCombination(std::uint32_t c1, std::uint32_t c2, 
    std::uint32_t c3, std::uint32_t c4, std::uint32_t c5);

int evaluate(std::uint32_t c1, std::uint32_t c2, 
    std::uint32_t c3, std::uint32_t c4, std::uint32_t c5);