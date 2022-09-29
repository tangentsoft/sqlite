CXXFLAGS := -O2 --std=c++11
LDFLAGS  := -lsqlite3

EXE      := walbanger
DBDIR    := ~/tmp
DBNAME   := $(DBDIR)/$(EXE).db
VERSION  := $(shell ../tools/version)
FILEURL  := $(shell ../tools/fileurl $(VERSION))

all: $(EXE)
	
clean:
	@rm -f *.o $(EXE)
	@-docker container rm $(EXE)-$(VERSION) > /dev/null 2>&1
	@-docker service   rm $(EXE)            > /dev/null 2>&1
	@-docker volume    rm $(EXE)            > /dev/null 2>&1
	@-docker image     rm $(EXE):$(VERSION) > /dev/null 2>&1

container-image:
	docker build \
		--build-arg FILEURL="$(FILEURL)" \
		--tag $(EXE):$(VERSION) .

container-volume:
	@if ! docker volume inspect $(EXE) > /dev/null 2>&1 ; then \
		docker volume create --name $(EXE) ; \
	fi

run: all
	@./$(EXE) -pr $(DBNAME)

solo: container-image container-volume
	@-docker container rm $(EXE)-$(VERSION) > /dev/null 2>&1
	docker container create \
		--cap-drop AUDIT_WRITE \
		--cap-drop CHOWN \
		--cap-drop DAC_OVERRIDE \
		--cap-drop FOWNER \
		--cap-drop FSETID \
		--cap-drop KILL \
		--cap-drop MKNOD \
		--cap-drop NET_BIND_SERVICE \
		--cap-drop NET_RAW \
		--cap-drop SETFCAP \
		--cap-drop SETGID \
		--cap-drop SETPCAP \
		--cap-drop SETUID \
		--cap-drop SYS_CHROOT \
		--interactive --tty \
			--volume $(EXE):/db \
		--name $(EXE)-$(VERSION) \
		       $(EXE):$(VERSION)
	docker start -ai $(EXE)-$(VERSION)

swarm: container-image container-volume swarm-stop
	docker service create \
		--cap-drop AUDIT_WRITE \
		--cap-drop CHOWN \
		--cap-drop DAC_OVERRIDE \
		--cap-drop FOWNER \
		--cap-drop FSETID \
		--cap-drop KILL \
		--cap-drop MKNOD \
		--cap-drop NET_BIND_SERVICE \
		--cap-drop NET_RAW \
		--cap-drop SETFCAP \
		--cap-drop SETGID \
		--cap-drop SETPCAP \
		--cap-drop SETUID \
		--cap-drop SYS_CHROOT \
		--name $(EXE) \
		--replicas 3 \
		--mount source=$(EXE),destination=/db \
		--constraint node.role==manager \
		$(EXE):$(VERSION)

swarm-shell:
	@docker exec -it -w /db $$(docker ps -q -f name='^$(EXE).1.[0-9a-z]+$$') sh

swarm-stop:
	@-docker service rm $(EXE) > /dev/null 2>&1

tags ctags:
	ctags -R .

$(EXE): harvey.o
	$(CXX) $< -o $(EXE) $(LDFLAGS)
