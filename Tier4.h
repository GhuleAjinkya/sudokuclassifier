#pragma once
#include "Grid.h"
#include <string>
#include <vector>

// One placement on the successful path through the search, in the order it was made.
struct SearchStep {
    int row;                 // 0-based
    int col;                 // 0-based
    int digit;
    bool guess;              // true: a guess; false: forced by propagation
    std::string reason;      // forced steps: "Naked Single" or "Hidden Single"
    int depth;               // number of guesses active once this step is made
};

// One event in the FULL exploration, including the wrong turns that were undone.
//   Guess          place `digit` at (row, col) tentatively; `options` lists what the cell could hold
//   Forced         propagation forced `digit` at (row, col) (`reason` says how)
//   Contradiction  this branch is a dead end; `reason` says why (and where, if it points at a cell or unit)
//   Backtrack      undo the guess at (row, col) and everything it forced
//   Solved         the first solution was reached
struct SearchEvent {
    enum class Type { Guess, Forced, Contradiction, Backtrack, Solved };

    Type type = Type::Guess;
    int row = -1;
    int col = -1;
    int digit = 0;
    int depth = 0;                   // guesses currently active (after applying this event)
    std::string reason;
    std::vector<int> options;        // Guess only
    std::string unitKind;            // Contradiction only: "row" | "column" | "box", or empty
    int unitIndex = -1;              // Contradiction only: 0-based index of that unit
};

// Result of a backtracking search.
struct SearchResult {
    int  solutionCount = 0;   // number of solutions found, capped at `limit`
    Grid solution;            // the first solution found (valid only if solutionCount > 0)
    long guesses = 0;         // candidate digits tried (search nodes)
    long assignments = 0;     // every digit placed during search: guesses + forced placements (hardware-independent effort)
    long deadEnds = 0;        // branches that ended in a contradiction
    int  maxDepth = 0;        // deepest nesting of guesses reached
    int  firstBranching = 0;  // how many candidates the very first guessed cell had

    std::vector<SearchStep> path;     // placements of the first solution, in order
    std::vector<SearchEvent> trace;   // everything the search did (only if requested)
    bool traceTruncated = false;      // true if the trace hit its size cap and stopped recording
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
    //
    // With recordTrace, every guess / forced placement / dead end / undo is recorded in
    // result.trace (capped at maxTraceEvents; the search itself is never cut short).
    SearchResult search(const Grid& start, int limit = 2, bool recordTrace = false, size_t maxTraceEvents = 4000) const;
};
