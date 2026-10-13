#include "Solver.h"
#include "Tier3.h"

Solver::Solver() {
    tier1_.push_back(std::make_unique<NakedSingle>());
    tier1_.push_back(std::make_unique<HiddenSingle>());

    tier2_.push_back(std::make_unique<NakedSubset>(2));
    tier2_.push_back(std::make_unique<HiddenSubset>(2));
    tier2_.push_back(std::make_unique<PointingPair>());
    tier2_.push_back(std::make_unique<ClaimingPair>());
    tier2_.push_back(std::make_unique<NakedSubset>(3));
    tier2_.push_back(std::make_unique<HiddenSubset>(3));
    tier2_.push_back(std::make_unique<NakedSubset>(4));
    tier2_.push_back(std::make_unique<HiddenSubset>(4));

    // Cheapest / most common patterns first.
    tier3_.push_back(std::make_unique<Fish>(2));        // X-Wing
    tier3_.push_back(std::make_unique<Skyscraper>());
    tier3_.push_back(std::make_unique<TwoStringKite>());
    tier3_.push_back(std::make_unique<XYWing>());
    tier3_.push_back(std::make_unique<XYZWing>());
    tier3_.push_back(std::make_unique<Fish>(3));        // Swordfish
    tier3_.push_back(std::make_unique<SimpleColoring>());
    tier3_.push_back(std::make_unique<Fish>(4));        // Jellyfish
}

bool Solver::tryTier(std::vector<std::unique_ptr<Technique>>& tier, int tierNumber,
                     Grid& grid, std::vector<PlacementLog>& log) {
    for (auto& technique : tier) {
        if (technique->apply(grid, log)) {
            stats_.techniqueUses[technique->name()]++;
            if (tierNumber > stats_.highestTier) stats_.highestTier = tierNumber;
            return true;
        }
    }
    return false;
}

bool Solver::solve(Grid& grid, std::vector<PlacementLog>& log) {
    stats_ = SolveStats{};

    while (!grid.isSolved()) {
        // Tier 1 to exhaustion (cheap, and placements cascade).
        bool tier1Changed = true;
        while (tier1Changed && !grid.isSolved()) {
            tier1Changed = false;
            for (auto& technique : tier1_) {
                if (technique->apply(grid, log)) {
                    stats_.techniqueUses[technique->name()]++;
                    if (stats_.highestTier < 1) stats_.highestTier = 1;
                    tier1Changed = true;
                }
            }
        }
        if (grid.isSolved()) break;

        // Escalate one rung at a time; any success sends us back to Tier 1.
        if (tryTier(tier2_, 2, grid, log)) continue;
        if (tryTier(tier3_, 3, grid, log)) continue;

        break; // stuck: needs Tier 4 (search)
    }
    return grid.isSolved();
}
