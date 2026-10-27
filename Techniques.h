#pragma once
#include "Grid.h"
#include <string>
#include <vector>

// Replaces a full "solvability certificate" for now: just records what was
// placed, where, and which technique placed it.
struct PlacementLog {
    int row;     // 0-based
    int col;     // 0-based
    int digit;
    std::string method;
};

class Technique {
public:
    virtual ~Technique() = default;
    virtual std::string name() const = 0;

    // Finds and applies exactly ONE pattern (one placement, or one pattern's
    // worth of eliminations), then returns true. Returns false if the technique
    // finds nothing. One pattern per call is what makes step counts meaningful.
    virtual bool apply(Grid& grid, std::vector<PlacementLog>& log) = 0;
};

// ---------------- Tier 1: Basic ----------------

// A cell with exactly one remaining candidate must hold that digit.
class NakedSingle : public Technique {
public:
    std::string name() const override { return "Naked Single"; }
    bool apply(Grid& grid, std::vector<PlacementLog>& log) override;
};

// A digit that fits in only one cell within a row/col/box must go there.
class HiddenSingle : public Technique {
public:
    std::string name() const override { return "Hidden Single"; }
    bool apply(Grid& grid, std::vector<PlacementLog>& log) override;
};

// ---------------- Tier 2: Intermediate ----------------

// Naked Pair / Triple / Quad, parameterized by subset size (2, 3, or 4).
// If `size` cells in a unit share a candidate pool of exactly `size` digits,
// those digits can be eliminated from every other cell in the unit.
class NakedSubset : public Technique {
    int size_;
    std::string label_;
public:
    explicit NakedSubset(int size);
    std::string name() const override { return label_; }
    bool apply(Grid& grid, std::vector<PlacementLog>& log) override;
};

// Hidden Pair / Triple / Quad, parameterized by subset size (2, 3, or 4).
// If `size` digits are confined to the same `size` cells within a unit,
// every other candidate can be stripped from those cells.
class HiddenSubset : public Technique {
    int size_;
    std::string label_;
public:
    explicit HiddenSubset(int size);
    std::string name() const override { return label_; }
    bool apply(Grid& grid, std::vector<PlacementLog>& log) override;
};

// Pointing Pair/Triple (box -> line): if a digit's candidates within a box
// are confined to a single row or column, it can be eliminated from the
// rest of that row/column outside the box.
class PointingPair : public Technique {
public:
    std::string name() const override { return "Pointing Pair/Triple (Box -> Line)"; }
    bool apply(Grid& grid, std::vector<PlacementLog>& log) override;
};

// Claiming / Line-Box reduction: if a digit's candidates within a row or
// column are confined to a single box, it can be eliminated from the rest
// of that box outside the row/column.
class ClaimingPair : public Technique {
public:
    std::string name() const override { return "Claiming (Line -> Box)"; }
    bool apply(Grid& grid, std::vector<PlacementLog>& log) override;
};

// All 27 units of the board: 9 rows, 9 columns, 9 boxes.
const std::vector<std::vector<Cell>>& allUnits();  // built once, cached

// All k-sized index combinations of [0, n). Shared by Tier 2 and Tier 3.
const std::vector<std::vector<int>>& combinations(int n, int k);  // cached (not thread-safe)
