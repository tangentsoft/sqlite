#!/bin/sh
if [ ! -e walbanger ] || [ harvey.cpp -nt walbanger ]
then
    g++ -O2 harvey.cpp -o walbanger -lsqlite3
fi
/walbanger /db/walbanger.db
