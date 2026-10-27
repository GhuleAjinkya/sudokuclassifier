#include "Grid.h"
#include "QQWingBridge.h"
#include "Rater.h"
#include "Solver.h"
#include "Tier4.h"
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>

static void usage() {
    std::cout <<
        "Usage:\n"
        "  sudoku_solver                                 solve a built-in sample puzzle\n"
        "  sudoku_solver --puzzle <81 chars>             solve one puzzle ('.' or '0' = blank)\n"
        "  sudoku_solver --file <path>                   solve every puzzle in a file (one per line)\n"
        "  sudoku_solver --stdin                         read puzzles from stdin, e.g.\n"
        "                                                  qqwing --generate 20 --difficulty easy --one-line | sudoku_solver --stdin\n"
        "  sudoku_solver --qqwing <difficulty> <count>   generate puzzles by running qqwing\n"
        "                                                  difficulty: simple | easy | intermediate | expert | any\n"
        "  --qqwing-path <exe>                           path to qqwing if it isn't on PATH\n"
        "  --config <file>                               load rating weights/thresholds (see ratings.cfg)\n"
        "  --dump-config                                 print the current rating settings and exit\n"
        "  -v                                            also print every placement (digit, coordinates, method)\n";
}

static std::string describe(const SolveStats& s) {
    std::string out;
    for (auto& [name, n] : s.techniqueSteps) {
        if (!out.empty()) out += ", ";
        out += name + " x" + std::to_string(n);
    }
    if (s.usedSearch) {
        if (!out.empty()) out += ", ";
        out += "Backtracking Search (" + std::to_string(s.searchCells) + " cells, " +
               std::to_string(s.guesses) + " guesses, depth " + std::to_string(s.searchDepth) + ")";
    }
    return out.empty() ? "(none)" : out;
}

static std::string fmt1(double v) {
    std::ostringstream o; o.precision(1); o << std::fixed << v; return o.str();
}

