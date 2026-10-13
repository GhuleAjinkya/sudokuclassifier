#include "Grid.h"
#include "QQWingBridge.h"
#include "Solver.h"
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
        "  -v                                            also print every placement (digit, coordinates, method)\n";
}

static std::string describe(const SolveStats& s) {
    std::string out;
    for (auto& [name, n] : s.techniqueUses) {
        if (!out.empty()) out += ", ";
        out += name + " x" + std::to_string(n);
    }
    return out.empty() ? "(none)" : out;
}

int main(int argc, char** argv) {
    std::vector<std::string> puzzles;
    std::string qqwingPath = "qqwing", qqDifficulty, source = "built-in sample";
    int qqCount = 0;
    bool verbose = false, useQQWing = false, useStdin = false;
    std::string file, single;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-v") verbose = true;
        else if (a == "--stdin") useStdin = true;
        else if (a == "--file" && i + 1 < argc) file = argv[++i];
        else if (a == "--puzzle" && i + 1 < argc) single = argv[++i];
        else if (a == "--qqwing-path" && i + 1 < argc) qqwingPath = argv[++i];
        else if (a == "--qqwing" && i + 2 < argc) {
            useQQWing = true;
            qqDifficulty = argv[++i];
            try { qqCount = std::stoi(argv[++i]); } catch (...) { usage(); return 1; }
        } else { usage(); return 1; }
    }

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
        puzzles = loadPuzzles(*std::make_unique<std::istringstream>(single + "\n"));
        source = "command line";
    } else {
        puzzles.push_back(
            "530070000600195000098000060800060003400803001700020006060000280000419005000080079");
    }

    if (puzzles.empty()) { std::cerr << "No valid puzzles found (need 81 chars of 1-9 / . / 0 per line).\n"; return 1; }

    Solver solver;
    int solvedCount = 0;
    std::map<int, int> byTier;   // highest tier needed -> count (solved only)
    int needTier4 = 0;

    for (size_t n = 0; n < puzzles.size(); ++n) {
        Grid grid = Grid::fromString(puzzles[n]);
        std::vector<PlacementLog> log;
        bool solved = solver.solve(grid, log);
        const SolveStats& st = solver.stats();

        if (puzzles.size() == 1) {
            std::cout << "--- Starting Grid ---\n";
            Grid::fromString(puzzles[n]).print();
            std::cout << "\n";
        }

        if (verbose || puzzles.size() == 1) {
            std::cout << "--- Placements (" << log.size() << ") ---\n";
            for (auto& e : log)
                std::cout << "Placed " << e.digit << " at (row " << e.row + 1 << ", col " << e.col + 1
                          << ") via " << e.method << "\n";
            std::cout << "\n";
        }

        if (puzzles.size() == 1) {
            std::cout << "--- Final Grid ---\n";
            grid.print();
            std::cout << "\n";
        }

        int empties = 0;
        for (int r = 0; r < SIZE; ++r) for (int c = 0; c < SIZE; ++c) if (grid.values[r][c] == 0) ++empties;

        std::cout << "#" << n + 1 << "  ";
        if (solved) {
            ++solvedCount;
            byTier[st.highestTier]++;
            std::cout << "solved by logic, hardest tier used: " << st.highestTier;
        } else {
            ++needTier4;
            std::cout << "STUCK with " << empties << " empty cells -> needs Tier 4 (search)";
        }
        std::cout << "  [" << describe(st) << "]\n";
    }

    std::cout << "\n=== Summary (" << source << ", " << puzzles.size() << " puzzles) ===\n"
              << "Solved by logic alone: " << solvedCount << "\n";
    for (auto& [tier, cnt] : byTier) std::cout << "  hardest technique in Tier " << tier << ": " << cnt << "\n";
    std::cout << "Needs Tier 4 (guessing): " << needTier4 << "\n";
    return 0;
}
