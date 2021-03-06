CXX = mpicxx
CXXFLAGS = -std=c++11 -Wall -O3 -g -I./include/
INCLUDE = $(shell pkg-config --cflags hpx)
LIBS = $(shell pkg-config --libs hpx)

SRC = $(shell ls src/*.cc)
OBJ = $(SRC:.cc=.o)

DASHMM = lib/libdashmm.a

all: $(DASHMM)

$(DASHMM): $(OBJ)
	ar -cvq $(DASHMM) $(OBJ)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDE)

clean:
	rm -rf $(DASHMM) $(OBJ) *~
