#pragma once
#include <iosfwd>
#include <string>
#include <vector>

// Reads puzzles from a text stream, one per line. Accepts 81-char lines using
// '1'-'9' for givens and '0' or '.' for blanks (qqwing --one-line format).
// Lines that aren't valid puzzles (headers, blank lines, solutions-only noise) are skipped.
std::vector<std::string> loadPuzzles(std::istream& in);
std::vector<std::string> loadPuzzlesFromFile(const std::string& path);

// Runs the external qqwing program:  qqwing --generate <count> --difficulty <d> --one-line
// difficulty must be one of: simple, easy, intermediate, expert, any.
// Returns an empty vector (and fills `error`) if qqwing can't be launched or the
// arguments are invalid.
std::vector<std::string> generateWithQQWing(const std::string& difficulty, int count,
                                            const std::string& qqwingPath, std::string& error);
