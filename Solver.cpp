#include "Solver.h"
#include "Tier3.h"
#include "Reasons.h"
#include "Tier4.h"

Solver::Solver() {
    auto add = [&](std::unique_ptr<Technique> t, int tier) {
        techniques_.push_back({std::move(t), tier});
    };

    // Tier 1
    add(std::make_unique<NakedSingle>(), 1);
    add(std::make_unique<HiddenSingle>(), 1);

    // Tier 2
    add(std::make_unique<NakedSubset>(2), 2);
    add(std::make_unique<HiddenSubset>(2), 2);
    add(std::make_unique<PointingPair>(), 2);
    add(std::make_unique<ClaimingPair>(), 2);
    add(std::make_unique<NakedSubset>(3), 2);
    add(std::make_unique<HiddenSubset>(3), 2);
    add(std::make_unique<NakedSubset>(4), 2);
    add(std::make_unique<HiddenSubset>(4), 2);

    // Tier 3 (cheapest / most common patterns first)
    add(std::make_unique<Fish>(2), 3);          // X-Wing
    add(std::make_unique<Skyscraper>(), 3);
    add(std::make_unique<TwoStringKite>(), 3);
    add(std::make_unique<XYWing>(), 3);
    add(std::make_unique<XYZWing>(), 3);
    add(std::make_unique<Fish>(3), 3);          // Swordfish
    add(std::make_unique<SimpleColoring>(), 3);
    add(std::make_unique<Fish>(4), 3);          // Jellyfish
}

bool Solver::solve(Grid& grid, std::vector<StepLog>& log) {
    stats_ = SolveStats{};
    searchTrace_.clear();
    searchTraceTruncated_ = false;
    const Grid start = grid;
    const size_t logStart = log.size();

    while (!grid.isSolved()) {
        bool progressed = false;

        // Simplest-first: after ANY success, start again from the top of the list.
        for (auto& entry : techniques_) {
            ++stats_.probes;
            if (entry.technique->apply(grid, log)) {
                stats_.techniqueSteps[entry.technique->name()]++;
                ++stats_.logicSteps;
                if (entry.tier > stats_.highestTier) stats_.highestTier = entry.tier;
                if (observer_) observer_(entry.technique->name(), grid);
                progressed = true;
                break;
            }
        }
        if (!progressed) break; // logic is stuck
    }

    // Work out why each logic step was possible (e.g. a Naked Single that an XY-Wing unlocked).
    reasons::attribute(start, log, logStart, [this](const std::string& name) {
        for (const auto& entry : techniques_)
            if (entry.technique->name() == name) return entry.tier;
        return 0;
    });

    if (!grid.isSolved() && searchEnabled_) {
        // Tier 4: hand the stuck grid (with all logical eliminations) to the search.
        Backtracker bt;
        SearchResult res = bt.search(grid, 1, recordSearchTrace_);
        stats_.usedSearch = true;
        stats_.guesses = res.guesses;
        stats_.searchAssignments = res.assignments;
        stats_.searchDeadEnds = res.deadEnds;
        stats_.searchDepth = res.maxDepth;
        stats_.firstBranching = res.firstBranching;
        stats_.highestTier = 4;
        searchTrace_ = std::move(res.trace);
        searchTraceTruncated_ = res.traceTruncated;

        if (res.solutionCount > 0) {
            // Log the placements of the successful path in the order they were really made
            // (guesses and forced placements), not in reading order.
            for (auto& step : res.path) {
                std::string detail = step.guess ? "guess (depth " + std::to_string(step.depth) + ")" : "forced: " + step.reason;
                log.push_back(StepLog::placement("Backtracking Search", step.row, step.col, step.digit, detail));
                ++stats_.searchCells;
            }
            grid = res.solution;
        }
    }
    return grid.isSolved();
}
