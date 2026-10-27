#pragma once
#include "Solver.h"
#include <map>
#include <string>

// Three independent Low/Medium/High-style ratings for one solved puzzle.
struct Rating {
    double difficultyScore = 0;   // weighted technique score (the number behind `difficulty`)
    std::string difficulty;       // Low / Medium / High   - what kind of reasoning is required
    int steps = 0;                // logic steps + cells filled by search
    std::string length;           // Short / Medium / Long - how many moves to finish
    long iterations = 0;          // technique probes + search assignments
    std::string complexity;       // Low / Medium / High   - how much solver work it took
};

class Rater {
public:
    Rater();  // built-in defaults (calibrated on qqwing batches)

    // Overrides defaults from a simple "key = value" text file. See ratings.cfg.
    bool loadConfig(const std::string& path, std::string& error);
    std::string configText() const;  // current settings in the same format

    double scoreOf(const SolveStats& s) const;
    Rating rate(const SolveStats& s) const;

    // ---- tunables ----
    std::map<std::string, double> weights;  // per technique, per logic step
    double searchBase = 40.0;               // flat penalty once Tier 4 guessing is needed
    double searchPerDepth = 5.0;            // extra per level of nested guessing
    double difficultyMedium = 0, difficultyHigh = 0;   // score thresholds
    int    lengthMedium = 0,     lengthLong = 0;       // step-count thresholds
    long   complexityMedium = 0, complexityHigh = 0;   // iteration thresholds
};
