#include "Tier4.h"
#include "Techniques.h" // allUnits()

namespace {

// Applies naked + hidden singles until nothing changes.
// Returns false if the grid is contradictory.
bool propagate(Grid& g, long& assignments) {
    bool progress = true;
    while (progress) {
        progress = false;

        for (int r = 0; r < SIZE; ++r) {
            for (int c = 0; c < SIZE; ++c) {
                if (g.values[r][c] != 0) continue;
                int n = Grid::popcount(g.candidates[r][c]);
                if (n == 0) return false;                 // empty cell with nothing left
                if (n == 1) {
                    if (!g.place(r, c, Grid::lowestSetDigit(g.candidates[r][c]))) return false;
                    ++assignments;
                    progress = true;
                }
            }
        }

        for (auto& unit : allUnits()) {
            for (int digit = 1; digit <= SIZE; ++digit) {
                uint16_t bit = static_cast<uint16_t>(1u << (digit - 1));
                bool placed = false;
                int hosts = 0;
                Cell only{-1, -1};
                for (auto& [r, c] : unit) {
                    if (g.values[r][c] == digit) { placed = true; break; }
                    if (g.values[r][c] == 0 && (g.candidates[r][c] & bit)) { ++hosts; only = {r, c}; }
                }
                if (placed) continue;
                if (hosts == 0) return false;             // digit has nowhere to go in this unit
                if (hosts == 1) {
                    if (!g.place(only.first, only.second, digit)) return false;
                    ++assignments;
                    progress = true;
                }
            }
        }
    }
    return true;
}

void recurse(Grid g, int depth, int limit, SearchResult& res) {
    if (!propagate(g, res.assignments)) return;

    if (g.isSolved()) {
        if (res.solutionCount == 0) res.solution = g;
        ++res.solutionCount;
        return;
    }

    // MRV: guess in the unsolved cell with the fewest candidates.
    int bestR = -1, bestC = -1, bestN = 100;
    for (int r = 0; r < SIZE; ++r) {
        for (int c = 0; c < SIZE; ++c) {
            if (g.values[r][c] != 0) continue;
            int n = Grid::popcount(g.candidates[r][c]);
            if (n < bestN) { bestN = n; bestR = r; bestC = c; }
        }
    }

    if (res.firstBranching == 0) res.firstBranching = bestN;
    if (depth + 1 > res.maxDepth) res.maxDepth = depth + 1;

    uint16_t mask = g.candidates[bestR][bestC];
    for (int digit = 1; digit <= SIZE; ++digit) {
        if (!(mask & (1u << (digit - 1)))) continue;
        ++res.guesses;
        ++res.assignments;
        Grid next = g;
        if (!next.place(bestR, bestC, digit)) continue;
        recurse(next, depth + 1, limit, res);
        if (res.solutionCount >= limit) return;
    }
}

} // namespace

SearchResult Backtracker::search(const Grid& start, int limit) const {
    SearchResult res;
    if (start.hasDuplicateValues()) return res; // contradictory givens: no solutions
    recurse(start, 0, limit, res);
    return res;
}
