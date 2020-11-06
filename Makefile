# Linux (default)
EXE = gsh

# Windows (cygwin)
ifeq ($(OS), Windows_NT)
	EXE = gsh.exe
endif

list.o: list.c list.h
	gcc -c $<

all: gsh.c list.o
	gcc -o $(EXE) $^