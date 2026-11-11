#pragma once
#include "Grid.h"
#include <string>
#include <vector>

// One candidate removed from one cell by an elimination step (or, in `evidence`, one that had to be absent).
struct Removal {
    int row;     // 0-based
    int col;     // 0-based
    int digit;
};

// One solving step, in the order the solver took it. BOTH placements and eliminations are
// logged, so replaying the log from the givens reproduces the solver's candidate grid exactly.
//
// Why this matters: only Naked/Hidden Singles ever place a digit. Pairs, pointing, X-Wing, XY-Wing and
// the rest only remove candidates. A digit "placed by an XY-Wing" is really an XY-Wing elimination that
// turned some cell into a single, followed by a single doing the placing. To print the real reason, each
// step also records which earlier elimination steps it relies on (see Reasons.h).
struct StepLog {
    enum class Kind { Placement, Elimination };

    Kind kind = Kind::Placement;
    std::string method;                  // the technique that performed this step
    std::string detail;                  // optional extra text, e.g. "in row 3" or "color wrap"

    // Placement: where and what.
    int row = -1;                        // 0-based
    int col = -1;                        // 0-based
    int digit = 0;

    // Elimination: the pattern that justifies it and everything it removed.
    std::vector<Cell> pattern;           // cells forming the pattern (pair cells, fish corners, wings...)
    std::vector<int> patternDigits;      // the digits the pattern is about
    std::vector<Removal> removed;        // each (cell, digit) taken out of the candidates
    std::vector<Removal> evidence;       // candidates that had to be ABSENT for the pattern to hold

    // Filled in after solving by reasons::attribute():
    std::vector<int> enabledBy;          // indices (in the log) of the elimination steps this step directly relies on
    std::string rootMethod;              // hardest technique among everything it relies on ("" = plain singles only)
    std::vector<std::string> chain;      // those techniques, in the order they were applied

    static StepLog placement(std::string method, int row, int col, int digit, std::string detail = "") {
        StepLog s;
        s.kind = Kind::Placement;
        s.method = std::move(method);
        s.detail = std::move(detail);
        s.row = row;
        s.col = col;
        s.digit = digit;
        return s;
    }
    static StepLog elimination(std::string method, std::vector<Cell> pattern, std::vector<int> digits,
                               std::vector<Removal> removed, std::vector<Removal> evidence, std::string detail = "") {
        StepLog s;
        s.kind = Kind::Elimination;
        s.method = std::move(method);
        s.detail = std::move(detail);
        s.pattern = std::move(pattern);
        s.patternDigits = std::move(digits);
        s.removed = std::move(removed);
        s.evidence = std::move(evidence);
        return s;
    }
};
using PlacementLog = StepLog;  // the old name, kept so existing code and tests still compile

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

// Removes `digit` from the candidates at (row, col) and records it. Returns true if it was present.
inline bool removeCandidate(Grid& grid, std::vector<Removal>& removed, int row, int col, int digit) {
    if (!grid.eliminateCandidate(row, col, digit)) return false;
    removed.push_back({row, col, digit});
    return true;
}

// Records that `digit` is currently NOT a candidate in an empty cell: a fact the pattern depends on.
inline void addAbsence(const Grid& grid, std::vector<Removal>& evidence, int row, int col, int digit) {
    if (grid.values[row][col] == 0 && !(grid.candidates[row][col] & static_cast<uint16_t>(1u << (digit - 1))))
        evidence.push_back({row, col, digit});
}

// The digits (1-9) whose bits are set in a candidate mask.
std::vector<int> digitsOf(uint16_t mask);

// Name of an entry in allUnits(): index 0-8 are rows, 9-17 columns, 18-26 boxes -> "row 3", "column 5", "box 2" (1-based).
std::string unitName(int unitIndex);
