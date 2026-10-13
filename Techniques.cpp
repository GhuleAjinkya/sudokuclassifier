#include "Techniques.h"
#include <functional>

std::vector<std::vector<Cell>> allUnits() {
    std::vector<std::vector<Cell>> units;
    units.reserve(27);
    for (int r = 0; r < SIZE; ++r) units.push_back(Grid::cellsInRow(r));
    for (int c = 0; c < SIZE; ++c) units.push_back(Grid::cellsInCol(c));
    for (int b = 0; b < SIZE; ++b) units.push_back(Grid::cellsInBox(b));
    return units;
}

// All k-sized index combinations of [0, n).
std::vector<std::vector<int>> combinations(int n, int k) {
    std::vector<std::vector<int>> out;
    std::vector<int> combo(k);
    std::function<void(int, int)> rec = [&](int start, int chosen) {
        if (chosen == k) { out.push_back(combo); return; }
        for (int i = start; i < n; ++i) {
            combo[chosen] = i;
            rec(i + 1, chosen + 1);
        }
    };
    rec(0, 0);
    return out;
}

// ---------------- Tier 1 ----------------

bool NakedSingle::apply(Grid& grid, std::vector<PlacementLog>& log) {
    bool changed = false;
    for (int r = 0; r < SIZE; ++r) {
        for (int c = 0; c < SIZE; ++c) {
            if (grid.values[r][c] != 0) continue;
            if (Grid::popcount(grid.candidates[r][c]) == 1) {
                int digit = Grid::lowestSetDigit(grid.candidates[r][c]);
                if (grid.place(r, c, digit)) {
                    log.push_back({r, c, digit, name()});
                    changed = true;
                }
            }
        }
    }
    return changed;
}

bool HiddenSingle::apply(Grid& grid, std::vector<PlacementLog>& log) {
    bool changed = false;
    for (auto& unit : allUnits()) {
        for (int digit = 1; digit <= SIZE; ++digit) {
            uint16_t bit = static_cast<uint16_t>(1u << (digit - 1));
            int candidateCount = 0;
            Cell only{-1, -1};
            for (auto& [r, c] : unit) {
                if (grid.values[r][c] == 0 && (grid.candidates[r][c] & bit)) {
                    ++candidateCount;
                    only = {r, c};
                }
            }
            if (candidateCount == 1) {
                auto [r, c] = only;
                if (grid.place(r, c, digit)) {
                    log.push_back({r, c, digit, name()});
                    changed = true;
                }
            }
        }
    }
    return changed;
}

// ---------------- Tier 2 ----------------

NakedSubset::NakedSubset(int size) : size_(size) {
    switch (size) {
        case 2: label_ = "Naked Pair"; break;
        case 3: label_ = "Naked Triple"; break;
        case 4: label_ = "Naked Quad"; break;
        default: label_ = "Naked Subset(" + std::to_string(size) + ")";
    }
}

bool NakedSubset::apply(Grid& grid, std::vector<PlacementLog>& log) {
    (void)log; // this technique only eliminates candidates, it never places
    bool changed = false;

    for (auto& unit : allUnits()) {
        std::vector<Cell> empties;
        for (auto& cell : unit)
            if (grid.values[cell.first][cell.second] == 0) empties.push_back(cell);

        if (static_cast<int>(empties.size()) < size_) continue;

        for (auto& combo : combinations(static_cast<int>(empties.size()), size_)) {
            uint16_t unionMask = 0;
            for (int idx : combo) {
                auto [r, c] = empties[idx];
                unionMask |= grid.candidates[r][c];
            }
            if (Grid::popcount(unionMask) != size_) continue; // not a tight subset

            std::vector<bool> inCombo(empties.size(), false);
            for (int idx : combo) inCombo[idx] = true;

            for (size_t i = 0; i < empties.size(); ++i) {
                if (inCombo[i]) continue;
                auto [r, c] = empties[i];
                for (int d = 1; d <= SIZE; ++d) {
                    if (unionMask & (1u << (d - 1))) {
                        if (grid.eliminateCandidate(r, c, d)) changed = true;
                    }
                }
            }
        }
    }
    return changed;
}

HiddenSubset::HiddenSubset(int size) : size_(size) {
    switch (size) {
        case 2: label_ = "Hidden Pair"; break;
        case 3: label_ = "Hidden Triple"; break;
        case 4: label_ = "Hidden Quad"; break;
        default: label_ = "Hidden Subset(" + std::to_string(size) + ")";
    }
}

