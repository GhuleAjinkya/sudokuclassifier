#include "Grid.h"
#include <bitset>
#include <iostream>
#include <set>
using namespace std;
Grid::Grid() {
    for (auto& row : values) row.fill(0);
    for (auto& row : candidates) row.fill(FULL_MASK);
}

Grid Grid::fromString(const string& s) {
    Grid g;
    for (int i = 0; i < SIZE * SIZE; ++i) {
        char c = s[i];
        int row = i / SIZE;
        int col = i % SIZE;
        if (c >= '1' && c <= '9') {
            g.values[row][col] = c - '0';
        } else {
            g.values[row][col] = 0; 
        }
    }
    g.recomputeCandidates();
    return g;
}

string Grid::toString() const {
    string s;
    s.reserve(SIZE * SIZE);
    for (int r = 0; r < SIZE; ++r)
        for (int c = 0; c < SIZE; ++c)
            s += static_cast<char>('0' + values[r][c]);
    return s;
}

void Grid::print() const {
    for (int r = 0; r < SIZE; ++r) {
        if (r > 0 && r % BOX == 0) cout << "---------------------\n";
        for (int c = 0; c < SIZE; ++c) {
            if (c > 0 && c % BOX == 0) cout << "| ";
            cout << (values[r][c] == 0 ? '0' : static_cast<char>('0' + values[r][c])) << ' ';
        }
        cout << "\n";
    }
}

bool Grid::isSolved() const {
    for (int r = 0; r < SIZE; ++r)
        for (int c = 0; c < SIZE; ++c)
            if (values[r][c] == 0) return false;
    return true;
}

bool Grid::place(int row, int col, int digit) {
    uint16_t bit = static_cast<uint16_t>(1u << (digit - 1));
    if (!(candidates[row][col] & bit)) return false; 

    values[row][col] = digit;
    candidates[row][col] = 0;

    for (auto& [pr, pc] : peersOf(row, col)) {
        eliminateCandidate(pr, pc, digit);
    }
    return true;
}

bool Grid::eliminateCandidate(int row, int col, int digit) {
    if (values[row][col] != 0) return false; 
    uint16_t bit = static_cast<uint16_t>(1u << (digit - 1));
    if (!(candidates[row][col] & bit)) return false;
    candidates[row][col] &= ~bit;
    return true;
}

void Grid::recomputeCandidates() {
    for (int r = 0; r < SIZE; ++r)
        for (int c = 0; c < SIZE; ++c)
            candidates[r][c] = (values[r][c] == 0) ? FULL_MASK : 0;

    for (int r = 0; r < SIZE; ++r) {
        for (int c = 0; c < SIZE; ++c) {
            if (values[r][c] == 0) continue;
            int digit = values[r][c];
            for (auto& [pr, pc] : peersOf(r, c)) {
                eliminateCandidate(pr, pc, digit);
            }
        }
    }
}

int Grid::boxOf(int row, int col) {
    return (row / BOX) * BOX + (col / BOX);
}

vector<Cell> Grid::cellsInRow(int row) {
    vector<Cell> out;
    out.reserve(SIZE);
    for (int c = 0; c < SIZE; ++c) out.emplace_back(row, c);
    return out;
}

vector<Cell> Grid::cellsInCol(int col) {
    vector<Cell> out;
    out.reserve(SIZE);
    for (int r = 0; r < SIZE; ++r) out.emplace_back(r, col);
    return out;
}

vector<Cell> Grid::cellsInBox(int boxId) {
    vector<Cell> out;
    out.reserve(SIZE);
    int baseRow = (boxId / BOX) * BOX;
    int baseCol = (boxId % BOX) * BOX;
    for (int r = baseRow; r < baseRow + BOX; ++r)
        for (int c = baseCol; c < baseCol + BOX; ++c)
            out.emplace_back(r, c);
    return out;
}

vector<Cell> Grid::peersOf(int row, int col) {
    set<Cell> seen;
    for (auto& cell : cellsInRow(row)) seen.insert(cell);
    for (auto& cell : cellsInCol(col)) seen.insert(cell);
    for (auto& cell : cellsInBox(boxOf(row, col))) seen.insert(cell);
    seen.erase({row, col});
    return vector<Cell>(seen.begin(), seen.end());
}

int Grid::popcount(uint16_t mask) {
    return bitset<16>(mask).count();
}

int Grid::lowestSetDigit(uint16_t mask) {
    for (int d = 1; d <= SIZE; ++d) {
        if (mask & (1u << (d - 1))) return d;
    }
    return 0;
}
