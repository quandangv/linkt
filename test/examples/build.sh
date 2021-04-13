#!/bin/bash
mkdir -p build
# Build the executables
g++ simple.cpp -llinked_lang -llinked_node -std=c++17 -o build/simple

echo "Run './build/simple' from the directory of this file to view the examples"