bool HiddenSubset::apply(Grid& grid, std::vector<PlacementLog>& log) {
    (void)log;
    bool changed = false;

    for (auto& unit : allUnits()) {
        // Digits not yet placed anywhere in this unit.
        std::vector<int> missingDigits;
        for (int d = 1; d <= SIZE; ++d) {
            bool placedInUnit = false;
            for (auto& [r, c] : unit)
                if (grid.values[r][c] == d) { placedInUnit = true; break; }
            if (!placedInUnit) missingDigits.push_back(d);
        }
        if (static_cast<int>(missingDigits.size()) < size_) continue;

        for (auto& combo : combinations(static_cast<int>(missingDigits.size()), size_)) {
            uint16_t digitMask = 0;
            for (int idx : combo) digitMask |= (1u << (missingDigits[idx] - 1));

            // Cells in the unit that could hold any of these digits.
            std::vector<Cell> hostCells;
            for (auto& [r, c] : unit) {
                if (grid.values[r][c] != 0) continue;
                if (grid.candidates[r][c] & digitMask) hostCells.push_back({r, c});
            }

            if (static_cast<int>(hostCells.size()) != size_) continue; // not confined tightly enough

            for (auto& [r, c] : hostCells) {
                uint16_t before = grid.candidates[r][c];
                uint16_t after = before & digitMask; // strip every candidate outside the subset
                if (after != before) {
                    grid.candidates[r][c] = after;
                    changed = true;
                }
            }
        }
    }
    return changed;
}

bool PointingPair::apply(Grid& grid, std::vector<PlacementLog>& log) {
    (void)log;
    bool changed = false;

    for (int b = 0; b < SIZE; ++b) {
        auto boxCells = Grid::cellsInBox(b);
        for (int digit = 1; digit <= SIZE; ++digit) {
            uint16_t bit = static_cast<uint16_t>(1u << (digit - 1));
            std::vector<Cell> hosts;
            for (auto& [r, c] : boxCells)
                if (grid.values[r][c] == 0 && (grid.candidates[r][c] & bit)) hosts.push_back({r, c});

            if (hosts.size() < 2) continue;

            bool sameRow = true, sameCol = true;
            int row0 = hosts[0].first, col0 = hosts[0].second;
            for (auto& [r, c] : hosts) {
                if (r != row0) sameRow = false;
                if (c != col0) sameCol = false;
            }

            if (sameRow) {
                for (auto& [r, c] : Grid::cellsInRow(row0)) {
                    if (Grid::boxOf(r, c) == b) continue; // skip the box itself
                    if (grid.eliminateCandidate(r, c, digit)) changed = true;
                }
            }
            if (sameCol) {
                for (auto& [r, c] : Grid::cellsInCol(col0)) {
                    if (Grid::boxOf(r, c) == b) continue;
                    if (grid.eliminateCandidate(r, c, digit)) changed = true;
                }
            }
        }
    }
    return changed;
}

bool ClaimingPair::apply(Grid& grid, std::vector<PlacementLog>& log) {
    (void)log;
    bool changed = false;

    auto tryLine = [&](const std::vector<Cell>& line) {
        for (int digit = 1; digit <= SIZE; ++digit) {
            uint16_t bit = static_cast<uint16_t>(1u << (digit - 1));
            std::vector<Cell> hosts;
            for (auto& [r, c] : line)
                if (grid.values[r][c] == 0 && (grid.candidates[r][c] & bit)) hosts.push_back({r, c});

            if (hosts.size() < 2) continue;

            int box0 = Grid::boxOf(hosts[0].first, hosts[0].second);
            bool sameBox = true;
            for (auto& [r, c] : hosts)
                if (Grid::boxOf(r, c) != box0) { sameBox = false; break; }

            if (!sameBox) continue;

            for (auto& [r, c] : Grid::cellsInBox(box0)) {
                bool onLine = false;
                for (auto& [lr, lc] : line)
                    if (lr == r && lc == c) { onLine = true; break; }
                if (onLine) continue; // skip cells that are part of the line itself
                if (grid.eliminateCandidate(r, c, digit)) changed = true;
            }
        }
    };

    for (int r = 0; r < SIZE; ++r) tryLine(Grid::cellsInRow(r));
    for (int c = 0; c < SIZE; ++c) tryLine(Grid::cellsInCol(c));

    return changed;
}
