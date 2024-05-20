CC=gcc
WARN=-Wall -Werror
CCFLAGS=$(WARN)
INCLUDE_DIR=src/util/*

all: utils client server

server: src/server/server.c
	$(CC) src/server/server.c -c -o bin/server/server.o -Isrc/util/parser/ -Isrc/util/music/ -Isrc/util/codec/ $(CCFLAGS)
	$(CC) bin/music.o bin/parser.o bin/codec.o bin/server/server.o -o server.out $(CCFLAGS)

client: src/client/client.c
	$(CC) src/client/client.c -c -o bin/client/client.o -Isrc/util/parser/ -Isrc/util/music/ -Isrc/util/codec/ $(CCFLAGS)
	$(CC) bin/music.o bin/parser.o bin/codec.o bin/client/client.o -o client.out $(CCFLAGS)

utils: src/util/*
	$(CC) src/util/parser/parser.c -c -o bin/parser.o $(CCFLAGS)
	$(CC) src/util/music/music.c -c -o bin/music.o -Isrc/util/parser/ $(CCFLAGS)
	$(CC) src/util/codec/codec.c -c -o bin/codec.o -Isrc/util/music/ -Isrc/util/parser/ $(CCFLAGS)