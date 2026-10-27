// Usage:  qqwing --generate 300 --difficulty expert --one-line --solution | ./soundness_test
// Reads alternating lines (puzzle, solution) from stdin and checks:
//   1. After EVERY logic step, the true digit is still a candidate in every unsolved cell
//      and every placed digit matches the known solution (no technique ever lies).
//   2. The full Solver (logic + Tier 4 search) always reproduces the known solution.
//   3. The uniqueness checker reports exactly 1 solution for qqwing puzzles.
//   4. The uniqueness checker also behaves on non-unique, contradictory and unsolvable grids.
#include "Grid.h"
#include "Solver.h"
#include "Tier4.h"
#include <iostream>
#include <map>

int main() {
    std::string puzzle, solution;
    int total = 0, solvedByLogic = 0, solvedWithSearch = 0, failures = 0;
    std::map<std::string, int> fired;

    while (std::getline(std::cin, puzzle) && std::getline(std::cin, solution)) {
        if (puzzle.size() < 81 || solution.size() < 81) continue;
        ++total;

        // --- uniqueness on a fresh grid ---
        SearchResult u = Backtracker().search(Grid::fromString(puzzle), 2);
        if (u.solutionCount != 1 || u.solution.toString() != solution.substr(0, 81)) {
            std::cout << "UNIQUENESS/SEARCH MISMATCH on puzzle " << total << " (count " << u.solutionCount << ")\n";
            ++failures;
        }

        // --- step-by-step soundness ---
        Solver solver;
        bool stepFailed = false;
        solver.setStepObserver([&](const std::string& name, const Grid& g) {
            if (stepFailed) return;
            for (int r = 0; r < 9; ++r) for (int c = 0; c < 9; ++c) {
                int truth = solution[r * 9 + c] - '0';
                bool bad = g.values[r][c] != 0 ? g.values[r][c] != truth
                                               : !(g.candidates[r][c] & (1u << (truth - 1)));
                if (bad) {
                    std::cout << "UNSOUND STEP by " << name << " at (" << r << "," << c << ") puzzle " << total << "\n";
                    stepFailed = true; ++failures; return;
                }
            }
        });

        Grid g = Grid::fromString(puzzle);
        std::vector<PlacementLog> log;
        bool solved = solver.solve(g, log);
        for (auto& [n, k] : solver.stats().techniqueSteps) fired[n] += k;

        if (!solved || g.toString() != solution.substr(0, 81)) {
            std::cout << "SOLVER DID NOT REACH THE KNOWN SOLUTION on puzzle " << total << "\n";
            ++failures;
        } else if (solver.stats().usedSearch) ++solvedWithSearch;
        else ++solvedByLogic;
    }

    // --- non-unique / contradictory / unsolvable grids ---
    int edgeCases = 0;
    auto expectCount = [&](const std::string& what, const std::string& grid81, int expected) {
        ++edgeCases;
        int got = Backtracker().search(Grid::fromString(grid81), 2).solutionCount;
        if (got != expected) {
            std::cout << "EDGE CASE FAILED: " << what << " (expected " << expected << ", got " << got << ")\n";
            ++failures;
        }
    };
    expectCount("empty grid has multiple solutions", std::string(81, '.'), 2);
    expectCount("duplicate givens have no solution", "55" + std::string(79, '.'), 0);
    // Row 1 holds 1-8, and a 9 sits in column 9 below it => cell (1,9) has no candidate left.
    expectCount("duplicate-free but unsolvable grid", "12345678." "........9" + std::string(63, '.'), 0);

    std::cout << "puzzles: " << total << " | solved by logic only: " << solvedByLogic
              << " | needed search: " << solvedWithSearch << " | failures: " << failures
              << " | edge-case checks: " << edgeCases << "\n";
    for (auto& [n, k] : fired) std::cout << "  " << n << ": " << k << " steps\n";
    return failures ? 1 : 0;
}
