#include "QQWingBridge.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

#ifdef _WIN32
  #define POPEN  _popen
  #define PCLOSE _pclose
#else
  #define POPEN  popen
  #define PCLOSE pclose
#endif

static bool isPuzzleLine(const std::string& s) {
    if (s.size() < 81) return false;
    for (int i = 0; i < 81; ++i) {
        char c = s[i];
        if (!((c >= '0' && c <= '9') || c == '.')) return false;
    }
    return true;
}

std::vector<std::string> loadPuzzles(std::istream& in) {
    std::vector<std::string> out;
    std::string line;
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        if (isPuzzleLine(line)) out.push_back(line.substr(0, 81));
    }
    return out;
}

std::vector<std::string> loadPuzzlesFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) return {};
    return loadPuzzles(f);
}

std::vector<std::string> generateWithQQWing(const std::string& difficulty, int count,
                                            const std::string& qqwingPath, std::string& error) {
    // The command line is built from user input, so only allow known-safe values.
    static const char* allowed[] = {"simple", "easy", "intermediate", "expert", "any"};
    bool okDifficulty = false;
    for (auto* a : allowed) if (difficulty == a) okDifficulty = true;
    if (!okDifficulty) { error = "difficulty must be simple, easy, intermediate, expert or any"; return {}; }
    if (count < 1 || count > 100000) { error = "count must be between 1 and 100000"; return {}; }
    if (qqwingPath.find('"') != std::string::npos) { error = "qqwing path must not contain quotes"; return {}; }

    std::ostringstream cmd;
    cmd << '"' << qqwingPath << '"' << " --generate " << count
        << " --difficulty " << difficulty << " --one-line";
    std::string command = cmd.str();
#ifdef _WIN32
    command = "\"" + command + "\"";   // cmd /c quirk: wrap the whole line when it starts with a quote
#endif

    FILE* pipe = POPEN(command.c_str(), "r");
    if (!pipe) { error = "could not launch qqwing (is it installed / on PATH, or use --qqwing-path?)"; return {}; }

    std::string output;
    char buf[512];
    while (fgets(buf, sizeof buf, pipe)) output += buf;
    PCLOSE(pipe);

    std::istringstream ss(output);
    auto puzzles = loadPuzzles(ss);
    if (puzzles.empty()) error = "qqwing produced no puzzles (is it installed / on PATH, or use --qqwing-path?)";
    return puzzles;
}
