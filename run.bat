@echo off
g++ -std=c++17 -O2 Grid.cpp Techniques.cpp Tier3.cpp Solver.cpp QQWingBridge.cpp main.cpp -o sudoku_solver.exe && sudoku_solver.exe %*
pause