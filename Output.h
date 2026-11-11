#pragma once
// Formatting for sudoku_solver: readable step/trace listings and the --json output used by the UI.
// Header-only so the build command does not need another .cpp file.
#include "Rater.h"
#include "Solver.h"
#include "Tier4.h"
#include <cstdio>
#include <iomanip>
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace output {

// ------------------------------------------------------------------ readable text

inline std::string cellShort(int r, int c) { return "r" + std::to_string(r + 1) + "c" + std::to_string(c + 1); }
inline std::string cellLong(int r, int c) { return "(row " + std::to_string(r + 1) + ", col " + std::to_string(c + 1) + ")"; }

inline std::string joinDigits(const std::vector<int>& digits) {
    std::string s;
    for (size_t i = 0; i < digits.size(); ++i) s += (i ? "," : "") + std::to_string(digits[i]);
    return s;
}

// "Naked Pair -> Naked Pair -> XY-Wing" becomes "Naked Pair x2 -> XY-Wing".
inline std::string chainText(const std::vector<std::string>& chain) {
    std::string out;
    for (size_t i = 0; i < chain.size();) {
        size_t j = i;
        while (j < chain.size() && chain[j] == chain[i]) ++j;
        if (!out.empty()) out += " -> ";
        out += chain[i];
        if (j - i > 1) out += " x" + std::to_string(j - i);
        i = j;
    }
    return out;
}

// " [relies on step 12 (XY-Wing); chain: Naked Pair -> XY-Wing]" or "" when the step relied on nothing.
inline std::string reliesOn(const StepLog& s, const std::vector<StepLog>& log) {
    if (s.enabledBy.empty()) return "";
    std::string out = "  [relies on ";
    for (size_t i = 0; i < s.enabledBy.size(); ++i) {
        const int k = s.enabledBy[i];
        out += (i ? ", " : "") + std::string("step ") + std::to_string(k + 1) + " (" + log[static_cast<size_t>(k)].method + ")";
    }
    if (s.chain.size() > s.enabledBy.size()) out += "; chain: " + chainText(s.chain);
    return out + "]";
}

// Every step in order: placements AND eliminations. A placement shows the technique it REALLY needed
// ("XY-Wing -> Naked Single": the XY-Wing made the single possible) and the steps it relies on.
inline void printSteps(std::ostream& os, const std::vector<StepLog>& log) {
    for (size_t i = 0; i < log.size(); ++i) {
        const StepLog& s = log[i];
        os << std::setw(3) << i + 1 << ". ";
        if (s.kind == StepLog::Kind::Placement) {
            os << "Placed " << s.digit << " at " << cellLong(s.row, s.col) << " via ";
            if (!s.rootMethod.empty()) os << s.rootMethod << " -> ";
            os << s.method;
            if (!s.detail.empty()) os << " (" << s.detail << ")";
            os << reliesOn(s, log) << "\n";
            continue;
        }
        std::map<int, std::vector<Removal>> byDigit;
        for (const auto& r : s.removed) byDigit[r.digit].push_back(r);
        os << "Eliminated ";
        bool firstDigit = true;
        for (const auto& [digit, list] : byDigit) {
            if (!firstDigit) os << "; ";
            firstDigit = false;
            os << digit << " from ";
            for (size_t k = 0; k < list.size(); ++k) os << (k ? ", " : "") << cellShort(list[k].row, list[k].col);
        }
        os << " via " << s.method << " [pattern";
        for (const auto& p : s.pattern) os << " " << cellShort(p.first, p.second);
        os << "; digits " << joinDigits(s.patternDigits);
        if (!s.detail.empty()) os << "; " << s.detail;
        os << "]" << reliesOn(s, log) << "\n";
    }
}

// The whole search, including guesses that turned out wrong and were undone.
inline void printTrace(std::ostream& os, const std::vector<SearchEvent>& trace, bool truncated, size_t maxLines = 200) {
    size_t shown = 0;
    for (const auto& e : trace) {
        if (shown >= maxLines) break;
        ++shown;
        const bool atGuessLevel = e.type == SearchEvent::Type::Guess || e.type == SearchEvent::Type::Backtrack;
        const std::string indent(static_cast<size_t>(e.depth > 0 ? (atGuessLevel ? e.depth - 1 : e.depth) : 0) * 2, ' ');
        switch (e.type) {
            case SearchEvent::Type::Guess:
                os << indent << "Guess " << e.digit << " at " << cellLong(e.row, e.col) << "  [depth " << e.depth
                   << ", options " << joinDigits(e.options) << "]\n";
                break;
            case SearchEvent::Type::Forced:
                os << indent << "  Forced " << e.digit << " at " << cellLong(e.row, e.col) << " via " << e.reason << "\n";
                break;
            case SearchEvent::Type::Contradiction:
                os << indent << "  Dead end: " << e.reason << "\n";
                break;
            case SearchEvent::Type::Backtrack:
                os << indent << "Undo guess " << e.digit << " at " << cellLong(e.row, e.col) << "  [wrong turn]\n";
                break;
            case SearchEvent::Type::Solved:
                os << indent << "Solved\n";
                break;
        }
    }
    if (trace.size() > shown) os << "... (" << trace.size() - shown << " more events not shown)\n";
    if (truncated) os << "(the search trace was cut off at " << trace.size() << " events; the search itself ran to completion)\n";
}

// ------------------------------------------------------------------ JSON

inline std::string esc(const std::string& s) {
    std::string out;
    for (unsigned char ch : s) {
        switch (ch) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (ch < 0x20) { char buf[8]; std::snprintf(buf, sizeof buf, "\\u%04x", ch); out += buf; }
                else out += static_cast<char>(ch);
        }
    }
    return out;
}
inline std::string str(const std::string& s) { return "\"" + esc(s) + "\""; }

