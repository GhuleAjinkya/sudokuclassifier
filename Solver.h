#pragma once
#include "Grid.h"
#include "Techniques.h"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

// Everything the Rater needs to know about how a puzzle was solved.
struct SolveStats {
    std::map<std::string, int> techniqueSteps; // technique name -> logical steps taken (Tier 1-3)
    int  highestTier = 0;       // 1..3 = hardest logic tier used, 4 = search was needed, 0 = nothing applied
    int  logicSteps = 0;        // total Tier 1-3 steps (each = one placement or one pattern's eliminations)
    long probes = 0;            // technique apply() calls, successful or not (work done by the logic engine)

    // Tier 4
    bool usedSearch = false;
    int  searchCells = 0;       // cells that were filled by search rather than logic
    long guesses = 0;           // search nodes
    long searchAssignments = 0; // every digit placed during search (guesses + forced placements)
    int  searchDepth = 0;       // deepest guess nesting
    int  firstBranching = 0;    // candidates at the first guessed cell

    int  totalSteps() const { return logicSteps + searchCells; }     // "length"
    long iterations() const { return probes + searchAssignments; }   // "complexity"
};

class Solver {
public:
    Solver();

    // Solves the grid. Always tries techniques simplest-first, applying ONE step at a
    // time and restarting from the simplest technique after every success. If logic
    // stalls, falls back to Tier 4 search (unless disabled).
    // Returns true if the grid ended up fully solved (check stats().usedSearch to see
    // whether guessing was needed).
    bool solve(Grid& grid, std::vector<PlacementLog>& log);

    void setSearchEnabled(bool on) { searchEnabled_ = on; }
    const SolveStats& stats() const { return stats_; }

    // Optional hook, called after every successful logic step with the technique name
    // and the grid as it now stands. Used by the soundness test.
    using StepObserver = std::function<void(const std::string&, const Grid&)>;
    void setStepObserver(StepObserver obs) { observer_ = std::move(obs); }

private:
    struct Entry {
        std::unique_ptr<Technique> technique;
        int tier;
    };
    std::vector<Entry> techniques_;   // in priority order (simplest first)
    SolveStats stats_;
    bool searchEnabled_ = true;
    StepObserver observer_;
};