int main(int argc, char** argv) {
    std::vector<std::string> puzzles;
    std::string qqwingPath = "qqwing", qqDifficulty, source = "built-in sample", configPath;
    int qqCount = 0;
    bool verbose = false, useQQWing = false, useStdin = false, dumpConfig = false;
    std::string file, single;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-v") verbose = true;
        else if (a == "--stdin") useStdin = true;
        else if (a == "--dump-config") dumpConfig = true;
        else if (a == "--file" && i + 1 < argc) file = argv[++i];
        else if (a == "--puzzle" && i + 1 < argc) single = argv[++i];
        else if (a == "--config" && i + 1 < argc) configPath = argv[++i];
        else if (a == "--qqwing-path" && i + 1 < argc) qqwingPath = argv[++i];
        else if (a == "--qqwing" && i + 2 < argc) {
            useQQWing = true;
            qqDifficulty = argv[++i];
            try { qqCount = std::stoi(argv[++i]); } catch (...) { usage(); return 1; }
        } else { usage(); return 1; }
    }

    Rater rater;
    if (!configPath.empty()) {
        std::string err;
        if (!rater.loadConfig(configPath, err)) { std::cerr << "config error: " << err << "\n"; return 1; }
    }
    if (dumpConfig) { std::cout << rater.configText(); return 0; }

    if (useQQWing) {
        std::string err;
        puzzles = generateWithQQWing(qqDifficulty, qqCount, qqwingPath, err);
        if (puzzles.empty()) { std::cerr << "qqwing error: " << err << "\n"; return 1; }
        source = "qqwing (" + qqDifficulty + ")";
    } else if (useStdin) {
        puzzles = loadPuzzles(std::cin);
        source = "stdin";
    } else if (!file.empty()) {
        puzzles = loadPuzzlesFromFile(file);
        source = file;
    } else if (!single.empty()) {
        std::istringstream in(single + "\n");
        puzzles = loadPuzzles(in);
        source = "command line";
    } else {
        puzzles.push_back(
            "530070000600195000098000060800060003400803001700020006060000280000419005000080079");
    }

    if (puzzles.empty()) { std::cerr << "No valid puzzles found (need 81 chars of 1-9 / . / 0 per line).\n"; return 1; }

    const bool detailed = puzzles.size() == 1;
    int solvedCount = 0, usedSearchCount = 0;
    int uniqueCount = 0, multipleCount = 0, noSolutionCount = 0;
    std::map<std::string, int> diffCount, lenCount, cplxCount;

    for (size_t n = 0; n < puzzles.size(); ++n) {
        Grid original = Grid::fromString(puzzles[n]);

        // Uniqueness check on a fresh grid (independent of any logic technique).
        SearchResult uniq = Backtracker().search(original, 2);
        std::string uniqueness = uniq.solutionCount == 1 ? "unique"
                               : uniq.solutionCount >= 2 ? "MULTIPLE SOLUTIONS" : "NO SOLUTION";
        if (uniq.solutionCount == 0) {
            // Contradictory or unsolvable: there is nothing meaningful to rate.
            ++noSolutionCount;
            if (detailed) { std::cout << "--- Starting Grid ---\n"; original.print(); std::cout << "\n"; }
            std::cout << "#" << n + 1 << "  NO SOLUTION (contradictory or unsolvable puzzle) | not rated\n";
            continue;
        }
        if (uniq.solutionCount == 1) ++uniqueCount;
        else ++multipleCount;

        Grid grid = original;
        Solver solver;
        std::vector<PlacementLog> log;
        bool solved = solver.solve(grid, log);
        const SolveStats& st = solver.stats();
        Rating rating = rater.rate(st);

        if (detailed) {
            std::cout << "--- Starting Grid ---\n";
            original.print();
            std::cout << "\n";
        }
        if (verbose || detailed) {
            std::cout << "--- Placements (" << log.size() << ") ---\n";
            for (auto& e : log)
                std::cout << "Placed " << e.digit << " at (row " << e.row + 1 << ", col " << e.col + 1
                          << ") via " << e.method << "\n";
            std::cout << "\n";
        }
        if (detailed) {
            std::cout << "--- Final Grid ---\n";
            grid.print();
            std::cout << "\n";
        }

        if (solved) {
            ++solvedCount;
            if (st.usedSearch) ++usedSearchCount;
        }
        diffCount[rating.difficulty]++;
        lenCount[rating.length]++;
        cplxCount[rating.complexity]++;

        std::cout << "#" << n + 1 << "  " << (solved ? (st.usedSearch ? "solved WITH SEARCH" : "solved by logic") : "UNSOLVED")
                  << " | " << uniqueness
                  << " | Difficulty: " << rating.difficulty << " (score " << fmt1(rating.difficultyScore) << ")"
                  << " | Length: " << rating.length << " (" << rating.steps << " steps)"
                  << " | Complexity: " << rating.complexity << " (" << rating.iterations << " iterations)\n";
        if (detailed || verbose) std::cout << "    techniques: " << describe(st) << "\n";
    }

    auto line = [&](const char* title, std::map<std::string, int>& m, std::initializer_list<const char*> order) {
        std::cout << "  " << title << ":";
        for (auto* k : order) std::cout << "  " << k << " " << m[k];
        std::cout << "\n";
    };
    std::cout << "\n=== Summary (" << source << ", " << puzzles.size() << " puzzles) ===\n"
              << "Solved: " << solvedCount << "  (needed search: " << usedSearchCount << ")\n"
              << "Uniqueness: " << uniqueCount << " unique, " << multipleCount << " multiple-solution, "
              << noSolutionCount << " no-solution\n";
    line("Difficulty", diffCount, {"Low", "Medium", "High"});
    line("Length    ", lenCount, {"Short", "Medium", "Long"});
    line("Complexity", cplxCount, {"Low", "Medium", "High"});
    return 0;
}
