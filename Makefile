CC = gcc -pedantic -Wall -fopenmp
FLAGS = -Ofast -march=native -funroll-loops -ftree-vectorize -fopt-info-vec-optimized -fopenmp 
PROFILE = -pg
DEBUG = -g

EXEC_NAME = solver

all: $(EXEC_NAME)

$(EXEC_NAME): $(EXEC_NAME).o verify.o
	$(CC) $(FLAGS) $^ -o $@ 

%.o: %.c
	$(CC) $(FLAGS) -c $< -o $@

$(EXEC_NAME)_debug: $(EXEC_NAME).c verify.c
	$(CC) $(FLAGS) $(DEBUG) -o $@ $^ 

$(EXEC_NAME)_profile: $(EXEC_NAME).c verify.c
	$(CC) $(FLAGS) $(PROFILE) -o $@ $^ 

run: $(EXEC_NAME)
	./$(EXEC_NAME)

valgrind: $(EXEC_NAME)_debug verify.c
	valgrind --leak-check=full ./$(EXEC_NAME) 64 4 100

cache: $(EXEC_NAME) verify.c
	valgrind --tool=cachegrind --branch-sim=yes ./$(EXEC_NAME) 64 4 100

clean:
	rm -f $(EXEC_NAME) $(EXEC_NAME)_debug $(EXEC_NAME)_profile *.o 

.PHONY: all clean run valgrind cache
