IDIR=h
OBJDIR=obj
SRCDIR=src
CC=g++
CFLAGS=-I$(IDIR)
ARCH=-std=c++11 -m32 -static
PROGRAM=assembler

SRC = $(wildcard $(SRCDIR)/*.cpp)
OBJ = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRC))

$(PROGRAM): $(OBJ)
	$(CC) -o $@ $^ $(ARCH)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CC) -o $@ -c $< $(CFLAGS) $(ARCH)


clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(PROGRAM)

.PHONY: clean