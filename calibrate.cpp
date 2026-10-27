// Usage: calibrate label1=file1.txt label2=file2.txt ...
// Each file holds puzzles one per line (e.g. from `qqwing --generate 500 --difficulty easy --one-line`).
// Prints the distribution of the three rating metrics per file, and how the CURRENT thresholds band them,
// so thresholds can be chosen (and re-checked on held-out data) from numbers rather than guesses.
#include "QQWingBridge.h"
#include "Rater.h"
#include <algorithm>
#include <iomanip>
#include <iostream>

static double pct(std::vector<double> v, double p) {
    std::sort(v.begin(), v.end());
    return v[static_cast<size_t>(p * (v.size() - 1))];
}

int main(int argc, char** argv) {
    Rater rater;
    std::vector<std::vector<double>> stepsPerFile, iterPerFile;   // for balanced pooling
    std::cout << std::fixed << std::setprecision(1);
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        size_t eq = arg.find('=');
        std::string label = arg.substr(0, eq), path = arg.substr(eq + 1);
        auto puzzles = loadPuzzlesFromFile(path);
        std::vector<double> score, steps, iter;
        int diffBand[3] = {0, 0, 0}, lenBand[3] = {0, 0, 0}, cplxBand[3] = {0, 0, 0};
        for (auto& p : puzzles) {
            Grid g = Grid::fromString(p);
            Solver s;
            std::vector<PlacementLog> log;
            s.solve(g, log);
            Rating r = rater.rate(s.stats());
            score.push_back(r.difficultyScore); steps.push_back(r.steps); iter.push_back(static_cast<double>(r.iterations));
            ++diffBand[r.difficulty == "Low" ? 0 : r.difficulty == "Medium" ? 1 : 2];
            ++lenBand[r.length == "Short" ? 0 : r.length == "Medium" ? 1 : 2];
            ++cplxBand[r.complexity == "Low" ? 0 : r.complexity == "Medium" ? 1 : 2];
        }
        stepsPerFile.push_back(steps);
        iterPerFile.push_back(iter);
        auto row = [&](const char* name, const std::vector<double>& v) {
            std::cout << "  " << std::setw(10) << std::left << name << std::right
                      << " min " << std::setw(6) << pct(v, 0) << "  p10 " << std::setw(6) << pct(v, .1)
                      << "  p25 " << std::setw(6) << pct(v, .25) << "  med " << std::setw(6) << pct(v, .5)
                      << "  p75 " << std::setw(6) << pct(v, .75) << "  p90 " << std::setw(6) << pct(v, .9)
                      << "  max " << std::setw(6) << pct(v, 1) << "\n";
        };
        std::cout << label << " (" << puzzles.size() << " puzzles)\n"
                  << "  difficulty Low/Med/High: " << diffBand[0] << "/" << diffBand[1] << "/" << diffBand[2]
                  << "   length Short/Med/Long: " << lenBand[0] << "/" << lenBand[1] << "/" << lenBand[2]
                  << "   complexity Low/Med/High: " << cplxBand[0] << "/" << cplxBand[1] << "/" << cplxBand[2] << "\n";
        row("score", score); row("steps", steps); row("iterations", iter);
    }
    // Balanced pooling: take the same number of puzzles from every file, so the pooled
    // tertiles don't depend on how many puzzles of each level happened to be generated.
    size_t minN = static_cast<size_t>(-1);
    for (auto& v : stepsPerFile) minN = std::min(minN, v.size());
    std::vector<double> allSteps, allIter;
    for (size_t f = 0; f < stepsPerFile.size(); ++f) {
        allSteps.insert(allSteps.end(), stepsPerFile[f].begin(), stepsPerFile[f].begin() + minN);
        allIter.insert(allIter.end(), iterPerFile[f].begin(), iterPerFile[f].begin() + minN);
    }
    std::cout << "\nBalanced pool (" << minN << " puzzles from each file):  steps p33=" << pct(allSteps, .33)
              << " p66=" << pct(allSteps, .66) << "   iterations p33=" << pct(allIter, .33)
              << " p66=" << pct(allIter, .66) << "\n";
    return 0;
}
