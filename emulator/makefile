
.PHONY: clean
IDIR=../h
OBJDIR=../obj/emulator
SRCDIR=../src
EMDIR=./
CC=g++
CFLAGS=-I$(IDIR)
ARCH=-m32 -std=c++17 -static -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
PROGRAM=../emul


SRC = $(wildcard $(SRCDIR)/*.cpp)
SRC1 = $(wildcard $(EMDIR)/*.cpp)
OBJ = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRC))
OBJ += $(patsubst $(EMDIR)/%.cpp,$(OBJDIR)/%.o,$(SRC1))


$(PROGRAM): $(OBJ)
	$(CC) -g -o $@ $^ $(ARCH)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CC) -g -o $@ -c $< $(CFLAGS) $(ARCH)

$(OBJDIR)/%.o: $(EMDIR)/%.cpp
	$(CC) -g -o $@ -c $< $(CFLAGS) $(ARCH)

clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(PROGRAM)
 
.PHONY: clean