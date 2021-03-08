#!/bin/bash
mkdir -p build
# Build the executables
g++ simple.cpp -llinked_nodes -llinked_nodes_node -std=c++17 -o build/simple
g++ lemonbar.cpp -llinked_nodes -llinked_nodes_node -std=c++17 -o build/lemonbar

echo "Run './build/lemonbar | lemonbar' or './build/simple' from the directory of this file to view the examples"
