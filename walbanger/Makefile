CXXFLAGS := --std=c++11
LDFLAGS  := -lsqlite3

EXE      := walbanger
DBNAME   := ~/tmp/$(EXE).db

all: $(EXE)
	
clean:
	rm -f *.o $(EXE)

run: all
	@./$(EXE) -pr $(DBNAME)

$(EXE): harvey.o
	$(CXX) $< -o $(EXE) $(LDFLAGS)
