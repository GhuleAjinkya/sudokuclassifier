#pragma once
#include "Techniques.h"

// ---------------- Tier 3: Advanced ----------------
// All of these only eliminate candidates; placements still come from Tier 1
// once an elimination unlocks a new single.

// X-Wing / Swordfish / Jellyfish, parameterized by fish size (2, 3, 4).
// If a digit's candidates in `size` base lines (rows or columns) are confined
// to the same `size` cover lines, the digit can be removed from every other
// cell of those cover lines.
class Fish : public Technique {
    int size_;
    std::string label_;
public:
    explicit Fish(int size);
    std::string name() const override { return label_; }
    bool apply(Grid& grid, std::vector<PlacementLog>& log) override;
};

// XY-Wing: pivot {A,B}, pincers {A,C} and {B,C}. Any cell that sees both
// pincers cannot be C.
class XYWing : public Technique {
public:
    std::string name() const override { return "XY-Wing"; }
    bool apply(Grid& grid, std::vector<PlacementLog>& log) override;
};

// XYZ-Wing: pivot {X,Y,Z}, pincers {X,Z} and {Y,Z}. Any cell that sees the
// pivot and both pincers cannot be Z.
class XYZWing : public Technique {
public:
    std::string name() const override { return "XYZ-Wing"; }
    bool apply(Grid& grid, std::vector<PlacementLog>& log) override;
};

// Skyscraper: two parallel lines each holding a digit in exactly two cells,
// sharing one end-column/row. The two free ends can't both be false, so any
// cell seeing both free ends loses the digit.
class Skyscraper : public Technique {
public:
    std::string name() const override { return "Skyscraper"; }
    bool apply(Grid& grid, std::vector<PlacementLog>& log) override;
};

// 2-String Kite: a row and a column that each hold a digit in exactly two
// cells, linked through one box. Any cell seeing both free ends loses the digit.
class TwoStringKite : public Technique {
public:
    std::string name() const override { return "2-String Kite"; }
    bool apply(Grid& grid, std::vector<PlacementLog>& log) override;
};

// Simple Coloring (single-digit chains over conjugate pairs).
// Rule "wrap": two same-colored cells see each other -> that color is false.
// Rule "trap": a cell sees both colors -> it can't hold the digit.
class SimpleColoring : public Technique {
public:
    std::string name() const override { return "Simple Coloring"; }
    bool apply(Grid& grid, std::vector<PlacementLog>& log) override;
};
