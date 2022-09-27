CXXFLAGS := -g -O0 --std=c++11
LDFLAGS  := -lsqlite3

EXE      := walbanger
DBNAME   := ~/tmp/$(EXE).db
DBVOL    := $(EXE)-vol
VERSION  := $(shell ../tools/version)
FILEURL  := $(shell ../tools/fileurl $(VERSION))

all: $(EXE)
	
clean:
	@rm -f *.o $(EXE)
	@-docker container rm $(EXE)-$(VERSION) > /dev/null 2>&1
	@-docker service   rm $(EXE)            > /dev/null 2>&1
	@-docker image     rm $(EXE):$(VERSION) > /dev/null 2>&1

container-image:
	docker build \
		--build-arg FILEURL="$(FILEURL)" \
		--tag $(EXE):$(VERSION) .

container-run: container-image
	@-docker container rm $(EXE)-$(VERSION) > /dev/null 2>&1
	docker container create \
		--interactive --tty \
		--volume ~/tmp:/db \
		--name $(EXE)-$(VERSION) \
		       $(EXE):$(VERSION)
	docker start -ai $(EXE)-$(VERSION)

single: container-run

swarm: container-image
	@-docker service rm $(EXE) > /dev/null 2>&1
	@if ! docker volume inspect $(DBVOL) > /dev/null 2>&1 ; then \
		docker volume create --name $(DBVOL) ; \
	fi
	docker service create \
		--name $(EXE) \
		--replicas 3 \
		--mount source=$(DBVOL),destination=/db \
		--constraint node.role==manager \
		$(EXE):$(VERSION)

run: all
	@./$(EXE) -pr $(DBNAME)

$(EXE): harvey.o
	$(CXX) $< -o $(EXE) $(LDFLAGS)
