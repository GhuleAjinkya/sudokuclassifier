#include "Solver.h"
using namespace std;
Solver::Solver() {
    tier1_.push_back(make_unique<DirectSingle>());
    tier1_.push_back(make_unique<HiddenSingle>());
    tier2_.push_back(make_unique<DirectSubset>(2));
    tier2_.push_back(make_unique<DirectSubset>(3));
    tier2_.push_back(make_unique<DirectSubset>(4));
    tier2_.push_back(make_unique<HiddenSubset>(2));
    tier2_.push_back(make_unique<HiddenSubset>(3));
    tier2_.push_back(make_unique<HiddenSubset>(4));
    tier2_.push_back(make_unique<PointingPair>());
    tier2_.push_back(make_unique<ClaimingPair>());
}

bool Solver::solve(Grid& grid, vector<PlacementLog>& log) {
    bool progress = true;

    while (!grid.isSolved() && progress) {
        progress = false;

        bool tier1Changed = true;
        while (tier1Changed) {
            tier1Changed = false;
            for (auto& technique : tier1_) {
                if (technique->apply(grid, log)) {
                    tier1Changed = true;
                    progress = true;
                }
            }
        }

        if (grid.isSolved()) break;
        for (auto& technique : tier2_) {
            if (technique->apply(grid, log)) {
                progress = true;
                break;
            }
        }
    }

    return grid.isSolved();
}
