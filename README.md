# Puzzle Solving Using Modern Sat Solvers
The Sokoban puzzle can be encoded and represented as a Bounded Model Checking (BMC) problem and solved using an SAT solver. By adopting BMC, we can determine the shortest sequence of movements required to complete the game.

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
The sokoban solver codes are within src/ext-lsv
First execute abc:

```sh
./abc
```

then run the sokoban solver with the following command:

```sh
sokoban <map_file_path> <run_type>
```

for example:

```sh
sokoban maps/map1.txt 1
```

(1 for BMC, 2 for BMC with binary search)
