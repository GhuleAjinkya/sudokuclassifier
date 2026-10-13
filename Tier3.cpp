#include "Tier3.h"
#include <array>
#include <queue>

namespace {

inline uint16_t digitBit(int digit) { return static_cast<uint16_t>(1u << (digit - 1)); }

inline bool hasCandidate(const Grid& g, Cell cell, int digit) {
    return g.values[cell.first][cell.second] == 0 &&
           (g.candidates[cell.first][cell.second] & digitBit(digit));
}

// Maps (line, pos) -> cell. orient 0: lines are rows, pos is the column.
// orient 1: lines are columns, pos is the row.
inline Cell lineCell(int orient, int line, int pos) {
    return orient == 0 ? Cell{line, pos} : Cell{pos, line};
}

// Bitmask (over pos 0..8) of where `digit` is still a candidate within a line.
uint16_t linePositions(const Grid& g, int orient, int line, int digit) {
    uint16_t m = 0;
    for (int pos = 0; pos < SIZE; ++pos)
        if (hasCandidate(g, lineCell(orient, line, pos), digit)) m |= static_cast<uint16_t>(1u << pos);
    return m;
}

// Eliminates `digit` from every cell that sees all of `anchors`.
bool eliminateSeenByAll(Grid& g, const std::vector<Cell>& anchors, int digit) {
    bool changed = false;
    for (int r = 0; r < SIZE; ++r) {
        for (int c = 0; c < SIZE; ++c) {
            Cell x{r, c};
            if (!hasCandidate(g, x, digit)) continue;
            bool seesAll = true;
            for (auto& a : anchors)
                if (!Grid::sees(x, a)) { seesAll = false; break; }
            if (seesAll && g.eliminateCandidate(r, c, digit)) changed = true;
        }
    }
    return changed;
}

} // namespace

// ---------------- Fish ----------------

Fish::Fish(int size) : size_(size) {
    switch (size) {
        case 2: label_ = "X-Wing"; break;
        case 3: label_ = "Swordfish"; break;
        case 4: label_ = "Jellyfish"; break;
        default: label_ = "Fish(" + std::to_string(size) + ")";
    }
}

bool Fish::apply(Grid& grid, std::vector<PlacementLog>& log) {
    (void)log;
    bool changed = false;

    for (int digit = 1; digit <= SIZE; ++digit) {
        for (int orient = 0; orient < 2; ++orient) {
            std::array<uint16_t, SIZE> posMask{};
            std::vector<int> eligible; // base lines with 2..size candidate positions
            for (int line = 0; line < SIZE; ++line) {
                posMask[line] = linePositions(grid, orient, line, digit);
                int n = Grid::popcount(posMask[line]);
                if (n >= 2 && n <= size_) eligible.push_back(line);
            }
            if (static_cast<int>(eligible.size()) < size_) continue;

            for (auto& combo : combinations(static_cast<int>(eligible.size()), size_)) {
                uint16_t unionMask = 0;
                std::array<bool, SIZE> inBase{};
                for (int idx : combo) {
                    unionMask |= posMask[eligible[idx]];
                    inBase[eligible[idx]] = true;
                }
                if (Grid::popcount(unionMask) != size_) continue; // base lines not confined to `size` cover lines

                for (int line = 0; line < SIZE; ++line) {
                    if (inBase[line]) continue;
                    for (int pos = 0; pos < SIZE; ++pos) {
                        if (!(unionMask & (1u << pos))) continue;
                        auto [r, c] = lineCell(orient, line, pos);
                        if (grid.eliminateCandidate(r, c, digit)) changed = true;
                    }
                }
            }
        }
    }
    return changed;
}

// ---------------- XY-Wing ----------------