inline std::string intArray(const std::vector<int>& v) {
    std::string s = "[";
    for (size_t i = 0; i < v.size(); ++i) s += (i ? "," : "") + std::to_string(v[i]);
    return s + "]";
}
inline std::string strArray(const std::vector<std::string>& v) {
    std::string s = "[";
    for (size_t i = 0; i < v.size(); ++i) s += (i ? "," : "") + str(v[i]);
    return s + "]";
}

// The attribution fields shared by placements and eliminations.
inline std::string reasonJson(const StepLog& s) {
    return ",\"enabledBy\":" + intArray(s.enabledBy) + ",\"root\":" + str(s.rootMethod) + ",\"chain\":" + strArray(s.chain);
}

inline std::string stepJson(const StepLog& s) {
    std::ostringstream o;
    if (s.kind == StepLog::Kind::Placement) {
        o << "{\"kind\":\"placement\",\"method\":" << str(s.method) << ",\"detail\":" << str(s.detail)
          << ",\"row\":" << s.row << ",\"col\":" << s.col << ",\"digit\":" << s.digit << reasonJson(s) << "}";
        return o.str();
    }
    o << "{\"kind\":\"elimination\",\"method\":" << str(s.method) << ",\"detail\":" << str(s.detail) << ",\"pattern\":[";
    for (size_t i = 0; i < s.pattern.size(); ++i) o << (i ? "," : "") << "[" << s.pattern[i].first << "," << s.pattern[i].second << "]";
    o << "],\"digits\":" << intArray(s.patternDigits) << ",\"removed\":[";
    for (size_t i = 0; i < s.removed.size(); ++i)
        o << (i ? "," : "") << "[" << s.removed[i].row << "," << s.removed[i].col << "," << s.removed[i].digit << "]";
    o << "]" << reasonJson(s) << "}";
    return o.str();
}

