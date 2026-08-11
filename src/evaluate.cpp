#include <evaluate.h>
#include <table.h>
/*
xxxbbbbb - bbbbbbbb - cdhsrrrr - xxpppppp
b is the bit turned on based on rank
cdhs is the bit turned on based on suit(from left to right idx, 0 is hearts, 1 is diamonds, 2 is spades, 3 is clubs)
r is the rank
p is the prime number based on the rank.

My first thought was to just add to the int at the appropriate position, but that might be annoying to handle. 
My next thought was to just use bitmasking with bitwise AND & and bitwise OR |. 
*/

int cardToInt(std::string card){
    if(card.length() != 2){
        return -1;
    }
    int res = 0;
    char rank = card[0], suit = card[1];
    //Flips the suit bit(cdhs above)
    switch(suit){
        case 'H':
            res = res | 0b1000'0000'0000'0000;
            break;
        case 'D':
            res = res | 0b0100'0000'0000'0000;
            break;
        case 'S':
            res = res | 0b0010'0000'0000'0000;
            break;
        case 'C':
            res = res | 0b0001'0000'0000'0000;
            break;
        default:
            return -1;
    }
    //Set rank bit, prime number rank bits, and the rank number bit(b, r, and p above)
    switch(rank){
        case '2':
            res = res | 0b0000'0000'0000'0001'0000'0010'0000'0010;
            break;
        case '3':
            res = res | 0b0000'0000'0000'0010'0000'0011'0000'0011;
            break;
        case '4':
            res = res | 0b0000'0000'0000'0100'0000'0100'0000'0101;
            break;
        case '5':
            res = res | 0b0000'0000'0000'1000'0000'0101'0000'0111;
            break;
        case '6':
            res = res | 0b0000'0000'0001'0000'0000'0110'0000'1011;
            break;
        case '7':
            res = res | 0b0000'0000'0010'0000'0000'0111'0000'1101;
            break;
        case '8':
            res = res | 0b0000'0000'0100'0000'0000'1000'0001'0001;
            break;
        case '9':
            res = res | 0b0000'0000'1000'0000'0000'1001'0001'0011;
            break;
        case 'T':
            res = res | 0b0000'0001'0000'0000'0000'1010'0001'0111;
            break;
        case 'J':
            res = res | 0b0000'0010'0000'0000'0000'1011'0001'1101;
            break;
        case 'Q':
            res = res | 0b0000'0100'0000'0000'0000'1100'0001'1111;
            break;
        case 'K':
            res = res | 0b0000'1000'0000'0000'0000'1101'0010'0101;
            break;
        case 'A':
            res = res | 0b0001'0000'0000'0000'0000'1110'0010'1001;
            break;
        default:
            return -1;
    }
    return res;
}

std::vector<std::uint32_t> cardsToInts(std::vector<std::string>& cards){
    std::vector<std::uint32_t> res;
    for(std::string& card : cards){
        res.push_back(cardToInt(card));
    }
    return res;
}


int primeCombination(std::uint32_t c1, std::uint32_t c2, 
    std::uint32_t c3, std::uint32_t c4, std::uint32_t c5){
        return (c1 & 0xFF) * (c2 & 0xFF) * (c3 & 0xFF) * (c4 & 0xFF) * (c5 & 0xFF);
    }

int evaluate(std::uint32_t c1, std::uint32_t c2, 
    std::uint32_t c3, std::uint32_t c4, std::uint32_t c5){
    int flush = c1 & c2 & c3 & c4 & c5 & 0xF000;
    int q = (c1 | c2 | c3 | c4 | c5) >> 16;
    int rank;
    if(flush){
        rank = flushes[q];
    } else if(unique5[q]){
        rank = unique5[q];
    } else {
        int key = primeCombination(c1, c2, c3, c4, c5);
        rank = groups.at(key);
    }
    return rank;
}