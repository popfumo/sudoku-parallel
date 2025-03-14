CC = gcc -pedantic -Wall -fopenmp
FLAGS = -Ofast -march=native -funroll-loops -ftree-vectorize -fopt-info-vec-optimized -fopenmp 
PROFILE = -pg
DEBUG = -g
C_LINK_OPTIONS = -L/opt/X11/lib -lX11 -lm

EXEC_NAME = solver_stack

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

valgrind: $(EXEC_NAME)_debug
	valgrind --leak-check=full ./$

cache: $(EXEC_NAME)
	valgrind --tool=cachegrind --branch-sim=yes ./$

clean:
	rm -f $(EXEC_NAME) $(EXEC_NAME)_debug $(EXEC_NAME)_profile *.o 

.PHONY: all clean run valgrind cache
