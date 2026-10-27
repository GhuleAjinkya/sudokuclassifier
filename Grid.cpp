#include "Grid.h"
#include <bitset>
#include <iostream>
#include <set>

Grid::Grid() {
    for (auto& row : values) row.fill(0);
    for (auto& row : candidates) row.fill(FULL_MASK);
}

Grid Grid::fromString(const std::string& s) {
    Grid g;
    for (int i = 0; i < SIZE * SIZE; ++i) {
        char c = s[i];
        int row = i / SIZE;
        int col = i % SIZE;
        if (c >= '1' && c <= '9') {
            g.values[row][col] = c - '0';
        } else {
            g.values[row][col] = 0; // '0' or '.' or anything else = blank
        }
    }
    g.recomputeCandidates();
    return g;
}

std::string Grid::toString() const {
    std::string s;
    s.reserve(SIZE * SIZE);
    for (int r = 0; r < SIZE; ++r)
        for (int c = 0; c < SIZE; ++c)
            s += static_cast<char>('0' + values[r][c]);
    return s;
}

void Grid::print() const {
    for (int r = 0; r < SIZE; ++r) {
        if (r > 0 && r % BOX == 0) std::cout << "------+-------+------\n";
        for (int c = 0; c < SIZE; ++c) {
            if (c > 0 && c % BOX == 0) std::cout << "| ";
            std::cout << (values[r][c] == 0 ? '.' : static_cast<char>('0' + values[r][c])) << ' ';
        }
        std::cout << "\n";
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
    if (!(candidates[row][col] & bit)) return false; // not a legal candidate here

    values[row][col] = digit;
    candidates[row][col] = 0;

    for (auto& [pr, pc] : peersOf(row, col)) {
        eliminateCandidate(pr, pc, digit);
    }
    return true;
}

bool Grid::eliminateCandidate(int row, int col, int digit) {
    if (values[row][col] != 0) return false; // already placed, nothing to eliminate
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

std::vector<Cell> Grid::cellsInRow(int row) {
    std::vector<Cell> out;
    out.reserve(SIZE);
    for (int c = 0; c < SIZE; ++c) out.emplace_back(row, c);
    return out;
}

std::vector<Cell> Grid::cellsInCol(int col) {
    std::vector<Cell> out;
    out.reserve(SIZE);
    for (int r = 0; r < SIZE; ++r) out.emplace_back(r, col);
    return out;
}

std::vector<Cell> Grid::cellsInBox(int boxId) {
    std::vector<Cell> out;
    out.reserve(SIZE);
    int baseRow = (boxId / BOX) * BOX;
    int baseCol = (boxId % BOX) * BOX;
    for (int r = baseRow; r < baseRow + BOX; ++r)
        for (int c = baseCol; c < baseCol + BOX; ++c)
            out.emplace_back(r, c);
    return out;
}

const std::vector<Cell>& Grid::peersOf(int row, int col) {
    // Built once; place() calls this constantly (hot path during search).
    static const std::vector<std::vector<Cell>> table = [] {
        std::vector<std::vector<Cell>> t(SIZE * SIZE);
        for (int r = 0; r < SIZE; ++r) {
            for (int c = 0; c < SIZE; ++c) {
                std::set<Cell> seen;
                for (auto& cell : cellsInRow(r)) seen.insert(cell);
                for (auto& cell : cellsInCol(c)) seen.insert(cell);
                for (auto& cell : cellsInBox(boxOf(r, c))) seen.insert(cell);
                seen.erase({r, c});
                t[r * SIZE + c] = std::vector<Cell>(seen.begin(), seen.end());
            }
        }
        return t;
    }();
    return table[row * SIZE + col];
}

bool Grid::hasDuplicateValues() const {
    auto unitHasDuplicate = [&](const std::vector<Cell>& unit) {
        uint16_t seen = 0;
        for (auto& [r, c] : unit) {
            int v = values[r][c];
            if (v == 0) continue;
            uint16_t bit = static_cast<uint16_t>(1u << (v - 1));
            if (seen & bit) return true;
            seen |= bit;
        }
        return false;
    };
    for (int i = 0; i < SIZE; ++i)
        if (unitHasDuplicate(cellsInRow(i)) || unitHasDuplicate(cellsInCol(i)) || unitHasDuplicate(cellsInBox(i)))
            return true;
    return false;
}

bool Grid::sees(Cell a, Cell b) {
    if (a == b) return false;
    return a.first == b.first || a.second == b.second ||
           boxOf(a.first, a.second) == boxOf(b.first, b.second);
}

int Grid::popcount(uint16_t mask) {
    return std::bitset<16>(mask).count();
}

int Grid::lowestSetDigit(uint16_t mask) {
    for (int d = 1; d <= SIZE; ++d) {
        if (mask & (1u << (d - 1))) return d;
    }
    return 0; // mask was empty
}
