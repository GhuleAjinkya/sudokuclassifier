#pragma once
#include "Grid.h"
#include "Techniques.h"
#include <memory>
#include <vector>

class Solver {
public:
    Solver();

    bool solve(Grid& grid, vector<PlacementLog>& log);

private:
    vector<unique_ptr<Technique>> tier1_;
    vector<unique_ptr<Technique>> tier2_;
};
