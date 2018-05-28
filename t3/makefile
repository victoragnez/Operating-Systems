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
main: bin pins_tables.o GPIO.o POINT.o main.o
	$(CXX) $(BINDIR)pins_tables.o $(BINDIR)GPIO.o $(BINDIR)POINT.o $(BINDIR)main.o \
	-o $(EXEC) $(OPTIONSCXX)

### bin folder (created only if it doesnt exist)
bin:
	mkdir -p ./bin/

%.o: $(SRCDIR)%.cpp
	$(CXX) -c $< -o $(BINDIR)$@ $(OPTIONSCXX) $(DECLRDIR)

main.o: $(SRCDIR)main.cpp
GPIO.o: $(SRCDIR)GPIO.cpp
POINT.o: $(SRCDIR)POINT.cpp
pins_tables.o: $(SRCDIR)pins_tables.cpp


### clear objects & executable
clean: bin
	$(DEL) ./$(EXEC)
	$(DEL) $(BINDIR)*.o
