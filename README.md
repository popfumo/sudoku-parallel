# Parallel Sudoku Solver using OpenMP  
**High Performance Programming (1TD062) - Spring 2025**  

## Running the Program  

### Compilation and Execution  
- **Compile:** `make`  
- **Running:** `./solver <board_size> <threads> <cutoff>`
- **Run with Valgrind:** `make valgrind`  
- **Run with Cachegrind:** `make cachegrind`  
- **Clean Build Files:** `make clean`  
- **Performance Testing:**  
  - Set `#DEFINE PERFORMANCE` to `1` in `solver.c` before compiling.
- **Heap Version:**
  - Set `#DEFINE HEAP_ALLOCATION` to `1` in `solver.c` before compiling.