CC := g++
CCFLAG := -std=c++11 -Wall
CCOBJFLAG := $(CCFLAG) -c
LIBFLAG := -lz3

main: main.o Architecture.o Solver.o
	$(CC) $(CCFLAG) $(LIBFLAG) -o $@ $^

%.o: %.c*
	$(CC) $(CCOBJFLAG) -o $@ $< 

debug: main_dbg.o Architecture_dbg.o Solver_dbg.o 
	$(CC) $(CCFLAG) $(LIBFLAG) -g -o $@ $^

%_dbg.o: %.c*
	$(CC) $(CCOBJFLAG) -g -o $@ $< 

test: test.o
	$(CC) $(CCFLAG) $(LIBFLAG) -o $@ $^

.PHONY: clean
clean:
	rm *.o
	rm main
	rm debug
