#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

// ---- Fixed to standard 3x3-box / 9x9 Sudoku for now ----
constexpr int SIZE = 9;
constexpr int BOX  = 3;
constexpr uint16_t FULL_MASK = 0x1FF; // bits 0..8 represent digits 1..9

using Cell = std::pair<int, int>; // (row, col)

struct Grid {
    std::array<std::array<int, SIZE>, SIZE> values{};        // 0 = empty
    std::array<std::array<uint16_t, SIZE>, SIZE> candidates{}; // bitmask of remaining possibilities

    Grid();

    // Parses an 81-character string ('1'-'9' for givens, '0' or '.' for blanks)
    static Grid fromString(const std::string& s);

    std::string toString() const;
    void print() const;

    bool isSolved() const;

    // Places `digit` at (row, col). Clears the cell's own candidates and strips
    // the digit bit from every peer's candidate set.
    // Returns false if the cell didn't actually have `digit` as a candidate
    // (signals a contradiction / bug in the calling technique).
    bool place(int row, int col, int digit);

    // Removes `digit` from the candidate mask at (row, col).
    // Returns true if the bit was actually present (i.e. something changed).
    bool eliminateCandidate(int row, int col, int digit);

    // Rebuilds every candidate mask from scratch based on `values`.
    void recomputeCandidates();

    // ---- Unit helpers (row / column / 3x3 box) ----
    static int boxOf(int row, int col);                 // box id 0..8
    static std::vector<Cell> cellsInRow(int row);
    static std::vector<Cell> cellsInCol(int col);
    static std::vector<Cell> cellsInBox(int boxId);
    static std::vector<Cell> peersOf(int row, int col);  // 20 unique peers, excludes self
    static bool sees(Cell a, Cell b);                    // same row/col/box and not the same cell

    static int popcount(uint16_t mask);
    static int lowestSetDigit(uint16_t mask); // 1..9, assumes exactly one bit set
};