bool XYWing::apply(Grid& grid, std::vector<PlacementLog>& log) {
    (void)log;
    bool changed = false;

    for (int r = 0; r < SIZE; ++r) {
        for (int c = 0; c < SIZE; ++c) {
            if (grid.values[r][c] != 0) continue;
            uint16_t pivotMask = grid.candidates[r][c];
            if (Grid::popcount(pivotMask) != 2) continue;
            Cell pivot{r, c};

            // Bi-value peers sharing exactly one digit with the pivot.
            std::vector<Cell> pincers;
            for (auto& p : Grid::peersOf(r, c)) {
                if (grid.values[p.first][p.second] != 0) continue;
                uint16_t m = grid.candidates[p.first][p.second];
                if (Grid::popcount(m) == 2 && Grid::popcount(m & pivotMask) == 1) pincers.push_back(p);
            }

            for (size_t i = 0; i < pincers.size(); ++i) {
                for (size_t j = i + 1; j < pincers.size(); ++j) {
                    uint16_t m1 = grid.candidates[pincers[i].first][pincers[i].second];
                    uint16_t m2 = grid.candidates[pincers[j].first][pincers[j].second];
                    if ((m1 & pivotMask) == (m2 & pivotMask)) continue; // pincers must pair with different pivot digits
                    uint16_t common = m1 & m2;
                    if (Grid::popcount(common) != 1 || (common & pivotMask)) continue;

                    int z = Grid::lowestSetDigit(common);
                    if (eliminateSeenByAll(grid, {pincers[i], pincers[j]}, z)) changed = true;
                }
            }
        }
    }
    return changed;
}

// ---------------- XYZ-Wing ----------------

bool XYZWing::apply(Grid& grid, std::vector<PlacementLog>& log) {
    (void)log;
    bool changed = false;

    for (int r = 0; r < SIZE; ++r) {
        for (int c = 0; c < SIZE; ++c) {
            if (grid.values[r][c] != 0) continue;
            uint16_t pivotMask = grid.candidates[r][c];
            if (Grid::popcount(pivotMask) != 3) continue;
            Cell pivot{r, c};

            // Bi-value peers whose candidates are a subset of the pivot's.
            std::vector<Cell> pincers;
            for (auto& p : Grid::peersOf(r, c)) {
                if (grid.values[p.first][p.second] != 0) continue;
                uint16_t m = grid.candidates[p.first][p.second];
                if (Grid::popcount(m) == 2 && (m & ~pivotMask) == 0) pincers.push_back(p);
            }

            for (size_t i = 0; i < pincers.size(); ++i) {
                for (size_t j = i + 1; j < pincers.size(); ++j) {
                    uint16_t m1 = grid.candidates[pincers[i].first][pincers[i].second];
                    uint16_t m2 = grid.candidates[pincers[j].first][pincers[j].second];
                    if (m1 == m2 || (m1 | m2) != pivotMask) continue;
                    uint16_t common = m1 & m2; // exactly one digit: Z
                    if (Grid::popcount(common) != 1) continue;

                    int z = Grid::lowestSetDigit(common);
                    if (eliminateSeenByAll(grid, {pivot, pincers[i], pincers[j]}, z)) changed = true;
                }
            }
        }
    }
    return changed;
}

// ---------------- Skyscraper ----------------

bool Skyscraper::apply(Grid& grid, std::vector<PlacementLog>& log) {
    (void)log;
    bool changed = false;

    for (int digit = 1; digit <= SIZE; ++digit) {
        for (int orient = 0; orient < 2; ++orient) {
            std::vector<std::pair<int, uint16_t>> strongLines; // lines with exactly 2 candidate cells
            for (int line = 0; line < SIZE; ++line) {
                uint16_t m = linePositions(grid, orient, line, digit);
                if (Grid::popcount(m) == 2) strongLines.push_back({line, m});
            }

            for (size_t a = 0; a < strongLines.size(); ++a) {
                for (size_t b = a + 1; b < strongLines.size(); ++b) {
                    uint16_t shared = strongLines[a].second & strongLines[b].second;
                    if (Grid::popcount(shared) != 1) continue; // need exactly one common base position

                    uint16_t topA = strongLines[a].second & ~shared;
                    uint16_t topB = strongLines[b].second & ~shared;
                    Cell endA = lineCell(orient, strongLines[a].first, Grid::lowestSetDigit(topA) - 1);
                    Cell endB = lineCell(orient, strongLines[b].first, Grid::lowestSetDigit(topB) - 1);

                    if (eliminateSeenByAll(grid, {endA, endB}, digit)) changed = true;
                }
            }
        }
    }
    return changed;
}

// ---------------- 2-String Kite ----------------

