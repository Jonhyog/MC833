CC=gcc
WARN=-Wall -Werror
CCFLAGS=$(WARN)
INCLUDE_DIR=src/util/*

all: test

test: src/test.c utils
	$(CC) src/test.c -c -o bin/test.o -Isrc/util/parser/ -Isrc/util/music/ $(CCFLAGS)
	$(CC) bin/music.o bin/parser.o bin/test.o -o test.out $(CCFLAGS)

utils: src/util/*
	$(CC) src/util/parser/parser.c -c -o bin/parser.o $(CCFLAGS)
	$(CC) src/util/music/music.c -c -o bin/music.o -Isrc/util/parser/ $(CCFLAGS)