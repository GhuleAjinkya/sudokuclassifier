@echo off
REM Compiles every source file and runs the solver. Any arguments are passed through, e.g.:
REM   run.bat --puzzle 530070000600195000098000060800060003400803001700020006060000280000419005000080079
REM   run.bat --file puzzles.txt
REM   run.bat --qqwing-path "C:\tools\qqwing.exe" --qqwing expert 20
g++ -std=c++17 -O2 src\Grid.cpp src\Techniques.cpp src\Tier3.cpp src\Tier4.cpp src\Solver.cpp src\Rater.cpp src\QQWingBridge.cpp src\main.cpp -o sudoku_solver.exe && sudoku_solver.exe %*
pause
