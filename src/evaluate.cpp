#include <evaluate.h>
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
        case 'D':
            res = res | 0b0100'0000'0000'0000;
        case 'S':
            res = res | 0b0010'0000'0000'0000;
        case 'C':
            res = res | 0b0001'0000'0000'0000;
        default:
            return -1;
    }
    //Set rank bit, prime number rank bits, and the rank number bit(b, r, and p above)
    switch(rank){
        case '2':
            res = res | 0b0000'0000'0000'0001'0000'0010'0000'0010;
        case '3':
            res = res | 0b0000'0000'0000'0010'0000'0011'0000'0011;
        case '4':
            res = res | 0b0000'0000'0000'0100'0000'0100'0000'0101;
        case '5':
            res = res | 0b0000'0000'0000'1000'0000'0101'0000'0111;
        case '6':
            res = res | 0b0000'0000'0001'0000'0000'0110'0000'1011;
        case '7':
            res = res | 0b0000'0000'0010'0000'0000'0111'0000'1101;
        case '8':
            res = res | 0b0000'0000'0100'0000'0000'1000'0001'0001;
        case '9':
            res = res | 0b0000'0000'1000'0000'0000'1001'0001'0011;
        case '10':
            res = res | 0b0000'0001'0000'0000'0000'1010'0001'0111;
        case 'J':
            res = res | 0b0000'0010'0000'0000'0000'1011'0001'1101;
        case 'Q':
            res = res | 0b0000'0100'0000'0000'0000'1100'0001'1111;
        case 'K':
            res = res | 0b0000'1000'0000'0000'0000'1101'0010'0101;
        case 'A':
            res = res | 0b0001'0000'0000'0000'0000'1110'0010'1001;
        default:
            return -1;
    }
    return res;
}
