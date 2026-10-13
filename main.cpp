#include "Grid.h"
#include "Solver.h"
#include <iostream>
using namespace std;

int main() {
    string puzzle =
        "530070000"
        "600195000"
        "098000060"
        "800060003"
        "400803001"
        "700020006"
        "060000280"
        "000419005"
        "000080070";

    Grid grid = Grid::fromString(puzzle);

    cout << "Starting Grid\n";
    grid.print();
    cout << "\n";

    Solver solver;
    vector<PlacementLog> log;
    bool solved = solver.solve(grid, log);

    cout << "Placements (" << log.size() << ")\n";
    for (auto& entry : log) {
        cout << "Placed " << entry.digit
                   << " at (row " << entry.row + 1 << ", col " << entry.col + 1 << ")"
                   << " via " << entry.method << "\n";
    }

    cout << "\nFinal Grid\n";
    grid.print();

    cout << "\nSolved by logic alone: " << (solved ? "yes" : "no") << "\n";
    if (!solved) {
        cout << "(Remaining cells would need more complex techniques or backtracking.)\n";
    }
    int temp;
    cin >> temp;
    return 0;
}