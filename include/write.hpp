#pragma once
#include "node/wrapper.hpp"
#include <iostream>

void write_ini(std::ostream&, const node::wrapper_s&, const string& prefix = "");
void write_yml(std::ostream&, const node::wrapper_s&, int indent = 0);
