CC = gcc -pedantic -Wall -fopenmp
FLAGS = -I/opt/X11/include 
UNOPTIMIZED_FLAGS = -I/opt/X11/include
DEBUG = -g
C_LINK_OPTIONS = -L/opt/X11/lib -lX11 -lm

EXEC_NAME = base_solver

all: $(EXEC_NAME)

$(EXEC_NAME): $(EXEC_NAME).o 
	$(CC) $(FLAGS) $^ -o $@ 

%.o: %.c
	$(CC) $(FLAGS) -c $< -o $@

$(EXEC_NAME)_debug: $(EXEC_NAME).c
	$(CC) $(FLAGS) $(DEBUG) -o $@ $< 

run: $(EXEC_NAME)
	./$(EXEC_NAME)

valgrind: $(EXEC_NAME)_debug
	valgrind --leak-check=full ./$

cache: $(EXEC_NAME)
	valgrind --tool=cachegrind --branch-sim=yes ./$

clean:
	rm -f $(EXEC_NAME) $(EXEC_NAME)_debug *.o graphics/*.o result.gal

.PHONY: all clean run valgrind cache
