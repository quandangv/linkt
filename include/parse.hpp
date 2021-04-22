#pragma once
#include "node/wrapper.hpp"
#include <iostream>

void parse_ini(std::istream&, node::errorlist&, node::wrapper_s& output);
void parse_yml(std::istream&, node::errorlist&, node::wrapper_s& output);

