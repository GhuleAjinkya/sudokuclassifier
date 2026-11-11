#pragma once
// Works out WHY each step was possible, so the printed technique is the real reason.
//
// Only Naked/Hidden Singles place digits; every other technique removes candidates. So a number
// "placed by an XY-Wing" is an XY-Wing elimination that turned a cell into a single, followed by a
// single doing the placing. attribute() recovers that link from the log.
//
// It replays the log on two candidate grids side by side:
//   SOLVER grid  givens + placements + every logged elimination (what the solver saw)
//   PLAIN grid   givens + placements only (what you see without any pencil-mark work)
//
// A step relies on elimination step E when it needs a candidate to be ABSENT, that candidate is still
// PRESENT on the plain grid, and E is the step that removed it. A candidate that placements already
// removed from the plain grid is "free", so a placement that plain singles could have made is NOT
// credited to an elimination, however early that elimination happened. Labels cannot be inflated.
//
// For a placement there can be several valid explanations (naked single, or hidden single in the row,
// column or box). The one that relies on the easiest eliminations is used.
//
// Dependencies are followed through chains of eliminations (an X-Wing that enabled a Pointing Pair
// that enabled a single), but not through other placements: a digit that follows from an earlier
// placement is an ordinary single on the board at that moment.
#include "Grid.h"
#include "Techniques.h"
#include <algorithm>
#include <array>
#include <functional>
#include <set>
#include <string>
#include <vector>

namespace reasons {

// Fills enabledBy / rootMethod / chain for log[from..]. `start` is the grid before solving began.
// `tierOf` maps a technique name to its tier (higher = harder).
inline void attribute(const Grid& start, std::vector<StepLog>& log, size_t from,
                      const std::function<int(const std::string&)>& tierOf) {
    constexpr int N = SIZE * SIZE;
    const auto bit = [](int d) { return static_cast<uint16_t>(1u << (d - 1)); };

    std::array<int, N> value{};
    std::array<uint16_t, N> solver{}, plain{};
    const Grid fresh = Grid::fromString(start.toString()); // candidates from the givens alone
    for (int r = 0; r < SIZE; ++r) {
        for (int c = 0; c < SIZE; ++c) {
            const int i = r * SIZE + c;
            value[i] = start.values[r][c];
            solver[i] = start.candidates[r][c];
            plain[i] = fresh.candidates[r][c];
        }
    }

    // removedBy[i][d-1]:  -1 = candidate still present
    //                     -2 = removed by the givens or by a placement
    //                    k>=0 = removed by elimination step k
    std::vector<std::array<int, 9>> removedBy(N);
    for (int i = 0; i < N; ++i)
        for (int d = 1; d <= SIZE; ++d) removedBy[i][d - 1] = (solver[i] & bit(d)) ? -1 : -2;

    std::vector<std::set<int>> closure(log.size());
    const auto& units = allUnits();

    for (size_t idx = from; idx < log.size(); ++idx) {
        StepLog& s = log[idx];
        std::set<int> deps;

        if (s.kind == StepLog::Kind::Placement) {
            const int X = s.row * SIZE + s.col;
            const int d = s.digit;

            struct Option { std::set<int> deps; int hardest = -1; };
            bool haveBest = false;
            Option best;
            auto consider = [&](std::set<int> ds) {
                int hardest = -1;
                for (int k : ds) hardest = std::max(hardest, tierOf(log[k].method));
                if (!haveBest || hardest < best.hardest || (hardest == best.hardest && ds.size() < best.deps.size())) {
                    best.deps = std::move(ds);
                    best.hardest = hardest;
                    haveBest = true;
                }
            };

            // Naked single: the cell is down to one candidate.
            if (solver[X] == bit(d)) {
                std::set<int> ds;
                for (int e = 1; e <= SIZE; ++e) {
                    if (e == d || !(plain[X] & bit(e))) continue;
                    const int cause = removedBy[X][e - 1];
                    if (cause >= 0) ds.insert(cause);
                }
                consider(std::move(ds));
            }

            // Hidden single: the digit has nowhere else to go in a row, column or box containing the cell.
            for (const auto& unit : units) {
                bool inUnit = false;
                for (const auto& cell : unit) if (cell.first * SIZE + cell.second == X) inUnit = true;
                if (!inUnit) continue;

                bool valid = true;
                for (const auto& cell : unit) {
                    const int Y = cell.first * SIZE + cell.second;
                    if (Y != X && value[Y] == 0 && (solver[Y] & bit(d))) { valid = false; break; }
                }
                if (!valid) continue;

                std::set<int> ds;
                for (const auto& cell : unit) {
                    const int Y = cell.first * SIZE + cell.second;
                    if (Y == X || value[Y] != 0) continue;
                    if ((plain[Y] & bit(d)) && !(solver[Y] & bit(d))) {
                        const int cause = removedBy[Y][d - 1];
                        if (cause >= 0) ds.insert(cause);
                    }
                }
                consider(std::move(ds));
            }

            if (haveBest) deps = best.deps;

            // Apply the placement to both grids.
            value[X] = d;
            solver[X] = 0;
            plain[X] = 0;
            for (const auto& [pr, pc] : Grid::peersOf(s.row, s.col)) {
                const int Y = pr * SIZE + pc;
                solver[Y] &= static_cast<uint16_t>(~bit(d));
                plain[Y] &= static_cast<uint16_t>(~bit(d));
                if (removedBy[Y][d - 1] == -1) removedBy[Y][d - 1] = -2;
            }
        } else {
            // An elimination relies on the candidates its pattern needed to be absent.
            for (const auto& ev : s.evidence) {
                const int Y = ev.row * SIZE + ev.col;
                if (value[Y] == 0 && (plain[Y] & bit(ev.digit)) && !(solver[Y] & bit(ev.digit))) {
                    const int cause = removedBy[Y][ev.digit - 1];
                    if (cause >= 0) deps.insert(cause);
                }
            }
            for (const auto& rm : s.removed) {
                const int Y = rm.row * SIZE + rm.col;
                solver[Y] &= static_cast<uint16_t>(~bit(rm.digit));
                if (removedBy[Y][rm.digit - 1] == -1) removedBy[Y][rm.digit - 1] = static_cast<int>(idx);
            }
        }

        s.enabledBy.assign(deps.begin(), deps.end());

        // Follow the chain through earlier eliminations.
        std::set<int> all = deps;
        for (int k : deps) all.insert(closure[k].begin(), closure[k].end());
        closure[idx] = all;

        s.rootMethod.clear();
        s.chain.clear();
        if (!all.empty()) {
            int rootIdx = -1;
            int bestTier = -1;
            for (int k : all) { // ascending, so on a tie the latest step wins
                const int t = tierOf(log[k].method);
                if (t >= bestTier) { bestTier = t; rootIdx = k; }
            }
            s.rootMethod = log[rootIdx].method;
            for (int k : all) s.chain.push_back(log[k].method);
        }
    }
}

} // namespace reasons
