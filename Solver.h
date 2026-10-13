#pragma once
#include "Grid.h"
#include "Techniques.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

// What the last solve() call needed. Counts are "successful passes" of each
// technique (one pass may place/eliminate several things at once).
struct SolveStats {
    std::map<std::string, int> techniqueUses;
    int highestTier = 0;   // 1..3, or 0 if nothing was applied
};

class Solver {
public:
    Solver();

    // Escalates Tier 1 -> Tier 2 -> Tier 3. After ANY successful technique it
    // drops back to Tier 1, so harder techniques only run when everything
    // simpler is stuck. Returns true if solved by logic alone (no guessing).
    bool solve(Grid& grid, std::vector<PlacementLog>& log);

    const SolveStats& stats() const { return stats_; }

private:
    std::vector<std::unique_ptr<Technique>> tier1_;
    std::vector<std::unique_ptr<Technique>> tier2_;
    std::vector<std::unique_ptr<Technique>> tier3_;
    SolveStats stats_;

    // Tries each technique in order; stops at the first that changes the grid.
    bool tryTier(std::vector<std::unique_ptr<Technique>>& tier, int tierNumber,
                 Grid& grid, std::vector<PlacementLog>& log);
};
