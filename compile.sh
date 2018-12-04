#!/usr/bin/env bash

exec g++ -g -Wall -std=gnu++17 -o modpro src/*.cxx -ljack -ldl -lpthread -lyaml-cpp
