#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
using namespace std;
constexpr int SIZE = 9;
constexpr int BOX  = 3;
constexpr uint16_t FULL_MASK = 0x1FF;

using Cell = pair<int, int>; 

struct Grid {
    array<array<int, SIZE>, SIZE> values{};      
    array<array<uint16_t, SIZE>, SIZE> candidates{}; 

    Grid();
    static Grid fromString(const string& s);

    string toString() const;
    void print() const;

    bool isSolved() const;

    bool place(int row, int col, int digit);

    bool eliminateCandidate(int row, int col, int digit);

    void recomputeCandidates();

    static int boxOf(int row, int col);           
    static vector<Cell> cellsInRow(int row);
    static vector<Cell> cellsInCol(int col);
    static vector<Cell> cellsInBox(int boxId);
    static vector<Cell> peersOf(int row, int col);  

    static int popcount(uint16_t mask);
    static int lowestSetDigit(uint16_t mask); 
};