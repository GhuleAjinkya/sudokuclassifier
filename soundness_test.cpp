// Usage:  qqwing --generate 300 --difficulty expert --one-line --solution | ./soundness_test
// Reads alternating lines (puzzle, solution) from stdin and checks:
//   1. After EVERY logic step, the true digit is still a candidate in every unsolved cell
//      and every placed digit matches the known solution (no technique ever lies).
//   2. The full Solver (logic + Tier 4 search) always reproduces the known solution.
//   3. The uniqueness checker reports exactly 1 solution for qqwing puzzles.
//   4. The uniqueness checker also behaves on non-unique, contradictory and unsolvable grids.
//   5. The step log is FAITHFUL: replaying the logged placements and eliminations from the givens
//      reproduces the solver's candidate grid exactly after every step.
//   6. Attribution is HONEST: a placement is credited to an elimination (e.g. "XY-Wing -> Naked Single")
//      exactly when it is NOT a plain naked/hidden single on a grid with no pencil-mark work, and every
//      candidate an elimination's evidence relies on really was absent when it ran.
//   7. The search trace is faithful: replaying its guesses, forced placements and undos reaches the
//      known solution, and its counts match the solver's statistics.
#include "Grid.h"
#include "Solver.h"
#include "Techniques.h"
#include "Tier4.h"
#include <iostream>
#include <map>

