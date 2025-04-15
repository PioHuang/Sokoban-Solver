# A formal verification project: puzzle solving using modern Sat Solvers

The Sokoban puzzle, a P-Space Complete problem, can be formally encoded and represented as a Bounded Model Checking (BMC) problem and solved using a SAT solver. This work implements a SAT-based Sokoban Solver while exploring multiple means to reduce variable/clause count, heuristics. In comparison to other simulation-based solvers (IDA\*, MCTS, etc.), this solver can find the shortest sequence of movements required to complete the puzzle. It also supports finding optimal solution for multi-agent on board, and it may also solve for testcases for which other solvers cannot solve in time (1 hour time limit).
Microban test set is used as testcases, located in folder /filled.

## Installation

To compile the project, you will need a C++ compiler and the ABC framework.

1. Clone the repository:

   ```sh
   git clone https://github.com/PioHuang/Sokoban-Solver
   cd Sokoban-Solver
   ```

2. Make the project:
   ```sh
   make
   ```

## Usage

The sokoban solver is integrated into the Berkeley ABC framework.
The sokoban solver, Preprocessor codes are located within src/ext-lsv
First execute abc:

```sh
./abc
```

then run the sokoban solver with the following command:

```sh
sokoban <map_file_path> <run_type> <verbose>
```

for example:

```sh
sokoban filled/microban_1.txt 1 1
```

### Runtypes

1 for BMC only, 2 for BMC with binary search