inline std::string eventJson(const SearchEvent& e) {
    using T = SearchEvent::Type;
    std::ostringstream o;
    switch (e.type) {
        case T::Guess:
            o << "{\"type\":\"guess\",\"row\":" << e.row << ",\"col\":" << e.col << ",\"digit\":" << e.digit
              << ",\"depth\":" << e.depth << ",\"options\":" << intArray(e.options) << "}";
            break;
        case T::Forced:
            o << "{\"type\":\"forced\",\"row\":" << e.row << ",\"col\":" << e.col << ",\"digit\":" << e.digit
              << ",\"depth\":" << e.depth << ",\"reason\":" << str(e.reason) << "}";
            break;
        case T::Contradiction:
            o << "{\"type\":\"contradiction\",\"depth\":" << e.depth << ",\"reason\":" << str(e.reason)
              << ",\"row\":" << e.row << ",\"col\":" << e.col << ",\"digit\":" << e.digit
              << ",\"unit\":" << str(e.unitKind) << ",\"unitIndex\":" << e.unitIndex << "}";
            break;
        case T::Backtrack:
            o << "{\"type\":\"backtrack\",\"row\":" << e.row << ",\"col\":" << e.col << ",\"digit\":" << e.digit
              << ",\"depth\":" << e.depth << "}";
            break;
        case T::Solved:
            o << "{\"type\":\"solved\",\"depth\":" << e.depth << "}";
            break;
    }
    return o.str();
}

// A puzzle that has no solution: nothing to rate.
inline std::string invalidJson(const std::string& puzzle) {
    return "{\"puzzle\":" + str(puzzle) + ",\"outcome\":\"invalid\",\"uniqueness\":\"none\",\"rating\":null,"
           "\"techniques\":[],\"stats\":null,\"search\":null,\"steps\":[],\"solution\":null}";
}

inline std::string resultJson(const std::string& puzzle, bool solved, int solutionCount, const Solver& solver,
                              const Rating& rating, const std::vector<StepLog>& log, const Grid& finalGrid) {
    const SolveStats& st = solver.stats();
    std::ostringstream o;
    o << std::fixed << std::setprecision(3);
    o << "{\"puzzle\":" << str(puzzle)
      << ",\"outcome\":" << str(solved ? (st.usedSearch ? "search" : "logic") : "unsolved")
      << ",\"uniqueness\":" << str(solutionCount == 1 ? "unique" : "multiple")
      << ",\"rating\":{\"difficulty\":" << str(rating.difficulty) << ",\"score\":" << rating.difficultyScore
      << ",\"length\":" << str(rating.length) << ",\"steps\":" << rating.steps
      << ",\"complexity\":" << str(rating.complexity) << ",\"iterations\":" << rating.iterations << "}";

    o << ",\"techniques\":[";
    bool first = true;
    for (const auto& [name, count] : st.techniqueSteps) {
        o << (first ? "" : ",") << "{\"name\":" << str(name) << ",\"count\":" << count << "}";
        first = false;
    }
    o << "],\"stats\":{\"logicSteps\":" << st.logicSteps << ",\"probes\":" << st.probes << ",\"iterations\":" << rating.iterations << "}";

    if (st.usedSearch) {
        o << ",\"search\":{\"cells\":" << st.searchCells << ",\"guesses\":" << st.guesses << ",\"deadEnds\":" << st.searchDeadEnds
          << ",\"depth\":" << st.searchDepth << ",\"firstBranching\":" << st.firstBranching
          << ",\"assignments\":" << st.searchAssignments
          << ",\"traceTruncated\":" << (solver.searchTraceTruncated() ? "true" : "false") << ",\"trace\":[";
        const auto& trace = solver.searchTrace();
        for (size_t i = 0; i < trace.size(); ++i) o << (i ? "," : "") << eventJson(trace[i]);
        o << "]}";
    } else {
        o << ",\"search\":null";
    }

    o << ",\"steps\":[";
    for (size_t i = 0; i < log.size(); ++i) o << (i ? "," : "") << stepJson(log[i]);
    o << "],\"solution\":" << (solved ? str(finalGrid.toString()) : std::string("null")) << "}";
    return o.str();
}

} // namespace output
