CXX = mpicxx
CXXFLAGS = -std=c++11 -Wall -O3 -g
INCLUDE = -I../../include $(shell pkg-config --cflags hpx)
LIBS = -L../../lib -ldashmm $(shell pkg-config --libs hpx)

SRC = stepping.cc
OBJ = $(SRC:.cc=.o)

EXEC = stepping

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CXX) -o $(EXEC) $(OBJ) $(LIBS)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDE)

clean:
	rm -rf $(EXEC) $(OBJ) *~
