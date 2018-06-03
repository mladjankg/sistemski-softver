IDIR=h
OBJDIR=obj
SRCDIR=src
CC=g++
CFLAGS=-I$(IDIR)
ARCH=-m32 -std=c++11 -static
PROGRAM=assembler

SRC = $(wildcard $(SRCDIR)/*.cpp)
OBJ = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRC))

$(PROGRAM): $(OBJ)
	$(CC) -g -o $@ $^ $(ARCH)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CC) -g -o $@ -c $< $(CFLAGS) $(ARCH)


clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(PROGRAM)
 
.PHONY: clean