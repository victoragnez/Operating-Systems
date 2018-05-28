# commands
ifeq ($(USER),sobuntu)
    CXX = arm-linux-gnueabihf-g++
else
    CXX = g++
endif

DEL = rm

# options
DEBUG = -g
WARNING = -Wall
OPTMIZE = -O3
STD = -std=c++11
THREAD = -pthread
NCURSES = -lncurses
OPTIONSCXX = $(STD) $(WARNING) $(THREAD) $(NCURSES)

# directories
DECLRDIR = -I ./include
SRCDIR = ./src/
BINDIR = ./bin/

EXEC = pong


### executable target
main: bin POINT.o server.o
	$(CXX) $(BINDIR)POINT.o $(BINDIR)server.o \
	-o $(EXEC) $(OPTIONSCXX)

### bin folder (created only if it doesnt exist)
bin:
	mkdir -p ./bin/

%.o: $(SRCDIR)%.cpp
	$(CXX) -c $< -o $(BINDIR)$@ $(OPTIONSCXX) $(DECLRDIR)

server.o: $(SRCDIR)server.cpp
POINT.o: $(SRCDIR)POINT.cpp

### clear objects & executable
clean: bin
	$(DEL) ./$(EXEC)
	$(DEL) $(BINDIR)*.o
