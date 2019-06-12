CC := g++
CCFLAG := -std=c++11 -Wall
CCOBJFLAG := $(CCFLAG) -c
LIBFLAG := -lz3

main: main.o Architecture.o
	$(CC) $(CCFLAG) $(LIBFLAG) -o $@ $^

%.o: %.c*
	$(CC) $(CCOBJFLAG) -o $@ $< 

.PHONY: clean
clean:
	rm *.o
	rm main