int main() {
    std::string puzzle, solution;
    int total = 0, solvedByLogic = 0, solvedWithSearch = 0, failures = 0, tracesChecked = 0;
    long wrongTurns = 0, creditedPlacements = 0;
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

        // --- step-by-step soundness + log fidelity ---
        Solver solver;
        solver.setRecordSearchTrace(true);
        std::vector<StepLog> log;
        Grid replay = Grid::fromString(puzzle);   // knows only the givens and the logged steps
        size_t replayed = 0;
        bool stepFailed = false;
        solver.setStepObserver([&](const std::string& name, const Grid& g) {
            if (stepFailed) return;

            // Replay everything logged since the last step and compare with the solver's real grid.
            for (; replayed < log.size(); ++replayed) {
                const StepLog& s = log[replayed];
                bool ok = true;
                if (s.kind == StepLog::Kind::Placement) {
                    ok = replay.place(s.row, s.col, s.digit);
                } else {
                    if (s.removed.empty() || s.pattern.empty()) ok = false;
                    for (auto& ev : s.evidence) {     // the facts the pattern relied on must really have been true
                        if (replay.values[ev.row][ev.col] != 0 ||
                            (replay.candidates[ev.row][ev.col] & (1u << (ev.digit - 1)))) ok = false;
                    }
                    for (auto& rm : s.removed) ok = replay.eliminateCandidate(rm.row, rm.col, rm.digit) && ok;
                }
                if (!ok) {
                    std::cout << "LOG ENTRY NOT REPLAYABLE (" << s.method << ") puzzle " << total << "\n";
                    stepFailed = true; ++failures; return;
                }
            }
            if (replay.values != g.values || replay.candidates != g.candidates) {
                std::cout << "LOG DOES NOT REPRODUCE THE SOLVER'S GRID after " << name << ", puzzle " << total << "\n";
                stepFailed = true; ++failures; return;
            }

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
        bool solved = solver.solve(g, log);
        for (auto& [n, k] : solver.stats().techniqueSteps) fired[n] += k;

        // --- attribution honesty (checked here independently of reasons::attribute) ---
        {
            static const std::map<std::string, int> tier = {
                {"Naked Single", 1}, {"Hidden Single", 1}, {"Naked Pair", 2}, {"Hidden Pair", 2}, {"Naked Triple", 2},
                {"Hidden Triple", 2}, {"Naked Quad", 2}, {"Hidden Quad", 2}, {"Pointing Pair/Triple (Box -> Line)", 2},
                {"Claiming (Line -> Box)", 2}, {"X-Wing", 3}, {"Swordfish", 3}, {"Jellyfish", 3}, {"XY-Wing", 3},
                {"XYZ-Wing", 3}, {"Skyscraper", 3}, {"2-String Kite", 3}, {"Simple Coloring", 3}};
            Grid plainGrid = Grid::fromString(puzzle);        // givens + placements only
            bool attributionOk = true;
            for (size_t i = 0; i < log.size() && attributionOk; ++i) {
                const StepLog& s = log[i];
                if (s.method == "Backtracking Search") break;
                for (int k : s.enabledBy)
                    if (k < 0 || k >= static_cast<int>(i) || log[k].kind != StepLog::Kind::Elimination) attributionOk = false;
                if (s.enabledBy.empty() != s.rootMethod.empty()) attributionOk = false;
                if (!s.rootMethod.empty()) {
                    int hardest = 0;
                    for (auto& m : s.chain) hardest = std::max(hardest, tier.at(m));
                    if (tier.at(s.rootMethod) != hardest || s.chain.size() < s.enabledBy.size()) attributionOk = false;
                }
                if (s.kind != StepLog::Kind::Placement) continue;

                // Is this placement a plain naked or hidden single, with no elimination work at all?
                const uint16_t bit = static_cast<uint16_t>(1u << (s.digit - 1));
                bool plainSingle = plainGrid.candidates[s.row][s.col] == bit;
                for (auto& unit : allUnits()) {
                    bool inUnit = false;
                    for (auto& cell : unit) if (cell.first == s.row && cell.second == s.col) inUnit = true;
                    if (!inUnit) continue;
                    bool onlyHere = true;
                    for (auto& cell : unit)
                        if ((cell.first != s.row || cell.second != s.col) && plainGrid.values[cell.first][cell.second] == 0 &&
                            (plainGrid.candidates[cell.first][cell.second] & bit)) onlyHere = false;
                    if (onlyHere) plainSingle = true;
                }
                // Credited to an elimination exactly when a plain grid could NOT have made this placement.
                if (plainSingle == !s.enabledBy.empty()) attributionOk = false;
                plainGrid.place(s.row, s.col, s.digit);
            }
            if (!attributionOk) {
                std::cout << "ATTRIBUTION WRONG on puzzle " << total << "\n";
                ++failures;
            }
            for (auto& s : log)
                if (s.kind == StepLog::Kind::Placement && !s.rootMethod.empty()) ++creditedPlacements;
        }

        // --- search trace fidelity ---
        if (solver.stats().usedSearch) {
            const auto& trace = solver.searchTrace();
            Grid board = replay;                       // the grid at the moment logic got stuck
            std::vector<Grid> frames;                  // one snapshot per active guess
            long guesses = 0, deadEnds = 0, undos = 0;
            bool solvedEvent = false, traceOk = true;
            using T = SearchEvent::Type;
            for (const auto& e : trace) {
                if (e.type == T::Guess) {
                    frames.push_back(board);
                    traceOk = board.place(e.row, e.col, e.digit) && traceOk;
                    ++guesses;
                    if (e.depth != static_cast<int>(frames.size())) traceOk = false;
                } else if (e.type == T::Forced) {
                    traceOk = board.place(e.row, e.col, e.digit) && traceOk;
                    if (e.depth != static_cast<int>(frames.size())) traceOk = false;
                } else if (e.type == T::Contradiction) {
                    ++deadEnds;
                } else if (e.type == T::Backtrack) {
                    if (frames.empty()) { traceOk = false; break; }
                    board = frames.back();
                    frames.pop_back();
                    ++undos;
                } else {
                    solvedEvent = true;
                }
            }
            if (!solver.searchTraceTruncated()) {
                if (!traceOk || !solvedEvent || board.toString() != solution.substr(0, 81) ||
                    guesses != solver.stats().guesses || deadEnds != solver.stats().searchDeadEnds ||
                    guesses - undos != static_cast<long>(frames.size())) {
                    std::cout << "SEARCH TRACE DOES NOT REPLAY TO THE SOLUTION on puzzle " << total << "\n";
                    ++failures;
                }
                ++tracesChecked;
                wrongTurns += undos;
            }
        }

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
              << " | needed search: " << solvedWithSearch << " (traces replayed: " << tracesChecked
              << ", wrong turns seen: " << wrongTurns << ") | placements credited to an elimination: " << creditedPlacements
              << " | failures: " << failures
              << " | edge-case checks: " << edgeCases << "\n";
    for (auto& [n, k] : fired) std::cout << "  " << n << ": " << k << " steps\n";
    return failures ? 1 : 0;
}