bool TwoStringKite::apply(Grid& grid, std::vector<PlacementLog>& log) {
    (void)log;
    bool changed = false;

    for (int digit = 1; digit <= SIZE; ++digit) {
        for (int row = 0; row < SIZE; ++row) {
            uint16_t rowMask = linePositions(grid, 0, row, digit);
            if (Grid::popcount(rowMask) != 2) continue;
            std::array<Cell, 2> rowCells{};
            int n = 0;
            for (int pos = 0; pos < SIZE; ++pos)
                if (rowMask & (1u << pos)) rowCells[n++] = {row, pos};

            for (int col = 0; col < SIZE; ++col) {
                uint16_t colMask = linePositions(grid, 1, col, digit);
                if (Grid::popcount(colMask) != 2) continue;
                std::array<Cell, 2> colCells{};
                int m = 0;
                for (int pos = 0; pos < SIZE; ++pos)
                    if (colMask & (1u << pos)) colCells[m++] = {pos, col};

                for (int i = 0; i < 2; ++i) {
                    for (int j = 0; j < 2; ++j) {
                        Cell r1 = rowCells[i], r2 = rowCells[1 - i];
                        Cell k1 = colCells[j], k2 = colCells[1 - j];
                        if (r1 == k1 || r2 == k2) continue;
                        if (Grid::boxOf(r1.first, r1.second) != Grid::boxOf(k1.first, k1.second)) continue;

                        // r1 and k1 can't both be true (same box), so r2 or k2 must be.
                        if (eliminateSeenByAll(grid, {r2, k2}, digit)) changed = true;
                    }
                }
            }
        }
    }
    return changed;
}

// ---------------- Simple Coloring ----------------

bool SimpleColoring::apply(Grid& grid, std::vector<PlacementLog>& log) {
    (void)log;
    bool changed = false;
    auto idx = [](Cell c) { return c.first * SIZE + c.second; };

    for (int digit = 1; digit <= SIZE; ++digit) {
        // Conjugate links: units where the digit has exactly two candidate cells.
        std::array<std::vector<int>, SIZE * SIZE> links;
        for (auto& unit : allUnits()) {
            std::vector<Cell> hosts;
            for (auto& cell : unit)
                if (hasCandidate(grid, cell, digit)) hosts.push_back(cell);
            if (hosts.size() == 2) {
                links[idx(hosts[0])].push_back(idx(hosts[1]));
                links[idx(hosts[1])].push_back(idx(hosts[0]));
            }
        }

        std::array<int, SIZE * SIZE> color;
        color.fill(-1);

        for (int start = 0; start < SIZE * SIZE; ++start) {
            if (color[start] != -1 || links[start].empty()) continue;

            // BFS two-coloring of this chain component.
            std::vector<int> members;
            std::queue<int> q;
            color[start] = 0;
            q.push(start);
            bool consistent = true;
            while (!q.empty()) {
                int cur = q.front(); q.pop();
                members.push_back(cur);
                for (int nxt : links[cur]) {
                    if (color[nxt] == -1) { color[nxt] = 1 - color[cur]; q.push(nxt); }
                    else if (color[nxt] == color[cur]) consistent = false;
                }
            }
            if (!consistent) continue; // odd cycle => inconsistent candidates; don't reason from it

            std::array<std::vector<Cell>, 2> byColor;
            for (int m : members) byColor[color[m]].push_back({m / SIZE, m % SIZE});

            // Rule "wrap": two same-colored cells that see each other => that color is false.
            bool wrapped = false;
            for (int k = 0; k < 2 && !wrapped; ++k) {
                auto& cells = byColor[k];
                for (size_t a = 0; a < cells.size() && !wrapped; ++a)
                    for (size_t b = a + 1; b < cells.size() && !wrapped; ++b)
                        if (Grid::sees(cells[a], cells[b])) wrapped = true;
                if (wrapped) {
                    for (auto& cell : cells)
                        if (grid.eliminateCandidate(cell.first, cell.second, digit)) changed = true;
                }
            }
            if (wrapped) continue;

            // Rule "trap": a cell outside the chain that sees both colors can't hold the digit.
            for (int r = 0; r < SIZE; ++r) {
                for (int c = 0; c < SIZE; ++c) {
                    Cell x{r, c};
                    if (color[idx(x)] != -1 || !hasCandidate(grid, x, digit)) continue;
                    bool seesColor[2] = {false, false};
                    for (int k = 0; k < 2; ++k)
                        for (auto& cell : byColor[k])
                            if (Grid::sees(x, cell)) { seesColor[k] = true; break; }
                    if (seesColor[0] && seesColor[1] && grid.eliminateCandidate(r, c, digit)) changed = true;
                }
            }
        }
    }
    return changed;
}
