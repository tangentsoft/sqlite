FROM alpine:latest

RUN set -x \
    && apk update \
    && apk upgrade --no-cache \
    && apk add g++ sqlite-dev
ARG FILEURL=https://tangentsoft.com/sqlite/raw?ci=trunk&filename=
ADD ${FILEURL}walbanger/harvey.cpp /harvey.cpp
RUN g++ -O2 harvey.cpp -o walbanger -lsqlite3

ENTRYPOINT [ "/walbanger", "-p", "/db/walbanger.db" ]
