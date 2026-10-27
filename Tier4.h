#pragma once
#include "Grid.h"

// Result of a backtracking search.
struct SearchResult {
    int  solutionCount = 0;   // number of solutions found, capped at `limit`
    Grid solution;            // the first solution found (valid only if solutionCount > 0)
    long guesses = 0;         // candidate digits tried (search nodes)
    long assignments = 0;     // every digit placed during search: guesses + forced placements (hardware-independent effort)
    int  maxDepth = 0;        // deepest nesting of guesses reached
    int  firstBranching = 0;  // how many candidates the very first guessed cell had
};

// Tier 4: constraint propagation (singles) + guessing on the cell with the fewest
// candidates (MRV). Used both to finish puzzles the logic techniques can't crack
// and to count solutions for the uniqueness check.
class Backtracker {
public:
    // Counts solutions up to `limit` (use 2 for a uniqueness check, 1 to just solve).
    // Stops as soon as `limit` solutions are found. A grid with duplicate givens
    // has 0 solutions. Existing candidate eliminations in `start` are respected, so
    // for a trustworthy uniqueness check pass a grid freshly built from the givens.
    SearchResult search(const Grid& start, int limit = 2) const;
};
