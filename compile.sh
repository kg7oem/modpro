#!/usr/bin/env bash

#dbusxx-xml2cpp src/dbus-adaptor.xml --adaptor=src/dbus-adaptor.h
#g++ -g -Wall -std=gnu++17 -o modpro src/*.cxx -ljack -ldl -lpthread -lyaml-cpp $(pkg-config dbus-c++-1 --cflags --libs)
g++ -g -Wall -std=gnu++17 -o modpro src/{pulsar,pulsar-ladspa,main}.cxx -ljack -ldl -lpthread -lyaml-cpp $(pkg-config dbus-c++-1 --cflags --libs)
