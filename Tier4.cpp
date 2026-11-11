#include "Tier4.h"
#include "Techniques.h" // allUnits(), digitsOf(), unitName()

namespace {

struct Context {
    int limit;
    SearchResult& res;
    bool recordTrace;
    size_t maxEvents;
    std::vector<SearchStep> stack;   // placements currently on the board, in order

    Context(int lim, SearchResult& r, bool trace, size_t maxEv) : limit(lim), res(r), recordTrace(trace), maxEvents(maxEv) {}

    void emit(SearchEvent e) {
        if (!recordTrace) return;
        if (res.trace.size() < maxEvents) res.trace.push_back(std::move(e));
        else res.traceTruncated = true;
    }

    void forced(int r, int c, int d, const char* reason, int depth) {
        stack.push_back({r, c, d, false, reason, depth});
        SearchEvent e;
        e.type = SearchEvent::Type::Forced;
        e.row = r; e.col = c; e.digit = d; e.depth = depth; e.reason = reason;
        emit(std::move(e));
    }
};

struct DeadEnd {
    std::string reason;
    int row = -1, col = -1;
    int digit = 0;
    std::string unitKind;
    int unitIndex = -1;
};

std::string cellName(int r, int c) { return "r" + std::to_string(r + 1) + "c" + std::to_string(c + 1); }

// Applies naked + hidden singles until nothing changes, recording each forced placement.
// Returns false (and says why in `dead`) if the grid is contradictory.
bool propagate(Grid& g, Context& ctx, int depth, DeadEnd& dead) {
    bool progress = true;
    while (progress) {
        progress = false;

        for (int r = 0; r < SIZE; ++r) {
            for (int c = 0; c < SIZE; ++c) {
                if (g.values[r][c] != 0) continue;
                int n = Grid::popcount(g.candidates[r][c]);
                if (n == 0) {                                   // empty cell with nothing left
                    dead.reason = "cell " + cellName(r, c) + " has no candidates left";
                    dead.row = r; dead.col = c;
                    return false;
                }
                if (n == 1) {
                    int digit = Grid::lowestSetDigit(g.candidates[r][c]);
                    if (!g.place(r, c, digit)) { dead.reason = "forced placements conflict at " + cellName(r, c); dead.row = r; dead.col = c; return false; }
                    ++ctx.res.assignments;
                    ctx.forced(r, c, digit, "Naked Single", depth);
                    progress = true;
                }
            }
        }

        const auto& units = allUnits();
        for (size_t u = 0; u < units.size(); ++u) {
            for (int digit = 1; digit <= SIZE; ++digit) {
                uint16_t bit = static_cast<uint16_t>(1u << (digit - 1));
                bool placed = false;
                int hosts = 0;
                Cell only{-1, -1};
                for (auto& [r, c] : units[u]) {
                    if (g.values[r][c] == digit) { placed = true; break; }
                    if (g.values[r][c] == 0 && (g.candidates[r][c] & bit)) { ++hosts; only = {r, c}; }
                }
                if (placed) continue;
                if (hosts == 0) {                               // digit has nowhere to go in this unit
                    dead.reason = std::to_string(digit) + " has nowhere to go in " + unitName(static_cast<int>(u));
                    dead.digit = digit;
                    dead.unitKind = u < 9 ? "row" : (u < 18 ? "column" : "box");
                    dead.unitIndex = static_cast<int>(u % 9);
                    return false;
                }
                if (hosts == 1) {
                    if (!g.place(only.first, only.second, digit)) {
                        dead.reason = "forced placements conflict at " + cellName(only.first, only.second);
                        dead.row = only.first; dead.col = only.second;
                        return false;
                    }
                    ++ctx.res.assignments;
                    ctx.forced(only.first, only.second, digit, "Hidden Single", depth);
                    progress = true;
                }
            }
        }
    }
    return true;
}

void recurse(Grid g, int depth, Context& ctx) {
    SearchResult& res = ctx.res;

    DeadEnd dead;
    if (!propagate(g, ctx, depth, dead)) {
        ++res.deadEnds;
        SearchEvent e;
        e.type = SearchEvent::Type::Contradiction;
        e.depth = depth;
        e.reason = dead.reason;
        e.row = dead.row; e.col = dead.col; e.digit = dead.digit;
        e.unitKind = dead.unitKind; e.unitIndex = dead.unitIndex;
        ctx.emit(std::move(e));
        return;
    }

    if (g.isSolved()) {
        if (res.solutionCount == 0) {
            res.solution = g;
            res.path = ctx.stack;
            SearchEvent e;
            e.type = SearchEvent::Type::Solved;
            e.depth = depth;
            ctx.emit(std::move(e));
        }
        ++res.solutionCount;
        return;
    }

    // MRV: guess in the unsolved cell with the fewest candidates.
    int bestR = -1, bestC = -1, bestN = 100;
    for (int r = 0; r < SIZE; ++r) {
        for (int c = 0; c < SIZE; ++c) {
            if (g.values[r][c] != 0) continue;
            int n = Grid::popcount(g.candidates[r][c]);
            if (n < bestN) { bestN = n; bestR = r; bestC = c; }
        }
    }

    if (res.firstBranching == 0) res.firstBranching = bestN;
    if (depth + 1 > res.maxDepth) res.maxDepth = depth + 1;

    const std::vector<int> options = digitsOf(g.candidates[bestR][bestC]);
    for (int digit : options) {
        ++res.guesses;
        ++res.assignments;

        const size_t mark = ctx.stack.size();
        ctx.stack.push_back({bestR, bestC, digit, true, "", depth + 1});
        SearchEvent guess;
        guess.type = SearchEvent::Type::Guess;
        guess.row = bestR; guess.col = bestC; guess.digit = digit; guess.depth = depth + 1;
        guess.options = options;
        ctx.emit(std::move(guess));

        Grid next = g;
        if (next.place(bestR, bestC, digit)) {
            recurse(next, depth + 1, ctx);
            if (res.solutionCount >= ctx.limit) return;   // keep the successful path on the stack
        }

        // This guess led nowhere: undo it and everything it forced.
        ctx.stack.resize(mark);
        SearchEvent undo;
        undo.type = SearchEvent::Type::Backtrack;
        undo.row = bestR; undo.col = bestC; undo.digit = digit; undo.depth = depth + 1;
        ctx.emit(std::move(undo));
    }
}

} // namespace

SearchResult Backtracker::search(const Grid& start, int limit, bool recordTrace, size_t maxTraceEvents) const {
    SearchResult res;
    if (start.hasDuplicateValues()) return res; // contradictory givens: no solutions
    Context ctx(limit, res, recordTrace, maxTraceEvents);
    recurse(start, 0, ctx);
    return res;
}
