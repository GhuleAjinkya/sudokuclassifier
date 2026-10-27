#include "Rater.h"
#include <fstream>
#include <sstream>

Rater::Rater() {
    weights = {
        // Tier 1: nearly free, so a long run of easy steps doesn't look "hard".
        {"Naked Single", 0.01}, {"Hidden Single", 0.02},
        // Tier 2: modest. A pile of these should stay below a single Tier 3 step.
        {"Naked Pair", 1.0}, {"Hidden Pair", 1.5},
        {"Pointing Pair/Triple (Box -> Line)", 1.0}, {"Claiming (Line -> Box)", 1.0},
        {"Naked Triple", 1.5}, {"Hidden Triple", 2.0},
        {"Naked Quad", 2.5}, {"Hidden Quad", 3.0},
        // Tier 3: each step outweighs any realistic stack of Tier 2 steps.
        {"X-Wing", 15.0}, {"Skyscraper", 15.0}, {"2-String Kite", 15.0},
        {"XY-Wing", 16.0}, {"XYZ-Wing", 17.0},
        {"Swordfish", 20.0}, {"Simple Coloring", 18.0}, {"Jellyfish", 25.0},
    };
    searchBase = 40.0;  searchPerDepth = 5.0;

    // Thresholds calibrated on qqwing batches (tests/calibrate.cpp), then validated on disjoint hold-out sets.
    // Difficulty: gap between easy (max 0.9) and intermediate (min 1.6); every Tier 3 step weighs >= 15.
    // Length / complexity: tertiles (p33 / p66) of a pool balanced across the four qqwing levels.
    difficultyMedium = 1.2; difficultyHigh = 14.0;
    lengthMedium = 56;      lengthLong = 58;
    complexityMedium = 65;  complexityHigh = 82;
}

double Rater::scoreOf(const SolveStats& s) const {
    double score = 0;
    for (auto& [name, count] : s.techniqueSteps) {
        auto it = weights.find(name);
        score += (it != weights.end() ? it->second : 1.0) * count;
    }
    if (s.usedSearch) score += searchBase + searchPerDepth * s.searchDepth;
    return score;
}

static std::string band3(double v, double mid, double high, const char* lo, const char* md, const char* hi) {
    return v < mid ? lo : (v < high ? md : hi);
}

Rating Rater::rate(const SolveStats& s) const {
    Rating r;
    r.difficultyScore = scoreOf(s);
    r.difficulty = band3(r.difficultyScore, difficultyMedium, difficultyHigh, "Low", "Medium", "High");
    r.steps = s.totalSteps();
    r.length = band3(r.steps, lengthMedium, lengthLong, "Short", "Medium", "Long");
    r.iterations = s.iterations();
    r.complexity = band3(static_cast<double>(r.iterations), static_cast<double>(complexityMedium),
                         static_cast<double>(complexityHigh), "Low", "Medium", "High");
    return r;
}

static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
    return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}

bool Rater::loadConfig(const std::string& path, std::string& error) {
    std::ifstream f(path);
    if (!f) { error = "cannot open " + path; return false; }
    std::string line;
    int lineNo = 0;
    while (std::getline(f, line)) {
        ++lineNo;
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        size_t eq = line.rfind('=');
        if (eq == std::string::npos) { error = "line " + std::to_string(lineNo) + ": expected key = value"; return false; }
        std::string key = trim(line.substr(0, eq));
        double val;
        try { val = std::stod(trim(line.substr(eq + 1))); }
        catch (...) { error = "line " + std::to_string(lineNo) + ": bad number"; return false; }

        if (key.rfind("weight.", 0) == 0) weights[key.substr(7)] = val;
        else if (key == "search.base") searchBase = val;
        else if (key == "search.perDepth") searchPerDepth = val;
        else if (key == "difficulty.medium") difficultyMedium = val;
        else if (key == "difficulty.high") difficultyHigh = val;
        else if (key == "length.medium") lengthMedium = static_cast<int>(val);
        else if (key == "length.long") lengthLong = static_cast<int>(val);
        else if (key == "complexity.medium") complexityMedium = static_cast<long>(val);
        else if (key == "complexity.high") complexityHigh = static_cast<long>(val);
        else { error = "line " + std::to_string(lineNo) + ": unknown key '" + key + "'"; return false; }
    }
    return true;
}

std::string Rater::configText() const {
    std::ostringstream o;
    o << "# Rating configuration. Lines are 'key = value'; '#' starts a comment.\n"
      << "# Difficulty score = sum(weight x steps per technique) + search penalty.\n"
      << "# A value below the 'medium' threshold rates Low/Short, below 'high'/'long' rates Medium, else High/Long.\n\n"
      << "# --- technique weights (points per logic step) ---\n";
    for (auto& [name, w] : weights) o << "weight." << name << " = " << w << "\n";
    o << "\n# --- Tier 4 (guessing) penalty ---\n"
      << "search.base = " << searchBase << "\n"
      << "search.perDepth = " << searchPerDepth << "\n"
      << "\n# --- difficulty bands (on the weighted score) ---\n"
      << "difficulty.medium = " << difficultyMedium << "\n"
      << "difficulty.high = " << difficultyHigh << "\n"
      << "\n# --- length bands (logic steps + cells filled by search) ---\n"
      << "length.medium = " << lengthMedium << "\n"
      << "length.long = " << lengthLong << "\n"
      << "\n# --- complexity bands (technique probes + search assignments) ---\n"
      << "complexity.medium = " << complexityMedium << "\n"
      << "complexity.high = " << complexityHigh << "\n";
    return o.str();
}
