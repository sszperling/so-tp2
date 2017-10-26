.PHONY : all clean run new test-1 test-2 test-3 test-4 test-5

MPIROOT = /usr

CFLAGS+= -Wall -g $(INCL)
CC=gcc
MPICC=  $(MPIROOT)/bin/mpic++
INCL= -I$(MPIROOT)/include
SRCS= consola.cpp main.cpp nodo.cpp HashMap.cpp
BIN= dist_hashmap

all: dist_hashmap

$(BIN): $(SRCS)
	$(MPICC) $(CFLAGS) -o $(BIN) $(SRCS)

test-all: test-1 test-2 test-3 test-4 test-5

test-1: dist_hashmap
	cat test-1.in |  mpiexec -np 3 ./dist_hashmap | tee test-1.out
	grep -f test-1.expected test-1.out
	rm -f test-1.out

test-2: dist_hashmap
	for i in 0 1 2 3 4; do sed -n "$$((i * 500 + 1)),$$(((i + 1) * 500))p" corpus >corpus-"$$i"; done
	cat test-2.in |  mpiexec -np 3 ./dist_hashmap | tee test-2.out
	grep -f test-2.expected test-2.out
	rm -f test-2.out
	rm -f corpus-[0-4]

test-3: dist_hashmap
	cat test-3.in |  mpiexec -np 3 ./dist_hashmap | tee test-3.out
	grep -f test-3.expected test-3.out
	rm -f test-3.out

test-4: dist_hashmap
	cat test-4.in |  mpiexec -np 3 ./dist_hashmap | tee test-4.out
	grep -f test-4.expected test-4.out
	rm -f test-4.out

test-5: dist_hashmap
	cat test-5.in |  mpiexec -np 3 ./dist_hashmap | tee test-5.out
	grep -f test-5.expected test-5.out
	rm -f test-5.out

clean:
	rm -f $(BIN)

new: clean all
