#include <table.h>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <evaluate.h>
/*
We can use the csv file added in the repo to make our precomputed tables.
SF or F means put it in the flush table.
S or HC means put it in the unique5 table.
Anything else we toss in our hashmap.
*/

//Dumps one of the flat tables as a pasteable array definition, 16 entries per line
static void printTable(const std::string& name, const std::vector<int>& table){
    std::cout << "inline constexpr int " << name << "[" << table.size() << "] = {\n";
    for(size_t i = 0; i < table.size(); ++i){
        if(i % 16 == 0){
            std::cout << "    ";
        }
        std::cout << table[i];
        if(i + 1 < table.size()){
            std::cout << ",";
        }
        if(i % 16 == 15 || i + 1 == table.size()){
            std::cout << "\n";
        } else {
            std::cout << " ";
        }
    }
    std::cout << "};\n\n";
}

//Same idea for the hashmap, one key/value pair per line
static void printGroups(const std::string& name, const std::unordered_map<int, int>& groups){
    std::cout << "inline const std::unordered_map<int, int> " << name << " = {\n";
    size_t i = 0;
    for(const auto& [primeProduct, rank] : groups){
        std::cout << "    {" << primeProduct << ", " << rank << "}";
        if(i + 1 < groups.size()){
            std::cout << ",";
        }
        std::cout << "\n";
        ++i;
    }
    std::cout << "};\n";
}

void getTables(){
    //Lets start looping through our csv then
    std::ifstream file("../poker_7462_rankings.csv");
    if(!file.is_open()){
        std::cerr << "File is unable to be opened" << std::endl;
    }
    std::string line;
    std::getline(file, line);
    //Lets initialize our tables(flushes, unique5, and groups)
    std::vector<int> flushes(7937);
    std::vector<int> unique5(7937);
    std::unordered_map<int, int> groups;
    while(std::getline(file, line)){
        std::stringstream ss(line);
        std::string cell;
        std::vector<std::string> row;
        while(std::getline(ss, cell, ',')){
            row.push_back(cell);
        }
        std::string card = row[1];
        std::vector<int> cardList;
        for(size_t i = 0; i < card.length(); ++i){
            cardList.push_back(cardToInt(card[i] + "D"));
        }
        int q = (cardList[0] | cardList[1] | cardList[2] | cardList[3] | cardList[4] | cardList[5]) >> 16;
        int rank = std::stoi(row[0]);
        if(row[2] == "SF" || row[2] == "F"){
            flushes[q] = rank;
        } else if(row[2] == "HC" || row[2] == "S"){
            unique5[q] = rank;
        } else {
            int primeProduct = primeCombination(cardList);
            groups[primeProduct] = rank;
        }
    }
    file.close();

    //Dump everything as a block that can be pasted straight into the top of table.h
    std::cout << "#include <unordered_map>\n\n";
    printTable("flushes", flushes);
    printTable("unique5", unique5);
    printGroups("groups", groups);
}