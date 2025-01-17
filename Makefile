ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
#LDFLAGS=-L/usr/local/lib -lpthread -latomic -Ltbb2020/lib/intel64/gcc4.8 -ltbb
LDFLAGS=-L/usr/local/lib -lpthread -latomic -Ltbb2020/lib/intel64/gcc4.8 -ltbb -g
BLISS_LDFLAGS=-L$(ROOT_DIR)/core/bliss-0.73/ -lbliss
#CFLAGS=-O3 -std=c++2a -Wall -Wextra -Wpedantic -fPIC -fconcepts -I$(ROOT_DIR)/core/ -Itbb2020/include
CFLAGS=-std=c++2a -Wall -Wextra -Wpedantic -fPIC -fconcepts -I$(ROOT_DIR)/core/ -Itbb2020/include
#CFLAGS=-std=c++2a -Wall -Wextra -Wpedantic -fPIC -fconcepts -I$(ROOT_DIR)/core/ -Itbb2020/include
OBJ=core/DataGraph.o core/PO.o core/utils.o core/PatternGenerator.o $(ROOT_DIR)/core/showg.o
OUTDIR=bin/
CC=g++

all: bliss fsm convert_data

core/roaring.o: core/roaring/roaring.c
	gcc -c core/roaring/roaring.c -o $@ -O3 -Wall -Wextra -Wpedantic -fPIC 

%.o: %.cc
	$(CC) -c $? -o $@ $(CFLAGS)

fsm: apps/fsm.cc $(OBJ) core/roaring.o bliss
	$(CC) apps/fsm.cc $(OBJ) core/roaring.o -o $(OUTDIR)/$@ $(BLISS_LDFLAGS) $(LDFLAGS) $(CFLAGS)

#output: apps/output.cc $(OBJ) bliss
#	$(CC) apps/output.cc $(OBJ) -o $(OUTDIR)/$@ $(BLISS_LDFLAGS) $(LDFLAGS) $(CFLAGS)


convert_data: core/convert_data.cc core/DataConverter.o core/utils.o
	$(CC) -o $(OUTDIR)/$@ $? $(LDFLAGS) $(CFLAGS)

bliss:
	make -C ./core/bliss-0.73

clean:
	make -C ./core/bliss-0.73 clean
	rm -f core/*.o bin/*
