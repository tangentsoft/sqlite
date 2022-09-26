CXXFLAGS := -g -O0 --std=c++11
LDFLAGS  := -lsqlite3

EXE      := walbanger
DBNAME   := ~/tmp/$(EXE).db
VERSION  := $(shell ../tools/version)

all: $(EXE)
	
clean:
	@rm -f *.o $(EXE)
	@-docker image     rm $(EXE):$(VERSION)
	@-docker container rm $(EXE)-$(VERSION)

container-image:
	docker build --tag $(EXE):$(VERSION) .

container-run: container-image
	@-docker container rm $(EXE)-$(VERSION)
	docker container create \
		--interactive --tty \
		--volume ~/tmp:/db \
		--name $(EXE)-$(VERSION) \
		       $(EXE):$(VERSION)
	docker cp entrypoint.sh $(EXE)-$(VERSION):/
	docker cp harvey.cpp    $(EXE)-$(VERSION):/
	docker start -ai $(EXE)-$(VERSION)

single: container-run

run: all
	@./$(EXE) -pr $(DBNAME)

$(EXE): harvey.o
	$(CXX) $< -o $(EXE) $(LDFLAGS)
