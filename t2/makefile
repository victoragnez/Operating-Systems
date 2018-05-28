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
OPTIONSCXX = $(STD) $(WARNING)

# directories
DECLRDIR = -I ./include
SRCDIR = ./src/
BINDIR = ./bin/

EXEC = gpio


### executable target
main: bin CPU.o GPIO_pins.o GPIO.o main.o
	$(CXX) $(BINDIR)CPU.o $(BINDIR)GPIO_pins.o $(BINDIR)GPIO.o $(BINDIR)main.o \
	-o $(EXEC) $(OPTIONSCXX)

### bin folder (created only if it doesnt exist)
bin:
	mkdir -p ./bin/

%.o: $(SRCDIR)%.cpp
	$(CXX) -c $< -o $(BINDIR)$@ $(OPTIONSCXX) $(DECLRDIR)

main.o: $(SRCDIR)main.cpp
GPIO.o: $(SRCDIR)GPIO.cpp
GPIO_pins.o: $(SRCDIR)GPIO_pins.cpp
CPU.o: $(SRCDIR)CPU.cpp


### clear objects & executable
clean: bin
	$(DEL) ./$(EXEC)
	$(DEL) $(BINDIR)*.o
